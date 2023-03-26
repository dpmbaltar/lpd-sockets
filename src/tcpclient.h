#pragma once

#include <glib.h>

#ifdef G_OS_UNIX
#include <netinet/in.h>
#endif

#ifdef G_OS_WIN32
#include <stdint.h>
#include <winsock2.h>

typedef uint32_t in_addr_t;
typedef uint16_t in_port_t;
#endif

typedef struct _TcpClient TcpClient;

typedef void* (*TcpClientFunc)          (int             sockfd,
                                         gpointer        data);

TcpClient      *tcp_client_new          (in_addr_t       addr,
                                         in_port_t       port);

GThread        *tcp_client_run          (TcpClient      *client,
                                         TcpClientFunc   func,
                                         gpointer        func_data,
                                         GError        **error);

void            tcp_client_free         (TcpClient      *client);
