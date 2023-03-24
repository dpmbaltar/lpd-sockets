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

/* Dirección por defecto */
#define SRV_ADDR           INADDR_ANY
/* Puerto por defecto */
#define SRV_PORT           24001
#define SRV_SEND_BUFF_SIZE 160
#define SRV_RECV_BUFF_SIZE 20
/* TTL del clima */
#define SRV_WEATHER_TTL    3600

/**
 * Comando para enviar info del clima para un día. Por ejemplo: info 3 devuelve
 * información para dentro de 3 días a partir de hoy.
 */
#define SRV_CMD_INFO       "info"

/* Dirección del servidor */
static in_addr_t addr = SRV_ADDR;

/* Puerto del servidor */
static in_port_t port = SRV_PORT;

/* Opciones de línea de comandos */
static GOptionEntry options[] =
{
  { "addr", 'a', 0, G_OPTION_ARG_INT, &addr, "Dirección (0 = INADDR_ANY)", "A" },
  { "port", 'p', 0, G_OPTION_ARG_INT, &port, "Puerto (> 1024)", "P" },
  { NULL }
};

/* Cache del clima */
static WeatherInfo weather_cache[WEATHER_MAX_DAYS] = { 0 };

/* Cache timestamps */
static time_t weather_cache_ts[WEATHER_MAX_DAYS] = { 0 };

static void get_weather(WeatherInfo *weather_info, int day)
{
  g_return_if_fail(weather_info != NULL);
  g_return_if_fail(day >= WEATHER_MIN_DAYS && day <= WEATHER_MAX_DAYS);

  struct timeval ts;
  gettimeofday(&ts, NULL);

  if ((ts.tv_sec - weather_cache_ts[day]) > SRV_WEATHER_TTL) {
    weather_get_info(&weather_cache[day], day);
    weather_cache_ts[day] = ts.tv_sec;
  }

  memcpy(weather_info, &weather_cache[day], sizeof(weather_cache[day]));
}

static void serve_weather(gpointer data, gpointer user_data)
{
  int connfd = GPOINTER_TO_INT(data);
  g_return_if_fail(connfd != -1);

  int weather_day = 0;
  char recv_buff[SRV_RECV_BUFF_SIZE];
  char send_buff[SRV_SEND_BUFF_SIZE];

  /* Leer instrucción del cliente */
  memset(recv_buff, 0, sizeof(recv_buff));
  recv(connfd, recv_buff, sizeof(recv_buff), 0);
  printf("Mensaje recibido: %s", recv_buff);

  /* Analizar mensaje recibido del cliente */
  if ((strncmp(recv_buff, SRV_CMD_INFO, strlen(SRV_CMD_INFO))) == 0) {
    //TODO: analizar comandos enviados por el cliente
    printf("Tokens:\n");
    GStrv tokens = g_str_tokenize_and_fold(recv_buff, NULL, NULL);
    if (tokens != NULL) {
      int tokens_len = g_strv_length(tokens);
      for (int i = 0; i < tokens_len; i++)
        printf("%s\n", tokens[i]);
      g_strfreev(tokens);
    }
  }

  /* Preparar datos del clima */
  WeatherInfo weather_info = WEATHER_INFO_INIT;
  get_weather(&weather_info, weather_day);
  memset(send_buff, 0, sizeof(send_buff));
  memcpy(send_buff, &weather_info, sizeof(weather_info));

  /* Enviar datos del clima */
  send(connfd, send_buff, sizeof(send_buff), 0);
  printf("Datos enviados: {%s, %d, %.1f}\n",
         weather_info.date,
         weather_info.cond,
         weather_info.temp);

  close(connfd);
  printf("Desconectado del cliente.\n");
}

int main(int argc, char **argv)
{
  GError         *error = NULL;
  GOptionContext *context;
  TcpServer      *server;

  context = g_option_context_new ("- Servidor del clima");
  g_option_context_add_main_entries(context, options, NULL);

  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    fprintf(stderr, "%s\n", error->message);
    return EXIT_FAILURE;
  }

  if (port <= 1024) {
    fprintf(stderr, "El puerto debe ser > 1024\n");
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
