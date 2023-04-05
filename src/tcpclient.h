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

/** Dominio de errores para funciones de TcpClient */
#define TCP_CLIENT_ERROR (tcp_client_error_quark())

/** Códigos de error para funciones de TcpClient */
typedef enum
{
  TCP_CLIENT_SOCK_ERROR,
  TCP_CLIENT_SOCK_CONNECT_ERROR,
} TcpClientError;

/** Contiene una configuración para un cliente TCP */
typedef struct TcpClient TcpClient;

/**
 * Tipo de función para ejecutar en tcp_client_run().
 *
 * @see tcp_client_run()
 * @param sockfd conexión TCP
 * @param data puntero a datos adicionales
 * @return el resultado de la función
 */
typedef void *(*TcpClientFunc)(int sockfd, void *data);

/**
 * Crea una nueva configuración para un cliente TCP.
 *
 * @see tcp_client_free()
 * @param host el nombre del host
 * @param port el puerto del host
 * @return puntero a TcpClient, NULL en caso de error (debe liberarse con
 * tcp_client_free() cuando ya no se utilice)
 */
TcpClient *tcp_client_new(const char *host, uint16_t port);

/**
 * Abre una conexión TCP y ejecuta la función dada en un nuevo hilo.
 *
 * @param client el cliente TCP
 * @param func la función a ejecutar en un nuevo hilo
 * @param data parámetro adicional opcional para la función
 * @param error puntero a error recuperable, debe estar inicializado a NULL
 * @return el hilo de ejecución de la función, NULL en caso de error
 */
GThread *tcp_client_run(TcpClient *client, TcpClientFunc func, void *data, GError **error);

/**
 * Libera los recursos asignados por tcp_client_new().
 *
 * @see tcp_client_new()
 * @param client puntero a TcpClient
 */
void tcp_client_free(TcpClient *client);

/**
 * Devuelve el dominio de errores para el cliente TCP.
 *
 * @return el dominio de errores para utilizar con GError
 */
GQuark tcp_client_error_quark(void);
