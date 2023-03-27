#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef G_OS_UNIX
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#ifdef G_OS_WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "tcpclient.h"

G_DEFINE_QUARK(tcp-client-error, tcp_client_error)

typedef struct _TcpClientThreadArgs TcpClientThreadArgs;

struct _TcpClient
{
  const char *host;
  uint16_t    port;
};

struct _TcpClientThreadArgs
{
  TcpClientFunc func;
  gpointer      data;
  int           sock;
};

static const char *error_messages[] = {
  [TCP_CLIENT_SOCK_ERROR]         = "Error al crear socket",
  [TCP_CLIENT_SOCK_CONNECT_ERROR] = "Error al abrir conexión con el socket",
};

/* Macro para manejar errores */
#define return_set_error_if(cond, error, code) \
  if (cond) {\
    g_set_error(error, TCP_CLIENT_ERROR, code, error_messages[code]);\
    return NULL;\
  }

TcpClient *tcp_client_new(const char *host, uint16_t port)
{
  TcpClient *client = (TcpClient*)malloc(sizeof(TcpClient));
  client->host = host;
  client->port = port;

  return client;
}

static gpointer run_client_thread(gpointer data)
{
  gpointer retval = NULL;
  TcpClientThreadArgs *args = (TcpClientThreadArgs*)data;

  g_return_val_if_fail(args != NULL, retval);

  retval = args->func(args->sock, args->data);
#ifdef G_OS_UNIX
  close(args->sock);
#endif
#ifdef G_OS_WIN32
  closesocket(args->sock);
  WSACleanup();
#endif
  g_free(args);
  printf("Desconectado del servidor.\n");

  return retval;
}

GThread *tcp_client_run(TcpClient      *client,
                        TcpClientFunc   func,
                        gpointer        func_data,
                        GError        **error)
{
  g_return_val_if_fail(client != NULL, NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);

  int sockfd, connected;
  struct sockaddr_in servaddr;
  size_t servaddr_len = sizeof(servaddr);
  TcpClientThreadArgs *thread_args;

#ifdef G_OS_WIN32
  /**
   * Inicializar Winsock
   * https://learn.microsoft.com/es-es/windows/win32/winsock/initializing-winsock
   */
  WSADATA wsa_data;
  int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
  if (result != 0) {
    printf("WSAStartup failed: %d\n", result);
    return NULL;
  }
#endif

  /* Asignar IP y puerto */
  memset(&servaddr, 0, servaddr_len);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(client->host);
  servaddr.sin_port = htons(client->port);

  /* Crear socket */
  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  return_set_error_if(sockfd == -1, error, TCP_CLIENT_SOCK_ERROR);
  printf("Socket creado correctamente...\n");

  /* Conectar socket del cliente al socket del servidor */
  connected = connect(sockfd, (struct sockaddr*)&servaddr, servaddr_len);
  return_set_error_if(connected == -1, error, TCP_CLIENT_SOCK_CONNECT_ERROR);
  printf("Conectado al servidor...\n");

  /* Preparar parámetros para función del cliente */
  thread_args = g_new0(TcpClientThreadArgs, 1);
  thread_args->func = func;
  thread_args->data = func_data;
  thread_args->sock = sockfd;

  /* Ejecutar función del cliente en un nuevo thread, si es posible */
  return g_thread_try_new(NULL, run_client_thread, thread_args, error);
}

void tcp_client_free(TcpClient *client)
{
  free(client);
}
