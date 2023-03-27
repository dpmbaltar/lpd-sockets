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

struct _TcpClient
{
  const char   *host;
  uint16_t      port;
  TcpClientFunc func;
  gpointer      data;
  int           sock;
};

TcpClient *tcp_client_new(const char *host, uint16_t port)
{
  TcpClient *client = (TcpClient*)malloc(sizeof(TcpClient));
  client->host = host;
  client->port = port;
  client->func = NULL;
  client->data = 0;
  client->sock = -1;

  return client;
}

static gpointer run_client_func(gpointer data)
{
  gpointer retval = NULL;
  TcpClient *client = (TcpClient*)data;
  int sockfd = client->sock;

  g_return_val_if_fail(client != NULL, NULL);
  g_return_val_if_fail(sockfd != -1, NULL);

  retval = client->func(sockfd, client->data);
  client->func = NULL;
  client->data = NULL;
  client->sock = -1;

#ifdef G_OS_UNIX
  close(sockfd);
#endif

#ifdef G_OS_WIN32
  closesocket(sockfd);
  WSACleanup();
#endif

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
  g_return_val_if_fail(sockfd != -1, NULL);
  printf("Socket creado correctamente...\n");

  /* Conectar socket del cliente al socket del servidor */
  connected = connect(sockfd, (struct sockaddr*)&servaddr, servaddr_len);
  g_return_val_if_fail(connected != -1, NULL);
  printf("Conectado al servidor...\n");

  /* Preparar parámetros para función del cliente */
  client->sock = sockfd;
  client->func = func;
  client->data = func_data;

  /* Ejecutar función del cliente en un nuevo thread, si es posible */
  return g_thread_try_new(NULL, run_client_func, client, error);
}

void tcp_client_free(TcpClient *client)
{
  free(client);
}
