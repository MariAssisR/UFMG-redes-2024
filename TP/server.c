#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>  // Incluir este cabeçalho para usar getaddrinfo
#include "common.h"

int configure_socket(const char *socket_type, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
        error("ERROR opening socket");

    // Configurar SO_REUSEADDR
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        error("ERROR setting SO_REUSEADDR");

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);
    printf("Servidor aguardando conexões %s na porta %d...\n", socket_type, port);

    return sockfd;
}

void connect_peer_or_listen(int *peer_sock, const char *hostname, int port) {
    struct sockaddr_in serv_addr;
    struct addrinfo hints, *res;

    // Preencher a estrutura 'hints' para configurar getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    // Obtém as informações do host usando getaddrinfo
    if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
        error("ERROR resolving host");
    }

    *peer_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*peer_sock < 0)
        error("ERROR opening peer socket");

    // Configurar o endereço do peer
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    serv_addr.sin_port = htons(port);

    freeaddrinfo(res); // Liberar a memória de getaddrinfo

    printf("Tentando conectar ao peer...\n");
    if (connect(*peer_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Falha na conexão. Escutando na porta %d como peer.\n", port);

        close(*peer_sock);
        *peer_sock = configure_socket("peer-to-peer", port);

        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        int newsockfd = accept(*peer_sock, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on peer accept");

        printf("Conexão peer-to-peer estabelecida.\n");
        close(newsockfd);
    } else {
        printf("Conexão peer-to-peer estabelecida com sucesso!\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <peer_port> <client_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int peer_port = atoi(argv[1]);  // Porta para peer-to-peer (40000)
    int client_port = atoi(argv[2]);  // Porta para clientes (50000 ou 60000)

    // Conexão peer-to-peer
    int peer_sock;
    connect_peer_or_listen(&peer_sock, "127.0.0.1", peer_port);

    // Configurar socket para clientes
    int client_sock = configure_socket("user/location", client_port);

    // Loop para aceitar conexões de clientes
    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        int newsockfd = accept(client_sock, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        printf("Conexão com cliente estabelecida na porta %d.\n", client_port);

        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);

        // Receber mensagem do cliente
        if (recv(newsockfd, buffer, BUFFER_SIZE - 1, 0) < 0)
            error("ERROR reading from client socket");

        printf("Mensagem do cliente: %s\n", buffer);

        // Enviar resposta
        const char *response = "Mensagem processada com sucesso!";
        if (send(newsockfd, response, strlen(response), 0) < 0)
            error("ERROR writing to client socket");

        close(newsockfd);
    }

    close(peer_sock);
    close(client_sock);
    return EXIT_SUCCESS;
}
