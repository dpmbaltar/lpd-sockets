/**
 * @file weatherserver.c
 * @author Diego Pablo Matias Baltar (diego.baltar@est.fi.uncoma.edu.ar)
 * @brief Servidor del clima
 * @version 0.1
 * @date 2023-04-04
 *
 * Programa del servidor del clima, que recibe consultas por fechas, válidas a
 * partir de la fecha actual, hasta siete días en adelante. Los datos del clima
 * se guardan en memoria por 1 hora. Si se consulta luego de 1 hora generado los
 * datos, se actualizan.
 *
 * A continuación se detallan las opciones por parámetros que toma el servidor,
 * que también puede verse al ejecutar el programa con el parámetro -h o --help:
 *
 * @code{.unparsed}
 * ./weather_server --help
 * @endcode
 *
 * Resultado:
 *
 * @code{.unparsed}
 * Uso:
 *   weather_server [OPTION?] - Servidor del clima
 *
 * Opciones de ayuda:
 *   -h, --help       Muestra ayuda de opciones
 *
 * Opciones de aplicación:
 *   -a, --addr=A     Direccion A (0 = INADDR_ANY por defecto)
 *   -p, --port=P     Puerto P > 1024 del servidor (24001 por defecto)
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

/** Nombre del servidor */
#define SRV_NAME     "Servidor del clima"
/** Descripción del programa */
#define SRV_INFO     "- Servidor del clima"
/** Dirección del servidor por defecto */
#define SRV_ADDR     INADDR_ANY
/** Puerto del servidor por defecto */
#define SRV_PORT     24001
/** Cantidad máxima para envío de bytes */
#define SRV_SEND_MAX 1024
/** Cantidad máxima para recepción de bytes */
#define SRV_RECV_MAX 1024
/** TTL para datos del clima (segundos) */
#define SRV_DATA_TTL 3600

/** Mímino de días para el clima, a partir de la fecha actual */
#define W_MIN_DAYS    0
/** Máximo de días para el clima, a partir de la fecha actual */
#define W_MAX_DAYS    7
/** Temperatura mínima para generar datos del clima */
#define W_MIN_TEMP    -25.0F
/** Temperatura máxima para generar datos del clima */
#define W_MAX_TEMP    50.0F
/** Formato de fecha para generar datos del clima */
#define W_DATE_FORMAT "%Y-%m-%d"

/* Dirección del servidor */
static uint32_t addr = SRV_ADDR;

/* Puerto del servidor */
static uint16_t port = SRV_PORT;

/* Opciones de línea de comandos */
static GOptionEntry options[] =
{
  { "addr", 'a', 0, G_OPTION_ARG_INT, &addr, "Direccion A (0 = INADDR_ANY por defecto)", "A" },
  { "port", 'p', 0, G_OPTION_ARG_INT, &port, "Puerto P > 1024 del servidor (24001 por defecto)", "P" },
  { NULL }
};

/* Instancia de JsonParser */
static JsonParser *json_parser = NULL;

/* Caché de datos del clima */
static WeatherInfo weather_data[W_MAX_DAYS] = { 0 };

/* Marcas de tiempo de la caché */
static time_t weather_cache[W_MAX_DAYS] = { 0 };

/* Condiciones del tiempo */
static const char *conditions[N_CONDITIONS] =
{
  [W_CLEAR]   = "Despejado",
  [W_CLOUD]   = "Nublado",
  [W_MIST]    = "Neblina",
  [W_RAIN]    = "Lluvia",
  [W_SHOWERS] = "Chubascos",
  [W_SNOW]    = "Nieve"
};

static void create_weather(WeatherInfo *weather_info, int day)
{
  g_return_if_fail(weather_info != NULL);
  g_return_if_fail(day >= W_MIN_DAYS && day <= W_MAX_DAYS);

  GDateTime *datetime_now = g_date_time_new_now_local();
  GDateTime *datetime = g_date_time_add_days(datetime_now, day);
  GRand *rand = g_rand_new();
  char *date = g_date_time_format(datetime, W_DATE_FORMAT);

  memcpy(weather_info->date, date, sizeof(((WeatherInfo*)0)->date));
  weather_info->cond = g_rand_int_range(rand, 0, N_CONDITIONS);
  weather_info->temp = g_rand_double_range(rand, W_MIN_TEMP, W_MAX_TEMP);

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
    create_weather(&weather_data[day], day);
    weather_cache[day] = time.tv_sec;
  }

  memcpy(weather_info, &weather_data[day], sizeof(WeatherInfo));
  g_mutex_unlock(&mutex);
}

