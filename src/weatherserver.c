#include <glib.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>
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
#define SRV_NAME     "Servidor del clima"
/* Descripción del programa */
#define SRV_INFO     "- Servidor del clima"
/* Dirección del servidor por defecto */
#define SRV_ADDR     INADDR_ANY
/* Puerto del servidor por defecto */
#define SRV_PORT     24001
/* Cantidad máxima para envío de bytes */
#define SRV_SEND_MAX 16
/* Cantidad máxima para recepción de bytes */
#define SRV_RECV_MAX 255
/* TTL para datos del clima (segundos) */
#define SRV_DATA_TTL 3600

/* Mímino de días para el clima, a partir de la fecha actual */
#define W_MIN_DAYS    0
/* Máximo de días para el clima, a partir de la fecha actual */
#define W_MAX_DAYS    7
/* Temperatura mínima para generar datos del clima */
#define W_MIN_TEMP    -25.0F
/* Temperatura máxima para generar datos del clima */
#define W_MAX_TEMP    50.0F
/* Formato de fecha para generar datos del clima */
#define W_DATE_FORMAT "%Y-%m-%d"

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

/* Instancia de JsonParser */
static JsonParser *json_parser = NULL;

/* Caché de datos del clima */
static WeatherInfo weather_data[W_MAX_DAYS] = { 0 };

/* Marcas de tiempo de la caché */
static time_t weather_cache[W_MAX_DAYS] = { 0 };

static void create_weather(WeatherInfo *weather_info, int day)
{
  g_return_if_fail(weather_info != NULL);
  g_return_if_fail(day >= W_MIN_DAYS && day <= W_MAX_DAYS);

  GDateTime *datetime_now = g_date_time_new_now_local();
  GDateTime *datetime = g_date_time_add_days(datetime_now, day);
  GRand *rand = g_rand_new();
  char *date = g_date_time_format(datetime, W_DATE_FORMAT);

  memcpy(&(weather_data[day].date), date, sizeof(((WeatherInfo*)0)->date));
  weather_data[day].cond = g_rand_int_range(rand, 0, N_CONDITIONS);
  weather_data[day].temp = g_rand_double_range(rand, W_MIN_TEMP, W_MAX_TEMP);

  g_date_time_unref(datetime_now);
  g_date_time_unref(datetime);
  g_rand_free(rand);
  g_free(date);
}

static void get_weather(WeatherInfo *weather_info, int day)
{
  static GMutex mutex;
  struct timeval time;

  g_return_if_fail(weather_info != NULL);
  g_return_if_fail(day >= W_MIN_DAYS && day <= W_MAX_DAYS);

  g_mutex_lock(&mutex);
  gettimeofday(&time, NULL);

  if ((time.tv_sec - weather_cache[day]) > SRV_DATA_TTL) {
    create_weather(weather_info, day);
  }

  memcpy(weather_info, &weather_data[day], sizeof(weather_data[day]));
  g_mutex_unlock(&mutex);
}

static JsonNode *parse_json(const char *data, int length)
{
  GError *error = NULL;

  g_return_val_if_fail(data != NULL, NULL);

  json_parser_load_from_data(json_parser, data, length, &error);

  if (error != NULL) {
    g_print("Error al obtener JSON `%s`: %s\n", data, error->message);
    g_error_free(error);
    return NULL;
  }

  return json_parser_steal_root(json_parser);
}

static GDate *parse_date(const char *data)
{
  GDate *date;

  g_return_val_if_fail(data != NULL, NULL);

  date = g_date_new();
  g_date_set_parse(date, data);

  if (!g_date_valid(date)) {
    g_date_free(date);
    date = NULL;
  }

  return date;
}

static int get_weather_day(const char *data, int length)
{
  struct timeval time;
  int day = -1;
  const char *date_str = NULL;
  GDate *date = NULL;
  GDate *today = NULL;
  JsonNode *json_node = NULL;
  JsonObject *json_object = NULL;

  g_return_val_if_fail(data != NULL, -1);
  g_return_val_if_fail(length > 0, -1);

  json_node = parse_json(data, length);
  json_object = json_node_get_object(json_node);
  date_str = json_object_get_string_member(json_object, "date");
  date = parse_date(date_str);

  if (date != NULL) {
    today = g_date_new();
    gettimeofday(&time, NULL);
    g_date_set_time_t(today, time.tv_sec);
    day = g_date_days_between(today, date);
    printf("Date: %s\n", date_str);
    printf("Today: %d-%d-%d\n", g_date_get_year(today),
                                g_date_get_month(today),
                                g_date_get_day(today));
    printf("Days between: %d\n", day);

    g_date_free(date);
    g_date_free(today);
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
  weather_day = get_weather_day(recv_buff, MIN(SRV_RECV_MAX, strlen(recv_buff)));

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

  json_parser = json_parser_new();
  printf("Iniciando %s...\n", SRV_NAME);
  server = tcp_server_new(addr, port, serve_weather, NULL);
  tcp_server_run(server, &error);

  if (error != NULL) {
    fprintf(stderr, "%s", error->message);
    return EXIT_FAILURE;
  }

  tcp_server_free(server);
  g_option_context_free(context);
  g_object_unref(json_parser);

  return EXIT_SUCCESS;
}
