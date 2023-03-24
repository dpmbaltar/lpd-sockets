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

TcpClient      *tcp_client_new          (in_addr_t   addr,
                                         in_port_t   port,
                                         GFunc       func,
                                         gpointer    data);

void            tcp_client_run          (TcpClient  *client,
                                         GError    **error);

void            tcp_client_free         (TcpClient  *client);
