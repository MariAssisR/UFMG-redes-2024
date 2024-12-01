#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include "common.h"

int connect_to_server(const char *hostname, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
        error("ERROR opening socket");

    struct hostent *server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    return sockfd;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "usage: %s <hostname> <port_user> <port_location> <location_code>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *hostname = argv[1];
    int port_user = atoi(argv[2]);
    int port_location = atoi(argv[3]);
    int location_code = atoi(argv[4]);

    // Conectar ao servidor de usuários
    int sockfd_user = connect_to_server(hostname, port_user);
    printf("Conectado ao servidor de usuários na porta %d.\n", port_user);

    // Enviar mensagem ao servidor de usuários
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "Location: %d", location_code);
    if (send(sockfd_user, buffer, strlen(buffer), 0) < 0)
        error("ERROR writing to user server socket");

    // Receber resposta do servidor de usuários
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(sockfd_user, buffer, BUFFER_SIZE - 1, 0) < 0)
        error("ERROR reading from user server socket");

    printf("Resposta do servidor de usuários: %s\n", buffer);
    close(sockfd_user);

    // Conectar ao servidor de localização
    int sockfd_location = connect_to_server(hostname, port_location);
    printf("Conectado ao servidor de localização na porta %d.\n", port_location);

    // Enviar mensagem ao servidor de localização
    snprintf(buffer, BUFFER_SIZE, "Requesting location data for code: %d", location_code);
    if (send(sockfd_location, buffer, strlen(buffer), 0) < 0)
        error("ERROR writing to location server socket");

    // Receber resposta do servidor de localização
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(sockfd_location, buffer, BUFFER_SIZE - 1, 0) < 0)
        error("ERROR reading from location server socket");

    printf("Resposta do servidor de localização: %s\n", buffer);
    close(sockfd_location);

    return EXIT_SUCCESS;
}
