/**
 * @file tcpclient.h
 * @author Diego Pablo Matias Baltar (diego.baltar@est.fi.uncoma.edu.ar)
 * @brief Funciones para ejecutar un cliente TCP
 * @version 0.1
 * @date 2023-03-29
 */
#pragma once

#include <glib.h>
#include <stdint.h>

/** @brief Dominio de errores para funciones de TcpClient */
#define TCP_CLIENT_ERROR (tcp_client_error_quark())

/** @brief Códigos de error para funciones de TcpClient */
typedef enum _TcpClientError
{
  TCP_CLIENT_SOCK_ERROR,
  TCP_CLIENT_SOCK_CONNECT_ERROR,
} TcpClientError;

/** @brief Representa una configuración para un cliente TCP */
typedef struct _TcpClient TcpClient;

/**
 * @brief Tipo de función para ejecutar en @link tcp_client_run()
 * @see tcp_client_run()
 * @param sockfd conexión TCP
 * @param data puntero a datos opcionales
 * @return el resultado de la función
 */
typedef void *(*TcpClientFunc)          (int             sockfd,
                                         gpointer        data);

TcpClient      *tcp_client_new          (const char     *host,
                                         uint16_t        port);

GThread        *tcp_client_run          (TcpClient      *client,
                                         TcpClientFunc   func,
                                         gpointer        func_data,
                                         GError        **error);

void            tcp_client_free         (TcpClient      *client);

GQuark          tcp_client_error_quark  (void);
