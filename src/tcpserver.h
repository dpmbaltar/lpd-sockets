#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <glib.h>
#include <stdbool.h>
#include <stdint.h>

#define TCP_SERVER_ERROR (tcp_server_error_quark())

/* Códigos de error de TcpServer */
typedef enum _TcpServerError
{
  TCP_SERVER_SOCK_ERROR,
  TCP_SERVER_SOCK_BIND_ERROR,
  TCP_SERVER_SOCK_LISTEN_ERROR,
  TCP_SERVER_SOCK_ACCEPT_ERROR,
} TcpServerError;

typedef struct _TcpServer TcpServer;

TcpServer      *tcp_server_new          (uint32_t    addr,
                                         uint16_t    port,
                                         GFunc       func,
                                         gpointer    data);

TcpServer      *tcp_server_new_full     (uint32_t    addr,
                                         uint16_t    port,
                                         GFunc       func,
                                         gpointer    data,
                                         int         max_conn,
                                         int         max_threads,
                                         bool        exclusive);

void            tcp_server_run          (TcpServer  *server,
                                         GError    **error);

void            tcp_server_free         (TcpServer  *server);

GQuark          tcp_server_error_quark  (void);

#endif
