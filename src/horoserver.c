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
/* Máximo de información para el horóscopo (i.e. mood_len/mood de AstroInfo) */
#define H_MOOD_MAX 255
/* Archivo de datos del horóscopo */
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
  { "addr", 'a', 0, G_OPTION_ARG_INT, &addr, "Direccion (0=INADDR_ANY)", "A" },
  { "port", 'p', 0, G_OPTION_ARG_INT, &port, "Puerto (>1024)", "P" },
  { "horoscope-file", 'f', 0, G_OPTION_ARG_FILENAME, &horoscope_file, "Archivo de datos del horóscopo", "F"},
  { NULL }
};

/* Rangos de fechas para los signos */
static const char astro_date_ranges[N_SIGNS][4] =
{
  { 3, 21, 4, 19 },
  { 4, 20, 5, 20 },
  { 5, 21, 6, 21 },
  { 6, 22, 7, 22 },
  { 7, 23, 8, 22 },
  { 8, 23, 9, 22 },
  { 9, 23, 10, 22 },
  { 10, 23, 11, 22 },
  { 11, 23, 12, 21 },
  { 12, 22, 1, 19 },
  { 1, 20, 2, 18 },
  { 2, 19, 3, 20 }
};

/* Datos del horóscopo que se cargan de un archivo dado */
static char astro_moods[N_SIGNS][H_MOOD_MAX] = { 0 };

/* Caché de datos del horóscopo */
static AstroInfo astro_data[H_MAX_DAYS][N_SIGNS] = { 0 };

/* Marcas de tiempo de la caché */
static time_t astro_cache[H_MAX_DAYS] = { 0 };

static void create_horoscope(AstroInfo *astro_info, int sign)
{
  GRand *rand = g_rand_new();
  int mood_num = g_rand_int_range(rand, 0, N_SIGNS);

  astro_info->sign = sign;
  astro_info->sign_compat = g_rand_int_range(rand, 0, N_SIGNS);
  astro_info->mood = g_strdup(astro_moods[mood_num]); /* Liberar con g_free() */
  astro_info->mood_len = strlen(astro_info->mood);
  memcpy(astro_info->date_range, astro_date_ranges[astro_info->sign], 4);

  g_free(rand);
}

static void get_horoscope(AstroInfo *astro_info, int day, int sign)
{
  static GMutex mutex;
  struct timeval ts;

  g_return_if_fail(astro_info != NULL);
  g_return_if_fail(day >= H_MIN_DAYS && day <= H_MAX_DAYS);
  g_return_if_fail(sign < N_SIGNS);

  gettimeofday(&ts, NULL);
  g_mutex_lock(&mutex);

  if ((ts.tv_sec - astro_cache[day]) > SRV_DATA_TTL) {
    if (astro_data[day][sign].mood != NULL) {
      g_free(astro_data[day][sign].mood);
    }

    AstroInfo astro_info_day = ASTRO_INFO_INIT;
    create_horoscope(&astro_info_day, sign);
    memcpy(&astro_data[day][sign], &astro_info_day, sizeof(astro_info_day));

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

static void serve_horoscope(gpointer data, gpointer user_data)
{
  int connfd = GPOINTER_TO_INT(data);
  g_return_if_fail(connfd != -1);

  int arg_day = -1;
  int arg_sign = 0;
  int send_len = SRV_SEND_MAX;
  char recv_buff[SRV_RECV_MAX];
  char send_buff[SRV_SEND_MAX];

  memset(recv_buff, 0, sizeof(recv_buff));
  memset(send_buff, 0, sizeof(send_buff));

  /* Leer solicitud del cliente */
  recv(connfd, recv_buff, sizeof(recv_buff), 0);
  printf("Bytes recibidos:\n");
  printx_bytes(recv_buff, (int)sizeof(recv_buff));

  /* Analizar datos recibidos */
  arg_day = get_client_arg(recv_buff, (int)sizeof(recv_buff));

  /* Preparar datos para el envío */
  if (arg_day != -1) {
    AstroInfo astro_info = ASTRO_INFO_INIT;
    send_len = ASTRO_INFO_FSIZE(&astro_info);
    get_horoscope(&astro_info, arg_day, arg_sign);
    memcpy(send_buff, &astro_info, send_len);

    /* Ajustar cantidad de datos a enviar según el tamaño de AstroInfo.mood */
    if (astro_info.mood_len > 0 && astro_info.mood != NULL) {
      int mood_start = send_len;
      send_len = ASTRO_INFO_DSIZE(&astro_info);

      /* Acortar datos si excede el máximo del búfer de envío de datos */
      if (send_len > SRV_SEND_MAX) {
        astro_info.mood_len = SRV_SEND_MAX - ASTRO_INFO_FSIZE(&astro_info);
      }

      memcpy(&send_buff[mood_start], astro_info.mood, astro_info.mood_len);
    } else {
      send_len = ASTRO_INFO_FSIZE(&astro_info);
    }

    printf("Mensaje enviado: {%d, %d, %d/%d - %d/%d, %.*s}\n",
           astro_info.sign,
           astro_info.sign_compat,
           astro_info.date_range[0],
           astro_info.date_range[1],
           astro_info.date_range[2],
           astro_info.date_range[3],
           astro_info.mood_len,
           astro_info.mood);
  }

  /* Enviar datos al cliente */
  send(connfd, send_buff, send_len, 0);
  printf("Bytes enviados (%d):\n", send_len);
  printx_bytes(send_buff, send_len);

  /* Cerrar conexión */
  close(connfd);
  printf("Desconectado del cliente.\n");
}

int main(int argc, char **argv)
{
  GError         *error = NULL;
  GOptionContext *context;
  TcpServer      *server;
  bool            options_parsed = FALSE;

  context = g_option_context_new(SRV_INFO);
  g_option_context_add_main_entries(context, options, NULL);
  options_parsed = g_option_context_parse(context, &argc, &argv, &error);
  g_option_context_free(context);

  if (!options_parsed) {
    fprintf(stderr, "Error al obtener opciones\n");
  }

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
      memcpy(&astro_moods[file_line], file_buf, sizeof(file_buf));
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
  server = tcp_server_new(addr, port, serve_horoscope, NULL);
  tcp_server_run(server, &error);
  tcp_server_free(server);

  if (error != NULL) {
    fprintf(stderr, "%s\n", error->message);
    g_error_free(error);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
