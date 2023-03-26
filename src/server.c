#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef G_OS_UNIX
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#ifdef G_OS_WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "tcpserver.h"

/* Descripción del servidor */
#define SRV_INFO     "- Servidor principal"
/* Dirección del servidor por defecto */
#define SRV_ADDR     INADDR_ANY
/* Puerto del servidor por defecto */
#define SRV_PORT     24000
/* Cantidad máxima para envío de bytes */
#define SRV_SEND_MAX 80
/* Cantidad máxima para recepción de bytes */
#define SRV_RECV_MAX 20

#define MAX_BUFF 255
#define CMD_EXIT "salir"

/* Dirección del servidor */
static in_addr_t addr = SRV_ADDR;

/* Puerto del servidor */
static in_port_t port = SRV_PORT;

/* Opciones de línea de comandos */
static GOptionEntry options[] =
{
  { "addr", 'a', 0, G_OPTION_ARG_INT, &addr, "Direccion (0=INADDR_ANY)", "A" },
  { "port", 'p', 0, G_OPTION_ARG_INT, &port, "Puerto (>1024)", "P" },
  { NULL }
};

/**
 * @brief Servidor eco (lectura/escritura).
 *
 * Hace eco de los mensajes enviados por un cliente. Se cierra si recibe el
 * mensaje de salida definido en CMD_EXIT, por ejemplo, "salir".
 *
 * @param data (int) resultado de accept()
 * @param user_data sin utilizar por ahora
 */
void serve_echo(gpointer data, gpointer user_data)
{
  int connfd = GPOINTER_TO_INT(data);
  g_return_if_fail(connfd != -1);

  int n;
  char buff[MAX_BUFF];

  for (n = 0;;n++) {
    // Leer mensaje del cliente y copiarlo al búfer
    memset(buff, 0, MAX_BUFF);
    recv(connfd, buff, sizeof(buff), 0);
    printf("Mensaje recibido: %s", buff);

    // Enviar eco
    send(connfd, buff, sizeof(buff), 0);
    printf("Mensaje enviado: %s", buff);

    // Salir del bucle si el cliente sale
    if ((strncmp(buff, CMD_EXIT, strlen(CMD_EXIT))) == 0) {
        printf("El cliente ha salido.\n");
        break;
    }
  }

  close(connfd);
  printf("Mensajes recibidos: %d\n", n);
}

/**
 * @brief Servidor TCP básico.
 *
 * Ejecuta un servidor TCP básico:
 * 1. socket()
 * 2. bind()
 * 3. listen()
 * 4. accept()
 * 5. read()/write()
 * 6. close()
 *
 * @param argc
 * @param argv
 * @return 0 si la ejecución es exitosa, 1 si falla
 */
int main(int argc, char **argv)
{
  GError         *error = NULL;
  GOptionContext *context;
  TcpServer      *server = tcp_server_new(SRV_ADDR, SRV_PORT, serve_echo, NULL);

  context = g_option_context_new(SRV_INFO);
  g_option_context_add_main_entries(context, options, NULL);

  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    fprintf(stderr, "%s\n", error->message);
    return EXIT_FAILURE;
  }

  if (port <= 1024) {
    fprintf(stderr, "El puerto debe ser mayor a 1024\n");
    return EXIT_FAILURE;
  }

  tcp_server_run(server, &error);

  if (error != NULL) {
    fprintf(stderr, "%s", error->message);
    return EXIT_FAILURE;
  }

  tcp_server_free(server);
  g_option_context_free(context);

  return EXIT_SUCCESS;
}
