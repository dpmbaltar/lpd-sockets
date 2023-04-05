#include <glib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef G_OS_UNIX
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#ifdef G_OS_WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "tcpserver.h"

/* Cantidad máxima de conexiones */
#define MAX_CONNECTIONS       10
/* Cantidad máxima de hilos */
#define MAX_THREADS           g_get_num_processors()
/* Utilizar threads exclusivos o no */
#define EXCLUSIVE_THREAD_POOL FALSE

G_DEFINE_QUARK(tcp-server-error, tcp_server_error)

struct _TcpServer
{
  uint32_t     addr;
  uint16_t     port;
  int          max_conn;

  GThreadPool *thread_pool;
};

static const char *error_messages[] = {
  [TCP_SERVER_SOCK_ERROR]        = "Error al crear socket",
  [TCP_SERVER_SOCK_BIND_ERROR]   = "Error al enlazar socket",
  [TCP_SERVER_SOCK_LISTEN_ERROR] = "Error al escuchar socket",
  [TCP_SERVER_SOCK_ACCEPT_ERROR] = "Error al aceptar conexión",
};

/* Macro para manejar errores */
#define return_set_error_if(cond, error, code) \
  if (cond) {\
    g_set_error(error, TCP_SERVER_ERROR, code, error_messages[code]);\
    return;\
  }

TcpServer *tcp_server_new_full(uint32_t  addr,
                               uint16_t  port,
                               GFunc     func,
                               gpointer  data,
                               int       max_conn,
                               int       max_threads,
                               bool      excl)
{
  GError *err = NULL;
  TcpServer *srv = (TcpServer*)malloc(sizeof(TcpServer));

  g_return_val_if_fail(srv != NULL, NULL);

  srv->addr = addr;
  srv->port = port;
  srv->max_conn = max_conn;
  srv->thread_pool = g_thread_pool_new(func, data, max_threads, excl, &err);

  if (err != NULL) {
    fprintf(stderr, "%s\n", err->message);
    g_error_free(err);
    return NULL;
  }

  return srv;
}

TcpServer *tcp_server_new(uint32_t  addr,
                          uint16_t  port,
                          GFunc     func,
                          gpointer  data)
{
  return tcp_server_new_full(addr, port, func, data,
                             MAX_CONNECTIONS,
                             MAX_THREADS,
                             EXCLUSIVE_THREAD_POOL);
}

void tcp_server_run(TcpServer  *srv,
                    GError    **err)
{
  int sockfd, connfd, binded, listening;
  struct sockaddr_in srvaddr, cliaddr;
  socklen_t srvaddr_len = sizeof(srvaddr);
  socklen_t cliaddr_len = sizeof(cliaddr);

#ifdef G_OS_WIN32
  /**
   * Inicializar Winsock
   * https://learn.microsoft.com/es-es/windows/win32/winsock/initializing-winsock
   */
  WSADATA wsa_data;
  int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
  if (result != 0) {
    printf("WSAStartup failed: %d\n", result);
    return;
  }
#endif

  /* Asignar IP y puerto */
  memset(&srvaddr, 0, srvaddr_len);
  srvaddr.sin_family = AF_INET;
  srvaddr.sin_addr.s_addr = htonl(srv->addr);
  srvaddr.sin_port = htons(srv->port);

  /* Crear socket */
  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  return_set_error_if(sockfd == -1, err, TCP_SERVER_SOCK_ERROR);
  printf("Socket creado correctamente...\n");

  /* Enlazar socket creado a IP/puerto */
  binded = bind(sockfd, (struct sockaddr*)&srvaddr, srvaddr_len);
  return_set_error_if(binded == -1, err, TCP_SERVER_SOCK_BIND_ERROR);
  printf("Socket enlazado correctamente...\n");

  /* Escuchar puerto */
  listening = listen(sockfd, srv->max_conn);
  return_set_error_if(listening == -1, err, TCP_SERVER_SOCK_LISTEN_ERROR);
  printf("Servidor escuchando puerto %d...\n", srv->port);

  do {
    /* Aceptar conexión de cliente */
    connfd = accept(sockfd, (struct sockaddr*)&cliaddr, &cliaddr_len);
    return_set_error_if(connfd == -1, err, TCP_SERVER_SOCK_ACCEPT_ERROR);
    printf("Conexión aceptada...\n");

    /* Ejecutar función del servidor en otro hilo */
    g_thread_pool_push(srv->thread_pool, GINT_TO_POINTER(connfd), err);

    if (*err != NULL) {
        break;
    }

  } while (TRUE);

#ifdef G_OS_UNIX
  close(sockfd);
#endif

#ifdef G_OS_WIN32
  closesocket(sockfd);
  WSACleanup();
#endif

  printf("Servidor desconectado.\n");
}

void tcp_server_free(TcpServer *srv)
{
  g_thread_pool_free(srv->thread_pool, FALSE, TRUE);
}
