#include <arpa/inet.h>
#include <glib.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 24000
#define SERV_ADDR "127.0.0.1"
#define MAX_BUFF 255
#define CMD_EXIT "salir"

/**
 * @brief Función del cliente (write()/read()).
 *
 * Toma mensajes del usuario y los envía al servidor.
 *
 * @param sockfd resultado de connect()
 */
void client(int sockfd)
{
    char buff[MAX_BUFF];
    int len;
    int n;

    for (n = 0;;n++) {
        // Leer mensaje del usuario al búfer
        len = 0;
        memset(buff, 0, MAX_BUFF);
        printf("Escribir mensaje: ");
        while ((buff[len++] = getchar()) != '\n');

        // Enviar mensaje al servidor
        send(sockfd, buff, sizeof(buff), 0);
        printf("Mensaje enviado...\n");

        // Salir si se escribe el mensaje CMD_EXIT
        if ((strncmp(buff, CMD_EXIT, strlen(CMD_EXIT))) == 0) {
            printf("Cliente saliendo...\n");
            break;
        }

        // Leer mensaje de respuesta del servidor al búfer
        memset(buff, 0, sizeof(buff));
        recv(sockfd, buff, sizeof(buff), 0);
        printf("Respuesta del servidor: %s\n", buff);
    }

    printf("Mensajes enviados: %d\n", n);
}

/**
 * @brief Cliente TCP básico.
 *
 * Ejecuta un cliente TCP básico:
 * 1. socket()
 * 2. connect()
 * 3. write()/read()
 * 4. close()
 *
 * @param argc
 * @param argv
 * @return 0 si la ejecución es exitosa, 1 si falla
 */
int main(int argc, char **argv)
{
    int sockfd, connfd;
    struct sockaddr_in servaddr;
    size_t servaddr_len = sizeof(servaddr);

    // Asignar IP y puerto
    memset(&servaddr, 0, servaddr_len);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(SERV_ADDR);
    servaddr.sin_port = htons(PORT);

    // Crear socket
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    g_return_val_if_fail(sockfd != -1, EXIT_FAILURE);
    printf("Socket creado correctamente...\n");

    // Conectar socket del cliente al socket del servidor
    connfd = connect(sockfd, (struct sockaddr*)&servaddr, servaddr_len);
    g_return_val_if_fail(connfd == 0, EXIT_FAILURE);
    printf("Conectado al servidor...\n");

    // Ejecutar función del cliente
    client(sockfd);

    // Cerrar socket
    close(sockfd);
    printf("Conexión con el servidor cerrada.\n");

    return EXIT_SUCCESS;
}
