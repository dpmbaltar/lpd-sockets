/**
 * @file client.c
 * @author Diego Pablo Matias Baltar (diego.baltar@est.fi.uncoma.edu.ar)
 * @brief Cliente para el servidor principal
 * @version 0.1
 * @date 2023-04-04
 *
 * Programa del cliente para interactuar con el servidor principal. También se
 * puede utilizar para interactuar de manera independiente con el servidor del
 * clima y del horóscopo, indicando el puerto por el parámetro -p al iniciarse.
 * En el caso del servidor del clima, se puede omitir ingresar el signo, ya que
 * se ignora dicho parámetro.
 *
 * A continuación se detallan las opciones por parámetros que toma el cliente,
 * que también puede verse al ejecutar el programa con el parámetro -h o --help:
 *
 * @code{.unparsed}
 * ./client --help
 * @endcode
 *
 * Resultado:
 *
 * @code{.unparsed}
 * Uso:
 *   client [OPTION?] - Cliente TCP
 *
 * Opciones de ayuda:
 *   -h, --help       Muestra ayuda de opciones
 *
 * Opciones de aplicación:
 *   -H, --host=H     Host del servidor (127.0.0.1 por defecto)
 *   -p, --port=P     Puerto del servidor (24000 por defecto)
 * @endcode
 */
#include <glib.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>
#include <stdbool.h>
#include <stdint.h>
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
#include "types.h"
#include "util.h"

/** Host del servidor por defecto */
#define SRV_HOST "127.0.0.1"
/** Puerto del servidor por defecto */
#define SRV_PORT 24000
/** Cantidad máxima de datos a enviar al servidor */
#define BUF_SEND_MAX 255
/** Cantidad máxima de datos a leer del servidor */
#define BUF_RECV_MAX 1023
/** Cantidad máxima de datos a leer del usuario */
#define USR_READ_MAX 80

/* Host del servidor */
static char *host = SRV_HOST;

/* Puerto del servidor */
static uint16_t port = SRV_PORT;

/* Opciones de línea de comandos */
static GOptionEntry options[] =
{
  { "host", 'H', 0, G_OPTION_ARG_STRING, &host, "Host del servidor (127.0.0.1 por defecto)", "H" },
  { "port", 'p', 0, G_OPTION_ARG_INT, &port, "Puerto del servidor (24000 por defecto)", "P" },
  { NULL }
};

/* Cliente TCP */
static TcpClient *client = NULL;

/* Instancia de JsonParser */
static JsonParser *json_parser = NULL;

static void print_json(const char *data)
{
  JsonGenerator *generator;
  JsonNode *root = parse_json(data, strlen(data), json_parser);
  char *json = NULL;

  if (root != NULL) {
    generator = json_generator_new();
    json_generator_set_root(generator, root);
    json_generator_set_pretty(generator, TRUE);
    json = json_generator_to_data(generator, NULL);

    printf("%s\n", json);

    json_node_free(root);
    g_object_unref(generator);
    g_free(json);
  }
}

static void *client_func(int sockfd, gpointer data)
{
  g_return_val_if_fail(sockfd != -1, NULL);
  g_return_val_if_fail(data != NULL, NULL);

  char recv_buf[BUF_RECV_MAX+1];
  char send_buf[BUF_SEND_MAX+1];
  int  send_len = MIN(BUF_SEND_MAX, strlen(data));

  memset(recv_buf, 0, sizeof(recv_buf));
  memset(send_buf, 0, sizeof(send_buf));
  memcpy(send_buf, data, send_len);

  /* Enviar mensaje al servidor */
  send(sockfd, send_buf, send_len, 0);
  printf("Mensaje enviado:\n%s\n", send_buf);

  /* Recibir respuesta del servidor */
  recv(sockfd, recv_buf, BUF_RECV_MAX, 0);
  printf("Mensaje recibido:\n%s\n", recv_buf);

  /* Mostrar datos al usuario */
  printf("Datos del clima y el horóscopo:\n");
  print_json(recv_buf);

  return NULL;
}

static void read_line(char *buffer, int buffer_limit)
{
  int i = 0;
  while (i < buffer_limit-1 && (buffer[i++] = getchar()) != '\n');
  buffer[strcspn(buffer, "\n")] = 0;
}

static int main_loop()
{
  g_return_val_if_fail(client != NULL, EXIT_FAILURE);

  GError *error = NULL;
  GThread *client_thread;

  bool exit = FALSE;
  char send_buf[BUF_SEND_MAX+1];
  char date_arg[USR_READ_MAX];
  char sign_arg[USR_READ_MAX];

  do {
    memset(send_buf, 0, sizeof(send_buf));
    memset(date_arg, 0, sizeof(date_arg));
    memset(sign_arg, 0, sizeof(sign_arg));

    /* Leer solicitud del usuario */
    printf("Escribir fecha:\n");
    read_line(date_arg, sizeof(date_arg));
    printf("Escribir signo:\n");
    read_line(sign_arg, sizeof(sign_arg));
    sprintf(send_buf, "{\"date\":\"%s\",\"sign\":\"%s\"}", date_arg, sign_arg);

    /* Ejecutar función del cliente en un nuevo hilo */
    client_thread = tcp_client_run(client, client_func, send_buf, &error);

    if (error != NULL) {
      fprintf(stderr, "%s\n", error->message);
      g_error_free(error);
      return EXIT_FAILURE;
    }

    if (client_thread != NULL) {
      g_thread_join(client_thread);
    }

    printf("Presionar Entrar para otra consulta / N para salir\n");
    switch (getchar()) {
      case 'N':
      case 'n':
        exit = TRUE;
      break;
    }

  } while (!exit);

  return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
  GError         *error = NULL;
  GOptionContext *context;
  int             retval = EXIT_SUCCESS;

  context = g_option_context_new("- Cliente TCP");
  g_option_context_add_main_entries(context, options, NULL);
  g_option_context_parse(context, &argc, &argv, &error);
  g_option_context_free(context);

  if (error != NULL) {
    fprintf(stderr, "%s\n", error->message);
    g_error_free(error);
    return EXIT_FAILURE;
  }

  json_parser = json_parser_new();
  client = tcp_client_new(host, port);
  retval = main_loop();
  tcp_client_free(client);

  return retval;
}
