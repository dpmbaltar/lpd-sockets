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
#include "tcpclient.h"
#include "types.h"
#include "util.h"

/* Nombre del servidor */
#define SRV_NAME         "Servidor principal"
/* Descripción del servidor */
#define SRV_INFO         "- Servidor principal"
/* Dirección del servidor por defecto */
#define SRV_ADDR         INADDR_ANY
/* Puerto del servidor por defecto */
#define SRV_PORT         24000
/* Host del servidor del clima por defecto */
#define SRV_WEATHER_HOST "127.0.0.1"
/* Puerto del servidor del clima por defecto */
#define SRV_WEATHER_PORT 24001
/* Host del servidor del horóscopo por defecto */
#define SRV_HOROS_HOST   "127.0.0.1"
/* Puerto del servidor del horóscopo por defecto */
#define SRV_HOROS_PORT   24002
/* Cantidad máxima de conexiones por defecto */
#define SRV_MAX_CONN     10
/* Cantidad máxima de hilos por defecto (0 = g_get_num_processors()) */
#define SRV_MAX_THREADS  0
/* Indica si se usan hilos exclusivos (no por defecto) */
#define SRV_EXC_THREADS  false
/* Cantidad máxima para envío de bytes */
#define SRV_SEND_MAX     1024
/* Cantidad máxima para recepción de bytes */
#define SRV_RECV_MAX     1024

/* Dirección del servidor */
static uint32_t addr = SRV_ADDR;

/* Puerto del servidor */
static uint16_t port = SRV_PORT;

/* Host del servidor del clima */
static char *weather_host = SRV_WEATHER_HOST;

/* Puerto del servidor del clima*/
static uint16_t weather_port = SRV_WEATHER_PORT;

/* Host del servidor del horóscopo */
static char *horoscope_host = SRV_HOROS_HOST;

/* Puerto del servidor del horóscopo*/
static uint16_t horoscope_port = SRV_HOROS_PORT;

/* Máximo de conexiones */
static int max_conn = SRV_MAX_CONN;

/* Máximo de hilos */
static int max_threads = SRV_MAX_THREADS;

/* Usar hilos exclusivos */
static bool exclusive = SRV_EXC_THREADS;

/* Opciones de línea de comandos */
static GOptionEntry options[] =
{
  { "addr", 'a', 0, G_OPTION_ARG_INT, &addr, "Direccion A (0 = INADDR_ANY por defecto)", "A" },
  { "port", 'p', 0, G_OPTION_ARG_INT, &port, "Puerto P > 1024 del servidor (24000 por defecto)", "P" },
  { "weather-host", 'w', 0, G_OPTION_ARG_STRING, &weather_host, "Host WH del servidor del clima (localhost por defecto)", "WH" },
  { "weather-port", 'W', 0, G_OPTION_ARG_INT, &weather_port, "Puerto WP > 1024 del servidor del clima (24001 por defecto)", "WP" },
  { "horoscope-host", 's', 0, G_OPTION_ARG_STRING, &horoscope_host, "Host SH del servidor del horoscopo (localhost por defecto)", "SH" },
  { "horoscope-port", 'S', 0, G_OPTION_ARG_INT, &horoscope_port, "Puerto SP > 1024 del servidor del horoscopo (24002 por defecto)", "SP" },
  { "max-conn", 'c', 0, G_OPTION_ARG_INT, &max_conn, "Aceptar hasta C conexiones (10 por defecto)", "C" },
  { "max-threads", 't', 0, G_OPTION_ARG_INT, &max_threads, "Usar hasta T hilos o -1 sin limites (usar cantidad de procesadores por defecto)", "T" },
  { "exclusive", 'e', 0, G_OPTION_ARG_NONE, &exclusive, "Usar hilos exclusivos (falso por defecto)", NULL },
  { NULL }
};

/* Cliente para el servidor del clima */
static TcpClient *weather_client = NULL;

