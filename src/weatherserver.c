#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
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
#include "weather.h"

/* Descripción del servidor */
#define SRV_INFO     "- Servidor del clima"
/* Dirección del servidor por defecto */
#define SRV_ADDR     INADDR_ANY
/* Puerto del servidor por defecto */
#define SRV_PORT     24001
/* Cantidad máxima para envío de bytes */
#define SRV_SEND_MAX 20
/* Cantidad máxima para recepción de bytes */
#define SRV_RECV_MAX 20
/* TTL para datos del clima (segundos) */
#define SRV_DATA_TTL 3600

/* Token de solicitud de datos. Ejemplo: get 0 = datos del día de hoy */
#define TOK_GET      "get"
/* Token de respuesta de error de solicitud */
#define TOK_ERROR    "error"

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

/* Caché de datos del clima */
static WeatherInfo weather_data[WEATHER_MAX_DAYS] = { 0 };

/* Marcas de tiempo de la caché */
static time_t weather_cache[WEATHER_MAX_DAYS] = { 0 };

static void get_weather(WeatherInfo *weather_info, int day)
{
  static GMutex mutex;
  struct timeval ts;

  g_return_if_fail(weather_info != NULL);
  g_return_if_fail(day >= WEATHER_MIN_DAYS && day <= WEATHER_MAX_DAYS);

  gettimeofday(&ts, NULL);
  g_mutex_lock(&mutex);

  if ((ts.tv_sec - weather_cache[day]) > SRV_DATA_TTL) {
    weather_get_info(&weather_data[day], day);
    weather_cache[day] = ts.tv_sec;
  }

  memcpy(weather_info, &weather_data[day], sizeof(weather_data[day]));
  g_mutex_unlock(&mutex);
}

static char **get_client_args(const char *str, unsigned int len)
{
  g_return_val_if_fail(str != NULL, NULL);
  g_return_val_if_fail(len != 0, NULL);

  GStrv tokens = NULL;
  char tokens_str[len+1];

  /* Forzar cadena con byte nulo (0x00) al final */
  tokens_str[len] = 0;
  memcpy(tokens_str, str, len);
  tokens = g_str_tokenize_and_fold(tokens_str, NULL, NULL);

  return tokens;
}

static void serve_weather(gpointer data, gpointer user_data)
{
  int connfd = GPOINTER_TO_INT(data);
  g_return_if_fail(connfd != -1);

  int weather_day = -1;
  char **client_args;
  char recv_buff[SRV_RECV_MAX];
  char send_buff[SRV_SEND_MAX];

  memset(recv_buff, 0, sizeof(recv_buff));
  memset(send_buff, 0, sizeof(send_buff));

  /* Leer solicitud del cliente */
  recv(connfd, recv_buff, sizeof(recv_buff), 0);
  printf("Mensaje recibido: %s", recv_buff);
  printf("Bytes recibidos:\n");
  for (int i = 0; i < (int)sizeof(recv_buff); i++)
    printf("%02x ", (unsigned char)recv_buff[i]);
  printf("\n");

  /* Analizar datos recibidos */
  client_args = get_client_args(recv_buff, sizeof(recv_buff));
  if (client_args != NULL && g_strv_length(client_args) >= 2) {
    if (strcmp(client_args[0], TOK_GET) == 0) {
      weather_day = atoi(client_args[1]);
      if (weather_day < WEATHER_MIN_DAYS || weather_day > WEATHER_MAX_DAYS) {
        weather_day = -1;
      }
    }
  }
  g_strfreev(client_args);

  /* Preparar datos para el envío */
  if (weather_day != -1) {
    WeatherInfo weather = WEATHER_INFO_INIT;
    get_weather(&weather, weather_day);
    memcpy(send_buff, &weather, sizeof(weather));
    printf("Mensaje enviado: {%s, %d, %.1f}\n", weather.date, weather.cond,
           weather.temp);
  } else {
    memcpy(send_buff, TOK_ERROR, sizeof(TOK_ERROR));
    printf("Mensaje enviado: %s\n", send_buff);
  }

  /* Enviar datos al cliente */
  send(connfd, send_buff, sizeof(send_buff), 0);
  printf("Bytes enviados:\n");
  for (int i = 0; i < (int)sizeof(send_buff); i++)
    printf("%02x ", (unsigned char)send_buff[i]);
  printf("\n");

  /* Cerrar conexión */
  close(connfd);
  printf("Desconectado del cliente.\n");
}

int main(int argc, char **argv)
{
  GError         *error = NULL;
  GOptionContext *context;
  TcpServer      *server;

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

  server = tcp_server_new(addr, port, serve_weather, NULL);
  tcp_server_run(server, &error);

  if (error != NULL) {
    fprintf(stderr, "%s", error->message);
    return EXIT_FAILURE;
  }

  tcp_server_free(server);
  g_option_context_free(context);

  return EXIT_SUCCESS;
}
