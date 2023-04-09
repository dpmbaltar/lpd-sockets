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
#define MAX_CONN      10
/* Máximo de tiempo de ejecución por hilo (milisegundos) */
#define MAX_IDLE_TIME 10000
/* Cantidad máxima de hilos */
#define MAX_THREADS   g_get_num_processors()
/* Utilizar threads exclusivos o no */
#define EXC_THREADS   false

/* Define el dominio de errores TCP_SERVER_ERROR */
G_DEFINE_QUARK(tcp-server-error, tcp_server_error)

/** Contiene una configuración para un servidor TCP */
struct TcpServer
{
  /** @privatesection */
  uint32_t addr;
  uint16_t port;
  int      max_conn;
  int      max_threads;
  bool     exclusive;
};

/** @private */
typedef struct TcpServerThreadArgs
{
  TcpServerFunc  func;
  void          *data;
} TcpServerThreadArgs;

/* Mensajes de error */
static const char *error_messages[] = {
  [TCP_SERVER_SOCK_ERROR]        = "Error al crear socket",
  [TCP_SERVER_SOCK_BIND_ERROR]   = "Error al enlazar socket",
  [TCP_SERVER_SOCK_LISTEN_ERROR] = "Error al escuchar socket",
  [TCP_SERVER_SOCK_ACCEPT_ERROR] = "Error al aceptar conexión",
};

/* Macro para manejar errores */
#define return_set_error_if(cond, error, code) \
  if (cond) {\
    g_set_error_literal(error, TCP_SERVER_ERROR, code, error_messages[code]);\
    return;\
  }

TcpServer *tcp_server_new_full(uint32_t addr,
                               uint16_t port,
                               int      max_conn,
                               int      max_threads,
                               bool     exclusive)
{
  TcpServer *server = (TcpServer*)malloc(sizeof(TcpServer));

  g_return_val_if_fail(server != NULL, NULL);

  server->addr = addr;
  server->port = port;
  server->max_conn = max_conn;
  server->max_threads = max_threads;
  server->exclusive = exclusive;

  return server;
}

TcpServer *tcp_server_new(uint32_t addr, uint16_t port)
{
  return tcp_server_new_full(addr, port, MAX_CONN, MAX_THREADS, EXC_THREADS);
}

static void run_server_thread(void *sock_ptr, void *data)
{
  int sock = GPOINTER_TO_INT(sock_ptr);
  TcpServerThreadArgs *args = (TcpServerThreadArgs*)data;

  g_return_if_fail(sock != -1);
  g_return_if_fail(args != NULL);

  args->func(sock, args->data);

#ifdef G_OS_UNIX
  close(sock);
#endif

#ifdef G_OS_WIN32
  closesocket(sock);
#endif

  printf("Desconectado del cliente.\n");
}

void tcp_server_run(TcpServer      *server,
                    TcpServerFunc   func,
                    void           *data,
                    GError        **error)
{
  struct sockaddr_in srvaddr, cliaddr;
  socklen_t srvaddr_len = sizeof(srvaddr);
  socklen_t cliaddr_len = sizeof(cliaddr);
  int sockfd, connfd, binded, listening;
  unsigned int prev_max_idle_time;
  GThreadPool *thread_pool;
  TcpServerThreadArgs *thread_args;

#ifdef G_OS_WIN32
  /**
   * Inicializar Winsock
   * https://learn.microsoft.com/es-es/windows/win32/winsock/initializing-winsock
   */
  WSADATA wsa_data;
  int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
  if (result != 0) {
    printf("WSAStartup error: %d\n", result);
    return;
  }
#endif

  /* Asignar IP y puerto */
  memset(&srvaddr, 0, srvaddr_len);
  srvaddr.sin_family = AF_INET;
  srvaddr.sin_addr.s_addr = htonl(server->addr);
  srvaddr.sin_port = htons(server->port);

  /* Crear socket */
  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  return_set_error_if(sockfd == -1, error, TCP_SERVER_SOCK_ERROR);
  printf("Socket creado correctamente...\n");

  /* Enlazar socket creado a IP/puerto */
  binded = bind(sockfd, (struct sockaddr*)&srvaddr, srvaddr_len);
  return_set_error_if(binded == -1, error, TCP_SERVER_SOCK_BIND_ERROR);
  printf("Socket enlazado correctamente...\n");

  /* Escuchar puerto */
  listening = listen(sockfd, server->max_conn);
  return_set_error_if(listening == -1, error, TCP_SERVER_SOCK_LISTEN_ERROR);
  printf("Servidor escuchando puerto %d...\n", server->port);

  /* Inicializar thread pool */
  thread_args = g_new0(TcpServerThreadArgs, 1);
  thread_args->func = func;
  thread_args->data = data;
  thread_pool = g_thread_pool_new(run_server_thread,
                                  thread_args,
                                  server->max_threads,
                                  server->exclusive,
                                  error);
  if (*error != NULL) {
    return;
  }

  prev_max_idle_time = g_thread_pool_get_max_idle_time();
  g_thread_pool_set_max_idle_time(MAX_IDLE_TIME);

  do {
    /* Aceptar conexión de cliente */
    connfd = accept(sockfd, (struct sockaddr*)&cliaddr, &cliaddr_len);
    return_set_error_if(connfd == -1, error, TCP_SERVER_SOCK_ACCEPT_ERROR);
    printf("Conexión aceptada...\n");

    /* Ejecutar función del servidor en otro hilo */
    g_thread_pool_push(thread_pool, GINT_TO_POINTER(connfd), error);
    if (*error != NULL) {
      break;
    }

  } while (TRUE);

  g_thread_pool_free(thread_pool, FALSE, TRUE);
  g_thread_pool_set_max_idle_time(prev_max_idle_time);
  g_free(thread_args);

#ifdef G_OS_UNIX
  close(sockfd);
#endif

#ifdef G_OS_WIN32
  closesocket(sockfd);
  WSACleanup();
#endif

  printf("Servidor desconectado.\n");
}

void tcp_server_free(TcpServer *server)
{
  free(server);
}