/* Cliente para el servidor del horóscopo */
static TcpClient *horoscope_client = NULL;

static void *get_info(int sockfd, gpointer data)
{
  char *request = (char*)data;

  g_return_val_if_fail(sockfd != -1, NULL);
  g_return_val_if_fail(request != NULL, NULL);

  char recv_buf[SRV_SEND_MAX+1];
  int send_len = strlen(request);

  memset(recv_buf, 0, sizeof(recv_buf));
  send(sockfd, request, send_len, 0);
  recv(sockfd, recv_buf, SRV_SEND_MAX, 0);

  return g_strdup(recv_buf);
}

static void serve(gpointer data, gpointer user_data)
{
  int connfd = GPOINTER_TO_INT(data);
  g_return_if_fail(connfd != -1);

  GError *error = NULL;
  GThread *wc_thread = NULL;
  GThread *hc_thread = NULL;
  char *weather_response = NULL;
  char *horoscope_response = NULL;
  char recv_buf[SRV_RECV_MAX+1];
  char send_buf[SRV_SEND_MAX+1];
  int recv_len = 0;
  int send_len = 0;

  memset(recv_buf, 0, sizeof(recv_buf));
  memset(send_buf, 0, sizeof(send_buf));

  /* Leer solicitud del cliente */
  recv_len = recv(connfd, recv_buf, SRV_RECV_MAX, 0);
  if (recv_len > 0) {
    printf("Mensaje recibido:\n%s\n", recv_buf);
    printf("Bytes recibidos:\n");
    printx_bytes(recv_buf, recv_len);

    /* Solicitar datos del clima */
    printf("Enviando mensaje al servidor del clima...\n");
    wc_thread = tcp_client_run(weather_client, get_info, &recv_buf, &error);
    if (error != NULL) {
      fprintf(stderr, "%s\n", error->message);
      g_error_free(error);
    }

    /* Solicitar datos del horóscopo */
    printf("Enviando mensaje al servidor del horóscopo...\n");
    hc_thread = tcp_client_run(horoscope_client, get_info, &recv_buf, &error);
    if (error != NULL) {
      fprintf(stderr, "%s\n", error->message);
      g_error_free(error);
    }

    if (wc_thread != NULL) {
      weather_response = g_thread_join(wc_thread);
      printf("Datos del clima recibidos:\n%s\n", weather_response);
    }

    if (hc_thread != NULL) {
      horoscope_response = g_thread_join(hc_thread);
      printf("Datos del horóscopo recibidos:\n%s\n", horoscope_response);
    }
  }

  /* Armar y enviar respuesta */
  send_len = sprintf(send_buf, "{\"clima\":%s,\"horoscopo\":%s}",
                     weather_response != NULL ? weather_response : "null",
                     horoscope_response != NULL ? horoscope_response : "null");
  g_free(weather_response);
  g_free(horoscope_response);
  send(connfd, send_buf, send_len, 0);

  close(connfd);
  printf("Desconectado del cliente.\n");
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

  if (port <= 1024 || weather_port <= 1024 || horoscope_port <= 1024) {
    fprintf(stderr, "Los puertos deben ser mayor a 1024\n");
    return EXIT_FAILURE;
  }

  if (max_threads == 0) {
    max_threads = g_get_num_processors();
  }

  printf("Iniciando %s...\n", SRV_NAME);
  weather_client = tcp_client_new(weather_host, weather_port);
  horoscope_client = tcp_client_new(horoscope_host, horoscope_port);
  server = tcp_server_new_full(addr, port, serve, NULL, max_conn, max_threads,
                               exclusive);
  tcp_server_run(server, &error);
  tcp_server_free(server);
  tcp_client_free(weather_client);
  tcp_client_free(horoscope_client);

  if (error != NULL) {
    fprintf(stderr, "%s\n", error->message);
    g_error_free(error);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
