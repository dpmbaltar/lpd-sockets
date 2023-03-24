#include <glib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "tcpserver.h"

/* Valores por defecto */
#define MAX_CONNECTIONS       10
#define MAX_THREADS           g_get_num_processors()
#define EXCLUSIVE_THREAD_POOL FALSE

/* Macros de ayuda */
#define return_throw_new_if(c,e,d) \
    if (c) {\
        error = g_error_new_literal(TCP_SERVER_ERROR, e, error_messages[e]);\
        g_propagate_error(d, error);\
        error = NULL;\
        return;\
    }

G_DEFINE_QUARK(tcp-server-error, tcp_server_error)

struct _TcpServer
{
    uint32_t     addr;
    uint16_t     port;
    uint32_t     max_conn;

    GThreadPool *thread_pool;
};

static GError *error = NULL;
static const char *error_messages[] = {
    [TCP_SERVER_SOCK_ERROR]        = "Error al crear socket",
    [TCP_SERVER_SOCK_BIND_ERROR]   = "Error al enlazar socket",
    [TCP_SERVER_SOCK_LISTEN_ERROR] = "Error al escuchar socket",
    [TCP_SERVER_SOCK_ACCEPT_ERROR] = "Error al aceptar conexión",
};

TcpServer *tcp_server_new_full(uint32_t    addr,
                               uint16_t    port,
                               GFunc       func,
                               gpointer    data,
                               int         max_conn,
                               int         max_threads,
                               bool        excl)
{
    GError *error = NULL;
    TcpServer *srv = (TcpServer*)malloc(sizeof(TcpServer));

    g_return_val_if_fail(srv != NULL, NULL);

    srv->addr = addr;
    srv->port = port;
    srv->max_conn = max_conn;
    srv->thread_pool = g_thread_pool_new(func, data, max_threads, excl, &error);

    if (error != NULL) {
        fprintf(stderr, "Error: %s\n", error->message);
        return NULL;
    }

    return srv;
}

TcpServer *tcp_server_new(uint32_t addr,
                          uint16_t port,
                          GFunc    func,
                          gpointer data)
{
    return tcp_server_new_full(addr, port, func, data,
                               MAX_CONNECTIONS,
                               MAX_THREADS,
                               EXCLUSIVE_THREAD_POOL);
}

void tcp_server_run(TcpServer  *srv,
                    GError    **err)
{
    int sockfd, connfd, binded, listening;
    struct sockaddr_in srvaddr, cliaddr;
    socklen_t srvaddr_len = sizeof(srvaddr);
    socklen_t cliaddr_len = sizeof(cliaddr);
    GError *local_err = NULL;

    // Asignar IP y puerto
    memset(&srvaddr, 0, srvaddr_len);
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = htonl(srv->addr);
    srvaddr.sin_port = htons(srv->port);

    // Crear socket
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    return_throw_new_if(sockfd == -1, TCP_SERVER_SOCK_ERROR, err);
    printf("Socket creado correctamente...\n");

    // Enlazar socket creado a IP/puerto
    binded = bind(sockfd, (struct sockaddr*)&srvaddr, srvaddr_len);
    return_throw_new_if(binded == -1, TCP_SERVER_SOCK_BIND_ERROR, err);
    printf("Socket enlazado correctamente...\n");

    // Escuchar puerto
    listening = listen(sockfd, srv->max_conn);
    return_throw_new_if(listening == -1, TCP_SERVER_SOCK_LISTEN_ERROR, err);
    printf("Servidor escuchando...\n");

    do {
        // Aceptar conexión de cliente
        connfd = accept(sockfd, (struct sockaddr*)&cliaddr, &cliaddr_len);
        return_throw_new_if(connfd == -1, TCP_SERVER_SOCK_ACCEPT_ERROR, err);
        printf("Conexión de cliente aceptada por el servidor...\n");

        // Ejecutar función del servidor en otro hilo
        g_thread_pool_push(srv->thread_pool,
                           GINT_TO_POINTER(connfd),
                           &local_err);

        if (local_err != NULL) {
            g_propagate_error(err, local_err);
            break;
        }

    } while (TRUE);

    // Cerrar socket
    close(sockfd);
    printf("Servidor cerrado.\n");
}

void tcp_server_free(TcpServer *srv)
{
    g_thread_pool_free(srv->thread_pool, FALSE, TRUE);
}
