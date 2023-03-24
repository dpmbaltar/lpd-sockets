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
  in_addr_t addr;
  in_port_t port;
  GFunc     func;
  gpointer  data;
};

TcpClient *tcp_client_new(in_addr_t addr,
                          in_port_t port,
                          GFunc     func,
                          gpointer  data)
{
  g_return_val_if_fail(func != NULL, NULL);

  TcpClient *cli = (TcpClient*)malloc(sizeof(TcpClient));

  cli->addr = addr;
  cli->port = port;
  cli->func = func;
  cli->data = data;

  return cli;
}

void tcp_client_run(TcpClient  *cli,
                    GError    **err)
{
  g_return_if_fail(cli != NULL);
  g_return_if_fail(err == NULL || *err == NULL);

  int sockfd, connfd;
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
    return;
  }
#endif

  /* Asignar IP y puerto */
  memset(&servaddr, 0, servaddr_len);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(cli->addr);
  servaddr.sin_port = htons(cli->port);

  /* Crear socket */
  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  g_return_if_fail(sockfd != -1);
  printf("Socket creado correctamente...\n");

  /* Conectar socket del cliente al socket del servidor */
  connfd = connect(sockfd, (struct sockaddr*)&servaddr, servaddr_len);
  g_return_if_fail(connfd == 0);
  printf("Conectado al servidor...\n");

  /* Ejecutar función del cliente */
  cli->func(GINT_TO_POINTER(sockfd), cli->data);

#ifdef G_OS_UNIX
  close(sockfd);
#endif

#ifdef G_OS_WIN32
  closesocket(sockfd);
  WSACleanup();
#endif

  printf("Desconectado del servidor.\n");
}

void tcp_client_free(TcpClient *client)
{
  free(client);
}
