#include <glib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 24000
#define MAX_CONN 10
#define MAX_BUFF 255
#define CMD_EXIT "salir"

/**
 * @brief Servidor eco (lectura/escritura).
 *
 * Hace eco de los mensajes enviados por un cliente. Se cierra si recibe el
 * mensaje de salida definido en CMD_EXIT, por ejemplo, "salir".
 *
 * @param data (int) resultado de accept()
 * @param user_data sin utilizar por ahora
 */
void serve_echo(gpointer data, gpointer user_data)
{
    int connfd = GPOINTER_TO_INT(data);
    g_return_if_fail(connfd != -1);

    int n;
    char buff[MAX_BUFF];

    for (n = 0;;n++) {
        // Leer mensaje del cliente y copiarlo al búfer
        memset(buff, 0, MAX_BUFF);
        recv(connfd, buff, sizeof(buff), 0);
        printf("Mensaje recibido: %s", buff);

        // Enviar eco
        send(connfd, buff, sizeof(buff), 0);
        printf("Mensaje enviado: %s", buff);

        // Salir del bucle si el cliente sale
        if ((strncmp(buff, CMD_EXIT, strlen(CMD_EXIT))) == 0) {
            printf("El cliente ha salido.\n");
            break;
        }
    }

    close(connfd);
    printf("Mensajes recibidos: %d\n", n);
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
    int sockfd, connfd, binded, listening, max_threads;
    struct sockaddr_in srvaddr, cliaddr;
    size_t srvaddr_len = sizeof(srvaddr);
    size_t cliaddr_len = sizeof(cliaddr);

    GError *error = NULL;
    GThreadPool *thread_pool;

    // Asignar IP y puerto
    memset(&srvaddr, 0, srvaddr_len);
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    srvaddr.sin_port = htons(PORT);

    // Crear socket
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    g_return_val_if_fail(sockfd != -1, EXIT_FAILURE);
    printf("Socket creado correctamente...\n");

    // Enlazar socket creado a IP/puerto
    binded = bind(sockfd, (struct sockaddr*)&srvaddr, srvaddr_len);
    g_return_val_if_fail(binded == 0, EXIT_FAILURE);
    printf("Socket enlazado correctamente...\n");

    // Escuchar puerto
    listening = listen(sockfd, MAX_CONN);
    g_return_val_if_fail(listening == 0, EXIT_FAILURE);
    printf("Servidor escuchando...\n");

    // Crear thread pool
    max_threads = g_get_num_processors();
    thread_pool = g_thread_pool_new(serve_echo, NULL, max_threads, TRUE, &error);
    if (error != NULL) {
        fprintf(stderr, "Error: %s\n", error->message);
        return EXIT_FAILURE;
    }

    do {
        // Aceptar conexión de cliente
        connfd = accept(sockfd, (struct sockaddr*)&cliaddr, (socklen_t*)&cliaddr_len);
        g_return_val_if_fail(connfd != -1, EXIT_FAILURE);
        printf("Conexión de cliente aceptada por el servidor...\n");

        // Ejecutar función del servidor en otro hilo
        g_thread_pool_push(thread_pool, GINT_TO_POINTER(connfd), &error);

    } while (TRUE);

    // Cerrar socket
    close(sockfd);
    printf("Servidor cerrado.\n");

    g_thread_pool_free(thread_pool, FALSE, TRUE);

    return EXIT_SUCCESS;
}
