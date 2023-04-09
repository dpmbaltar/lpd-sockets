/**
 * @file horoscopeserver.c
 * @author Diego Pablo Matias Baltar (diego.baltar@est.fi.uncoma.edu.ar)
 * @brief Servidor del horóscopo
 * @version 0.1
 * @date 2023-04-04
 *
 * Programa del servidor del horóscopo, que recibe consultas por fechas válidas
 * a partir de la fecha actual, hasta siete días en adelante, y por el signo.
 * Los datos del horóscopo se guardan en memoria por 1 día. Si se consulta luego
 * de 1 día generado los datos, se actualizan.
 *
 * A continuación se detallan las opciones por parámetros que toma el servidor,
 * que también puede verse al ejecutar el programa con el parámetro -h o --help:
 *
 * @code{.unparsed}
 * ./horoscope_server --help
 * @endcode
 *
 * Resultado:
 *
 * @code{.unparsed}
 * Uso:
 *   horoscope_server [OPTION?] - Servidor del horóscopo
 *
 * Opciones de ayuda:
 *   -h, --help             Muestra opciones de ayuda
 *
 * Opciones de aplicación:
 *   -a, --addr=A           Direccion A (0 = INADDR_ANY por defecto)
 *   -p, --port=P           Puerto P > 1024 del servidor (24002 por defecto)
 *   -f, --horos-file=F     Archivo de datos del horóscopo
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
#define SRV_NAME     "Servidor del horóscopo"
/** Descripción del programa */
#define SRV_INFO     "- Servidor del horóscopo"
/** Dirección del servidor por defecto */
#define SRV_ADDR     INADDR_ANY
/** Puerto del servidor por defecto */
#define SRV_PORT     24002
/** Cantidad máxima para envío de bytes */
#define SRV_SEND_MAX 1023
/** Cantidad máxima para recepción de bytes */
#define SRV_RECV_MAX 255
/** TTL para datos del horóscopo (segundos) */
#define SRV_DATA_TTL 86400

/** Mímino de días para el horóscopo, a partir de la fecha actual */
#define H_MIN_DAYS 0
/** Máximo de días para el horóscopo, a partir de la fecha actual */
#define H_MAX_DAYS 7
/** Máximo de información para el horóscopo @see AstroInfo::mood */
#define H_MOOD_MAX 255
/** Archivo de datos del horóscopo */
#define H_FILENAME "horoscope.txt"

/* Dirección del servidor */
static uint32_t addr = SRV_ADDR;

/* Puerto del servidor */
static uint16_t port = SRV_PORT;

/* Archivo de datos del horóscopo */
static char *horoscope_file = NULL;

/* Opciones de línea de comandos */
static GOptionEntry options[] =
{
  { "addr", 'a', 0, G_OPTION_ARG_INT, &addr, "Direccion A (0 = INADDR_ANY por defecto)", "A" },
  { "port", 'p', 0, G_OPTION_ARG_INT, &port, "Puerto P > 1024 del servidor (24002 por defecto)", "P" },
  { "horos-file", 'f', 0, G_OPTION_ARG_FILENAME, &horoscope_file, "Archivo de datos del horóscopo", "F"},
  { NULL }
};

/* Instancia de JsonParser */
static JsonParser *json_parser = NULL;

/* Signos */
static const char *astro_signs[N_SIGNS] =
{
  [S_ARIES]       = "aries",
  [S_TAURUS]      = "tauro",
  [S_GEMINI]      = "geminis",
  [S_CANCER]      = "cancer",
  [S_LEO]         = "leo",
  [S_VIRGO]       = "virgo",
  [S_LIBRA]       = "libra",
  [S_SCORPIO]     = "escorpio",
  [S_SAGITTARIUS] = "sagitario",
  [S_CAPRICORN]   = "capricornio",
  [S_AQUARIUS]    = "acuario",
  [S_PISCES]      = "piscis"
};

/* Rangos de fechas para los signos */
static const char astro_date_ranges[N_SIGNS][2][6] =
{
  { "03-21", "04-19" },
  { "04-20", "05-20" },
  { "05-21", "06-21" },
  { "06-22", "07-22" },
  { "07-23", "08-22" },
  { "08-23", "09-22" },
  { "09-23", "10-22" },
  { "10-23", "11-22" },
  { "11-23", "12-21" },
  { "12-22", "01-19" },
  { "01-20", "02-18" },
  { "02-19", "03-20" }
};

/* Datos del horóscopo que se cargan de un archivo dado */
static char astro_moods[N_SIGNS][H_MOOD_MAX] = { 0 };

/* Caché de datos del horóscopo */
static AstroInfo astro_data[H_MAX_DAYS][N_SIGNS] = { 0 };

/* Marcas de tiempo de la caché */
static time_t astro_cache[H_MAX_DAYS][N_SIGNS] = { 0 };

static void create_horoscope(AstroInfo *astro_info, unsigned int sign)
{
  g_return_if_fail(astro_info != NULL);
  g_return_if_fail(sign < N_SIGNS);

  GRand *rand = g_rand_new();
  int mood_n = g_rand_int_range(rand, 0, N_SIGNS);

  astro_info->sign = sign;
  astro_info->sign_compat = g_rand_int_range(rand, 0, N_SIGNS);
  memcpy(astro_info->mood, &astro_moods[mood_n], sizeof(((AstroInfo*)0)->mood));
  memcpy(astro_info->date_range,
         astro_date_ranges[sign],
         sizeof(((AstroInfo*)0)->date_range));

  g_free(rand);
}

