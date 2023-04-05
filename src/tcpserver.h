/**
 * @file tcpserver.h
 * @author Diego Pablo Matias Baltar (diego.baltar@est.fi.uncoma.edu.ar)
 * @brief Funciones para ejecutar un servidor TCP
 * @version 0.1
 * @date 2023-04-05
 */
#pragma once

#include <glib.h>
#include <stdbool.h>
#include <stdint.h>

/** Dominio de errores para funciones de TcpServer */
#define TCP_SERVER_ERROR (tcp_server_error_quark())

/** Códigos de error de TcpServer */
typedef enum
{
  TCP_SERVER_SOCK_ERROR,
  TCP_SERVER_SOCK_BIND_ERROR,
  TCP_SERVER_SOCK_LISTEN_ERROR,
  TCP_SERVER_SOCK_ACCEPT_ERROR,
} TcpServerError;

/** Contiene una configuración para un servidor TCP */
typedef struct TcpServer TcpServer;

/**
 * Tipo de función para ejecutar en tcp_server_run().
 *
 * @see tcp_server_run()
 * @param sockfd conexión TCP
 * @param data puntero a datos adicionales
 */
typedef void (*TcpServerFunc)(int sockfd, void *data);

/**
 * Crea una nueva configuración para un servidor TCP.
 *
 * La configuración se crea llamando a tcp_server_new_full() con el resto de las
 * opciones con los valores por defecto: máximo de conexiones en 10, máximo de
 * hilos igual a la cantidad de núcleos del procesador, e hilos no exclusivos.
 *
 * @see tcp_server_free()
 * @param addr la dirección del servidor
 * @param port el puerto del servidor
 * @return puntero a TcpServer, NULL en caso de error (debe liberarse con
 * tcp_server_free() cuando ya no se utilice)
 */
TcpServer *tcp_server_new(uint32_t addr, uint16_t port);

/**
 * Crea una nueva configuración para un servidor TCP con todas las opciones.
 *
 * La cantidad máxima de conexiones establece hasta que punto se aceptan y se
 * encolan las conexiones entrantes. Si entran más conexiones del límite se
 * rechazan. Además, el máximo de hilos establece cuantos hilos se reutilizarán
 * para atender las conexiones. Si se especifica -1, no se limitará la cantidad
 * de hilos. Por último, se puede especificar si los hilos son exclusivos o no.
 * Se debe usar con precaución, ya que si se establece a verdadero con -1 en la
 * cantidad de hilos, puede llegar a consumir recursos en exceso.
 *
 * @see tcp_server_new()
 * @see tcp_server_free()
 * @param addr dirección del servidor
 * @param port puerto del servidor
 * @param max_conn máximo de conexiones en cola
 * @param max_threads máximo de hilos para ejecutar las solicitudes
 * @param exclusive usar hilos exclusivos o no
 * @return puntero a TcpServer, NULL en caso de error (debe liberarse con
 * tcp_server_free() cuando ya no se utilice)
 */
TcpServer *tcp_server_new_full(uint32_t addr, uint16_t port, int max_conn, int max_threads, bool exclusive);

/**
 * Inicia el servidor TCP y ejecuta la función en un nuevo hilo por solicitud.
 *
 * Las solicitudes aceptadas se ejecutan en otro hilo, obtenido de un "pool" de
 * hilos según la configuración data.
 *
 * @param server configuración del servidor TCP
 * @param func función a ejecutar en un nuevo hilo
 * @param data parámetro adicional opcional para la función
 * @param error puntero a error recuperable, debe estar inicializado a NULL
 */
void tcp_server_run(TcpServer *server, TcpServerFunc func, void *data, GError **error);

/**
 * Libera los recursos asociados a una configuración TcpServer.
 *
 * @see tcp_server_new()
 * @see tcp_server_new_full()
 * @param server puntero a TcpClient
 */
void tcp_server_free(TcpServer *server);

/**
 * Devuelve el dominio de errores para el servidor TCP.
 *
 * @return el dominio de errores para utilizar con GError
 */
GQuark tcp_server_error_quark(void);