static void get_client_args(const char *data, int *arg_day)
{
  g_return_if_fail(data != NULL);
  g_return_if_fail(arg_day != NULL);

  struct timeval time;
  int day = -1;
  const char *date_str = NULL;
  GDate *date = NULL;
  GDate *today = NULL;
  JsonNode *json_node = NULL;
  JsonObject *json_object = NULL;

  json_node = parse_json(data, strlen(data), json_parser);
  json_object = json_node_get_object(json_node);
  date_str = json_object_get_string_member(json_object, "fecha");
  date = parse_date(date_str);

  if (date != NULL) {
    today = g_date_new();
    gettimeofday(&time, NULL);
    g_date_set_time_t(today, time.tv_sec);
    day = g_date_days_between(today, date);

    g_date_free(date);
    g_date_free(today);
  }

  *arg_day = day >= W_MIN_DAYS && day <= W_MAX_DAYS ? day : -1;
}

static char *weather_to_json(WeatherInfo *weather_info)
{
  g_return_val_if_fail(weather_info != NULL, NULL);

  return g_strdup_printf("{\"fecha\":\"%s\",\"temperatura\":%.1f,\"condicion\":\"%s\"}",
                         weather_info->date,
                         weather_info->temp,
                         conditions[(int)weather_info->cond]);
}

static void serve_weather(int connfd, void *data)
{
  g_return_if_fail(connfd != -1);

  int arg_day = -1;
  int send_len = SRV_SEND_MAX;
  char recv_buff[SRV_RECV_MAX+1];
  char send_buff[SRV_SEND_MAX+1];

  memset(recv_buff, 0, sizeof(recv_buff));
  memset(send_buff, 0, sizeof(send_buff));

  /* Leer solicitud del cliente */
  recv(connfd, recv_buff, SRV_RECV_MAX, 0);
  printf("Mensaje recibido:\n%s\n", recv_buff);

  /* Analizar datos recibidos */
  get_client_args(recv_buff, &arg_day);

  /* Preparar datos para el envío */
  if (arg_day != -1) {
    WeatherInfo weather;
    char *weather_json;

    memset(&weather, 0, sizeof(WeatherInfo));
    get_weather(&weather, arg_day);
    weather_json = weather_to_json(&weather);
    send_len = MIN(SRV_SEND_MAX, strlen(weather_json));
    memcpy(send_buff, weather_json, send_len);

    g_free(weather_json);
  } else {
    send_len = sprintf(send_buff, "{\"error\":\"%s\"}", "Fecha incorrecta");
  }

  /* Enviar datos al cliente */
  send(connfd, send_buff, send_len, 0);
  printf("Mensaje enviado:\n%s\n", send_buff);
}

int main(int argc, char **argv)
{
  GError         *error = NULL;
  GOptionContext *context;
  TcpServer      *server;

  context = g_option_context_new(SRV_INFO);
  g_option_context_add_main_entries(context, options, NULL);
  g_option_context_parse(context, &argc, &argv, &error);
  g_option_context_free(context);

  if (error != NULL) {
    fprintf(stderr, "%s\n", error->message);
    g_error_free(error);
    return EXIT_FAILURE;
  }

  if (port <= 1024) {
    fprintf(stderr, "El puerto debe ser mayor a 1024\n");
    return EXIT_FAILURE;
  }

  printf("Iniciando %s...\n", SRV_NAME);
  json_parser = json_parser_new();
  server = tcp_server_new(addr, port);
  tcp_server_run(server, serve_weather, NULL, &error);
  tcp_server_free(server);
  g_object_unref(json_parser);

  if (error != NULL) {
    fprintf(stderr, "%s\n", error->message);
    g_error_free(error);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