static void get_horoscope(AstroInfo *astro_info, int day, unsigned int sign)
{
  static GMutex mutex;
  struct timeval time;

  g_return_if_fail(astro_info != NULL);
  g_return_if_fail(day >= H_MIN_DAYS && day <= H_MAX_DAYS);
  g_return_if_fail(sign < N_SIGNS);

  g_mutex_lock(&mutex);
  gettimeofday(&time, NULL);

  if ((time.tv_sec - astro_cache[day][sign]) > SRV_DATA_TTL) {
    create_horoscope(&astro_data[day][sign], sign);
    astro_cache[day][sign] = time.tv_sec;
  }

  memcpy(astro_info, &astro_data[day][sign], sizeof(AstroInfo));
  g_mutex_unlock(&mutex);
}

static int parse_sign(const char *data)
{
  g_return_val_if_fail(data != NULL, -1);

  int sign = -1;
  int length = strlen(data);

  for (int i = 0; i < N_SIGNS; i++) {
    if (g_ascii_strncasecmp(astro_signs[i], data, length) == 0) {
      sign = i;
      break;
    }
  }

  return sign;
}

static void get_client_args(const char *data, int *arg_day, int *arg_sign)
{
  g_return_if_fail(data != NULL);
  g_return_if_fail(arg_day != NULL);
  g_return_if_fail(arg_sign != NULL);

  struct timeval time;
  int day = -1;
  int sign = -1;
  const char *date_str = NULL;
  const char *sign_str = NULL;
  GDate *date = NULL;
  GDate *today = NULL;
  JsonNode *json_node = NULL;
  JsonObject *json_object = NULL;

  json_node = parse_json(data, strlen(data), json_parser);
  json_object = json_node_get_object(json_node);
  sign_str = json_object_get_string_member(json_object, "signo");
  date_str = json_object_get_string_member(json_object, "fecha");

  if (sign_str != NULL)
    sign = parse_sign(sign_str);

  if (date_str != NULL)
    date = parse_date(date_str);

  if (date != NULL) {
    today = g_date_new();
    gettimeofday(&time, NULL);
    g_date_set_time_t(today, time.tv_sec);
    day = g_date_days_between(today, date);

    g_date_free(date);
    g_date_free(today);
  }

  *arg_day = day >= H_MIN_DAYS && day <= H_MAX_DAYS ? day : -1;
  *arg_sign = sign >= 0 && sign < N_SIGNS ? sign : -1;
}

static char *astro_to_json(AstroInfo *astro_info)
{
  g_return_val_if_fail(astro_info != NULL, NULL);

  return g_strdup_printf("{\"signo\":\"%s\",\"compatible\":\"%s\",\"periodo\":[\"%s\",\"%s\"],\"estado\":\"%s\"}",
                         astro_signs[astro_info->sign],
                         astro_signs[astro_info->sign_compat],
                         astro_info->date_range[0],
                         astro_info->date_range[1],
                         astro_info->mood);
}

static void serve_horoscope(int connfd, void *data)
{
  g_return_if_fail(connfd != -1);

  int arg_day = -1;
  int arg_sign = -1;
  int send_len = SRV_SEND_MAX;
  char recv_buff[SRV_RECV_MAX+1];
  char send_buff[SRV_SEND_MAX+1];

  memset(recv_buff, 0, sizeof(recv_buff));
  memset(send_buff, 0, sizeof(send_buff));

  /* Leer solicitud del cliente */
  recv(connfd, recv_buff, SRV_RECV_MAX, 0);
  printf("Mensaje recibido:\n%s\n", recv_buff);

  /* Analizar datos recibidos */
  get_client_args(recv_buff, &arg_day, &arg_sign);

  /* Preparar datos para el envío */
  if (arg_day != -1 && arg_sign != -1) {
    AstroInfo astro_info;
    char *astro_json;

    memset(&astro_info, 0, sizeof(AstroInfo));
    get_horoscope(&astro_info, arg_day, arg_sign);
    astro_json = astro_to_json(&astro_info);
    send_len = MIN(SRV_SEND_MAX, strlen(astro_json));
    memcpy(send_buff, astro_json, send_len);

    g_free(astro_json);
  } else {
    send_len = sprintf(send_buff,
                       "{\"error\":\"%s\"}",
                       "Fecha y/o signo incorrectos");
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

  if (horoscope_file == NULL) {
    horoscope_file = H_FILENAME;
    printf("No se indica un archivo para datos del horóscopo\n");
  }

  /* Leer datos del horóscopo desde archivo */
  int file_line = 0;
  char file_buf[H_MOOD_MAX];
  FILE *file = fopen(horoscope_file, "r");
  if (file == NULL) {
    perror(horoscope_file);
    return EXIT_FAILURE;
  } else {
    memset(file_buf, 0, sizeof(file_buf));
    while (fgets(file_buf, sizeof(file_buf), file) && file_line < N_SIGNS) {
      memcpy(&astro_moods[file_line], file_buf, strcspn(file_buf, "\n"));
      memset(file_buf, 0, sizeof(file_buf));
      file_line++;
    }
    fclose(file);
  }

  if (port <= 1024) {
    fprintf(stderr, "El puerto debe ser mayor a 1024\n");
    return EXIT_FAILURE;
  }

  printf("Iniciando %s...\n", SRV_NAME);
  json_parser = json_parser_new();
  server = tcp_server_new(addr, port);
  tcp_server_run(server, serve_horoscope, NULL, &error);
  tcp_server_free(server);
  g_object_unref(json_parser);

  if (error != NULL) {
    fprintf(stderr, "%s\n", error->message);
    g_error_free(error);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
