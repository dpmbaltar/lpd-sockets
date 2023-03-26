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
#include "weather.h"
#include "util.h"

/* Descripción del servidor */
#define SRV_INFO         "- Servidor principal"
/* Dirección del servidor por defecto */
#define SRV_ADDR         INADDR_ANY
/* Puerto del servidor por defecto */
#define SRV_PORT         24000
/* Puerto del servidor del clima por defecto */
#define SRV_WEATHER_PORT 24001
/* Cantidad máxima de conexiones por defecto */
#define SRV_MAX_CONN     10
/* Cantidad máxima de hilos por defecto (0 = g_get_num_processors()) */
#define SRV_MAX_THREADS  0
/* Indica si se usan hilos exclusivos (no por defecto) */
#define SRV_EXC_THREADS  false
/* Cantidad máxima para envío de bytes */
#define SRV_SEND_MAX     80
/* Cantidad máxima para recepción de bytes */
#define SRV_RECV_MAX     20

/* Dirección del servidor */
static in_addr_t addr = SRV_ADDR;

/* Puerto del servidor */
static in_port_t port = SRV_PORT;

/* Máximo de conexiones */
static int max_conn = SRV_MAX_CONN;

/* Máximo de hilos */
static int max_threads = SRV_MAX_THREADS;

/* Usar hilos exclusivos */
static bool exclusive = SRV_EXC_THREADS;

/* Opciones de línea de comandos */
static GOptionEntry options[] =
{
  { "addr", 'a', 0, G_OPTION_ARG_INT, &addr, "Usar direccion A (0 = INADDR_ANY por defecto)", "A" },
  { "port", 'p', 0, G_OPTION_ARG_INT, &port, "Enlazar al puerto P > 1024 (24000 por defecto)", "P" },
  { "max_conn", 'c', 0, G_OPTION_ARG_INT, &max_conn, "Aceptar hasta C conexiones (10 por defecto)", "C" },
  { "max_threads", 't', 0, G_OPTION_ARG_INT, &max_threads, "Usar hasta T hilos o -1 sin limites (usar cantidad de procesadores por defecto)", "T" },
  { "exclusive", 'e', 0, G_OPTION_ARG_NONE, &exclusive, "Usar hilos exclusivos (falso por defecto)", NULL },
  { NULL }
};

/* Cliente para el servidor del clima */
static TcpClient *weather_client = NULL;

static void *get_weather(int sockfd, gpointer _request)
{
  Request *request = (Request*)_request;

  g_return_val_if_fail(sockfd != -1, NULL);
  g_return_val_if_fail(request != NULL, NULL);

  char recv_buf[request->recv_len];

  printf("Enviar mensaje al servidor del clima.\n");
  send(sockfd, request->send, request->send_len, 0);

  printf("Recibir mensaje del servidor del clima.\n");
  recv(sockfd, recv_buf, request->recv_len, 0);
  memcpy(request->recv, recv_buf, request->recv_len);

  return NULL;
}

void serve(gpointer data, gpointer user_data)
{
  int connfd = GPOINTER_TO_INT(data);
  g_return_if_fail(connfd != -1);

  GError *error = NULL;
  GThread *weather_thread = NULL;
  Date date = DATE_INIT;
  WeatherInfo weather = WEATHER_INFO_INIT;
  Request req = REQUEST_INIT;
  int recv_len = 0;
  char recv_buf[SRV_RECV_MAX];
  char send_buf[SRV_SEND_MAX];
  memset(recv_buf, 0, sizeof(recv_buf));
  memset(send_buf, 0, sizeof(send_buf));

  /* Leer solicitud del cliente */
  recv_len = recv(connfd, recv_buf, sizeof(recv_buf), 0);
  if (recv_len > 0) {
    memcpy(&date, recv_buf, sizeof(date));
    printf("Mensaje recibido: %.*s", recv_len, recv_buf);
    printf("Bytes recibidos:\n");
    for (int i = 0; i < recv_len; i++)
      printf("%02x ", (unsigned char)recv_buf[i]);
    printf("\n");

    /* Solicitar datos del clima */
    req.send = &date;
    req.recv = &weather;
    req.send_len = sizeof(date);
    req.recv_len = sizeof(weather);
    weather_thread = tcp_client_run(weather_client, get_weather, &req, &error);

    if (weather_thread != NULL) {
      g_thread_join(weather_thread);
      g_thread_unref(weather_thread);
      memcpy(send_buf, &weather, sizeof(weather));
      printf("Datos del clima recibidos: {%s, %d, %.1f}\n",
             weather.date, weather.cond, weather.temp);

      /* Enviar datos obtenidos al cliente */
      send(connfd, send_buf, sizeof(send_buf), 0);
    }
  }

  close(connfd);
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

  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    fprintf(stderr, "%s\n", error->message);
    return EXIT_FAILURE;
  }

  if (port <= 1024) {
    fprintf(stderr, "El puerto debe ser mayor a 1024\n");
    return EXIT_FAILURE;
  }

  if (max_threads == 0) {
    max_threads = g_get_num_processors();
  }

  weather_client = tcp_client_new(addr, SRV_WEATHER_PORT);
  server = tcp_server_new_full(addr, port, serve, NULL, max_conn, max_threads,
                               exclusive);
  tcp_server_run(server, &error);

  if (error != NULL) {
    fprintf(stderr, "%s", error->message);
    return EXIT_FAILURE;
  }

  tcp_server_free(server);
  g_option_context_free(context);

  return EXIT_SUCCESS;
}
