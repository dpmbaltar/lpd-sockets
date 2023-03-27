#pragma once

#include <glib.h>
#include <stdint.h>

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
