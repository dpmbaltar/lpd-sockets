#pragma once

#include <glib.h>
#include <stdint.h>

#define TCP_CLIENT_ERROR (tcp_client_error_quark())

/* Códigos de error de TcpClient */
typedef enum _TcpClientError
{
  TCP_CLIENT_SOCK_ERROR,
  TCP_CLIENT_SOCK_CONNECT_ERROR,
} TcpClientError;

typedef struct _TcpClient TcpClient;

typedef void* (*TcpClientFunc)          (int             sockfd,
                                         gpointer        data);

TcpClient      *tcp_client_new          (const char     *host,
                                         uint16_t        port);

GThread        *tcp_client_run          (TcpClient      *client,
                                         TcpClientFunc   func,
                                         gpointer        func_data,
                                         GError        **error);

void            tcp_client_free         (TcpClient      *client);

GQuark          tcp_client_error_quark  (void);
