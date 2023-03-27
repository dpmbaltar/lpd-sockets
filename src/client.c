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
#include "weather.h"
#include "util.h"

/* Host del servidor por defecto */
#define SRV_HOST "127.0.0.1"
/* Puerto del servidor por defecto */
#define SRV_PORT 24000
/* Cantidad máxima de datos a leer del usuario */
#define MAX_BUFF 20
/* Token para salir del programa */
#define TOK_EXIT "salir"

static void parse_date(const char *str, Date *date)
{
  g_return_if_fail(str != NULL || strlen(str) < 10);
  g_return_if_fail(date != NULL);

  GDate *gdate = g_date_new();
  char date_str[11];
  date_str[10] = 0;
  memcpy(date_str, str, 10);
  g_date_set_parse(gdate, date_str);

  if (g_date_valid(gdate)) {
    date->year = g_date_get_year(gdate);
    date->month = g_date_get_month(gdate);
    date->day = g_date_get_day(gdate);
  }

  g_date_free(gdate);
}

static void *client_func(int sockfd, gpointer data)
{
  g_return_val_if_fail(sockfd != -1, NULL);

  char buff[MAX_BUFF];
  Date date = DATE_INIT;
  WeatherInfo weather = WEATHER_INFO_INIT;

  /* Obtener fecha ingresada por el usuario */
  parse_date((const char*)data, &date);
  memset(buff, 0, sizeof(buff));
  memcpy(buff, &date, sizeof(date));

  /* Enviar mensaje al servidor */
  send(sockfd, buff, sizeof(buff), 0);
  printf("Bytes enviados:\n");
  printx_bytes(buff, (int)sizeof(buff));

  /* Leer respuesta del servidor */
  memset(buff, 0, sizeof(buff));
  recv(sockfd, buff, sizeof(buff), 0);
  printf("Bytes recibidos:\n");
  printx_bytes(buff, (int)sizeof(buff));

  /* Mostrar datos al usuario */
  memcpy(&weather, buff, sizeof(weather));
  printf("Datos del clima recibidos: {%s, %d, %.1f}\n",
          weather.date, weather.cond, weather.temp);

  return NULL;
}

/**
 * Ejecuta un cliente TCP básico:
 * 1. socket()
 * 2. connect()
 * 3. send()/recv()
 * 4. close()
 *
 * @param argc cantidad de argumentos de línea de comandos
 * @param argv argumentos de línea de comandos
 * @return 0 si la ejecución es exitosa, 1 si falla
 */
int main(int argc, char **argv)
{
  GError    *error = NULL;
  GThread   *client_thread;
  TcpClient *client = tcp_client_new(SRV_HOST, SRV_PORT);

  g_return_val_if_fail(client != NULL, EXIT_FAILURE);

  bool exit = FALSE;
  int  buff_len = 0;
  char buff[MAX_BUFF];

  do {
    /* Leer mensaje del usuario */
    printf("Escribir mensaje:\n");
    memset(buff, 0, sizeof(buff));
    while (buff_len < sizeof(buff)-1 && (buff[buff_len++] = getchar()) != '\n');
    buff_len = 0;

    /* Salir si se escribe el mensaje TOK_EXIT */
    if ((strncmp(buff, TOK_EXIT, strlen(TOK_EXIT))) == 0) {
      exit = TRUE;
      continue;
    }

    client_thread = tcp_client_run(client, client_func, buff, &error);

    if (error != NULL) {
      fprintf(stderr, "%s", error->message);
      g_error_free(error);
      return EXIT_FAILURE;
    }

    if (client_thread != NULL) {
      g_thread_join(client_thread);
      g_thread_unref(client_thread);
    }

  } while (!exit);

  tcp_client_free(client);

  return EXIT_SUCCESS;
}
