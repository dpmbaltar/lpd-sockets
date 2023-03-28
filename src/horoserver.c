#include <glib.h>
#include <stdbool.h>
#include <stdint.h>
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
#include "types.h"
#include "util.h"

/* Nombre del servidor */
#define SRV_NAME     "Servidor del horóscopo"
/* Descripción del programa */
#define SRV_INFO     "- Servidor del horóscopo"
/* Dirección del servidor por defecto */
#define SRV_ADDR     INADDR_ANY
/* Puerto del servidor por defecto */
#define SRV_PORT     24002
/* Cantidad máxima para envío de bytes */
#define SRV_SEND_MAX 256
/* Cantidad máxima para recepción de bytes */
#define SRV_RECV_MAX 4
/* TTL para datos del horóscopo (segundos) */
#define SRV_DATA_TTL 86400

/* Mímino de días para el horóscopo, a partir de la fecha actual */
#define H_MIN_DAYS 0
/* Máximo de días para el horóscopo, a partir de la fecha actual */
#define H_MAX_DAYS 7

/* Dirección del servidor */
static uint32_t addr = SRV_ADDR;

/* Puerto del servidor */
static uint16_t port = SRV_PORT;

/* Opciones de línea de comandos */
static GOptionEntry options[] =
{
  { "addr", 'a', 0, G_OPTION_ARG_INT, &addr, "Direccion (0=INADDR_ANY)", "A" },
  { "port", 'p', 0, G_OPTION_ARG_INT, &port, "Puerto (>1024)", "P" },
  { NULL }
};

/* Rangos de fechas para los signos */
static const char *astro_date_ranges[N_SIGNS][2][2] =
{
  { { 3, 21 }, { 4, 19 } },
  { { 4, 20 }, { 5, 20 } },
  { { 5, 21 }, { 6, 21 } },
  { { 6, 22 }, { 7, 22 } },
  { { 7, 23 }, { 8, 22 } },
  { { 8, 23 }, { 9, 22 } },
  { { 9, 23 }, { 10, 22 } },
  { { 10, 23 }, { 11, 22 } },
  { { 11, 23 }, { 12, 21 } },
  { { 12, 22 }, { 1, 19 } },
  { { 1, 20 }, { 2, 18 } },
  { { 2, 19 }, { 3, 20 } }
};

/* Caché de datos del horóscopo */
static AstroInfo astro_data[H_MAX_DAYS][N_SIGNS] = { {0} };

/* Marcas de tiempo de la caché */
static time_t astro_cache[H_MAX_DAYS] = { 0 };

static void get_horoscope(AstroInfo *astro_info, char day, unsigned char sign)
{
  static GMutex mutex;
  struct timeval ts;

  g_return_if_fail(astro_info != NULL);
  g_return_if_fail(day >= H_MIN_DAYS && day <= H_MAX_DAYS);
  g_return_if_fail(sign < N_SIGNS);

  gettimeofday(&ts, NULL);
  g_mutex_lock(&mutex);

  if ((ts.tv_sec - astro_cache[day]) > SRV_DATA_TTL) {
    AstroInfo *astro_data_day = &astro_data[day][sign];

    /* Generar datos aleatorios */
    astro_data_day->sign = sign;
    astro_data_day->sign_compat = g_rand_int_range(rand, 0, N_SIGNS);
    memcpy(astro_data_day->date_range, astro_date_ranges[sign], 4);
    /* TODO: mood_len y mood */

    astro_cache[day] = ts.tv_sec;
  }

  memcpy(astro_info, &astro_data[day][sign], sizeof(astro_data[day][sign]));
  g_mutex_unlock(&mutex);
}

static int get_client_arg(const char *str, unsigned int len)
{
  int day = -1;
  Date date = DATE_INIT;
  GDateTime *dt_arg;
  GDateTime *dt_now;
  GDateTime *dt_today;
  GTimeSpan dt_diff = 0;

  g_return_val_if_fail(str != NULL, day);
  g_return_val_if_fail(len != 0 || len < sizeof(Date), day);

  memcpy(&date, str, sizeof(Date));
  dt_arg = g_date_time_new_local(date.year, date.month, date.day, 0, 0, 0);

  if (dt_arg != NULL) {
    dt_now = g_date_time_new_now_local();
    dt_today = g_date_time_new_local(g_date_time_get_year(dt_now),
                                     g_date_time_get_month(dt_now),
                                     g_date_time_get_day_of_month(dt_now),
                                     0, 0, 0);
    dt_diff = g_date_time_difference(dt_arg, dt_today);
    day = dt_diff / G_TIME_SPAN_DAY;

    g_date_time_unref(dt_arg);
    g_date_time_unref(dt_now);
    g_date_time_unref(dt_today);
  }

  return day;
}

static void serve_weather(gpointer data, gpointer user_data)
{
  int connfd = GPOINTER_TO_INT(data);
  g_return_if_fail(connfd != -1);

  int weather_day = -1;
  char recv_buff[SRV_RECV_MAX];
  char send_buff[SRV_SEND_MAX];

  memset(recv_buff, 0, sizeof(recv_buff));
  memset(send_buff, 0, sizeof(send_buff));

  /* Leer solicitud del cliente */
  recv(connfd, recv_buff, sizeof(recv_buff), 0);
  printf("Bytes recibidos:\n");
  printx_bytes(recv_buff, (int)sizeof(recv_buff));

  /* Analizar datos recibidos */
  weather_day = get_client_arg(recv_buff, (int)sizeof(recv_buff));

  /* Preparar datos para el envío */
  if (weather_day != -1) {
    WeatherInfo weather = WEATHER_INFO_INIT;
    get_weather(&weather, weather_day);
    memcpy(send_buff, &weather, sizeof(weather));
    printf("Mensaje enviado: {%s, %d, %.1f}\n", weather.date, weather.cond,
           weather.temp);
  }

  /* Enviar datos al cliente */
  send(connfd, send_buff, sizeof(send_buff), 0);
  printf("Bytes enviados:\n");
  printx_bytes(send_buff, (int)sizeof(send_buff));

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

  printf("Iniciando %s...\n", SRV_NAME);
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
