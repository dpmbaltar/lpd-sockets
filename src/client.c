#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#define SRV_ADDR "127.0.0.1"
#define SRV_PORT 24000
#define MAX_BUFF 255
#define CMD_EXIT "salir"

/**
 * Solicita mensajes al usuario y los envía al servidor principal.
 *
 * @param data resultado de socket()
 * @param user_data sin utilizar
 */
void client(gpointer data, gpointer user_data)
{
  int sockfd = GPOINTER_TO_INT(data);
  g_return_if_fail(sockfd != -1);

  bool exit = FALSE;
  int send_num = 0;
  int buff_len = 0;
  char buff[MAX_BUFF];

  do {
    /* Leer mensaje del usuario */
    memset(buff, 0, sizeof(buff));
    printf("Escribir mensaje: ");
    while (buff_len < sizeof(buff) && (buff[buff_len++] = getchar()) != '\n');
    buff_len = 0;

    /* Salir si se escribe el mensaje CMD_EXIT */
    if ((strncmp(buff, CMD_EXIT, strlen(CMD_EXIT))) == 0)
      exit = TRUE;

    /* Enviar mensaje al servidor */
    send(sockfd, buff, sizeof(buff), 0);
    printf("Mensaje enviado.\n");

    /* Leer respuesta del servidor */
    memset(buff, 0, sizeof(buff));
    recv(sockfd, buff, sizeof(buff), 0);
    printf("Respuesta del servidor: %s\n", buff);

    send_num++;
  } while (!exit);

  close(sockfd);
  printf("Desconectado del servidor.");
  printf("Mensajes enviados: %d\n", send_num);
}

/**
 * Ejecuta un cliente TCP básico:
 * 1. socket()
 * 2. connect()
 * 3. send()/recv()
 * 4. close()
 *
 * @param argc cantidad de parámetros
 * @param argv parámetros
 * @return 0 si la ejecución es exitosa, 1 si falla
 */
int main(int argc, char **argv)
{
  GError    *err = NULL;
  TcpClient *cli = tcp_client_new(inet_addr(SRV_ADDR), SRV_PORT, client, NULL);

  tcp_client_run(cli, &err);

  if (err != NULL) {
    fprintf(stderr, "%s", err->message);
    return EXIT_FAILURE;
  }

  tcp_client_free(cli);

  return EXIT_SUCCESS;
}
