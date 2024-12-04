#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h> 
#include <sys/types.h>  
#include <sys/select.h> 
#include <errno.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 500

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void handle_message(int socket, const char *source);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <peer_port> <client_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int peer_port = atoi(argv[1]);  // Porta para peer-to-peer (40000)
    int client_port = atoi(argv[2]);  // Porta para clientes (50000 ou 60000)

    int peer_socket, client_socket, active_peer_socket = -1;
    struct sockaddr_in peer_addr, client_addr;

    // Socket to peer-to-peer connection
    if ((peer_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        error("Peer socket failed");

    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr.s_addr = INADDR_ANY;
    peer_addr.sin_port = htons(peer_port);

    // Try active connection mode
    int peer_connection = connect(peer_socket, (struct sockaddr*)&peer_addr, sizeof(peer_addr));
    if (peer_connection < 0) {
        printf("No peer found, starting to listen...\n");

        // passive connection mode
        if (bind(peer_socket, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) < 0)
            error("Erro ao fazer bind no peer socket");

        if (listen(peer_socket, MAX_CLIENTS) < 0) 
            error("Erro ao fazer listen no peer socket");

    } else {
        // active connection mode
        printf("Conexão peer-to-peer estabelecida (modo ativo).\n");
        active_peer_socket = peer_socket;
    }

    // Socket for client connection
    if ((client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        error("Client socket failed");

    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(client_port);

    if (bind(client_socket, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0)
        error("Client bind failed");

    if (listen(client_socket, MAX_CLIENTS) < 0)
        error("Client listen failed");

    printf("Client socket listening on port %d\n", client_port);


    fd_set read_fds;
    int max_fd = (peer_socket > client_socket) ? peer_socket : client_socket;

     while (1) {
        FD_ZERO(&read_fds);
        if (active_peer_socket > 0)
            FD_SET(active_peer_socket, &read_fds);
        else
            FD_SET(peer_socket, &read_fds);
        FD_SET(client_socket, &read_fds);

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) 
            error("Erro no select");

        // Verifica atividade no socket peer-to-peer
        if (FD_ISSET(peer_socket, &read_fds) && active_peer_socket < 0) {
            int new_peer_socket = accept(peer_socket, NULL, NULL);
            if (new_peer_socket > 0) {
                printf("Conexão recebida do peer.\n");
                active_peer_socket = new_peer_socket;
            }
        } else if (FD_ISSET(active_peer_socket, &read_fds)) {
            handle_message(active_peer_socket, "Peer");
        }

        // Verifica atividade no socket de clientes
        if (FD_ISSET(client_socket, &read_fds)) {
            int new_client_socket = accept(client_socket, NULL, NULL);
            if (new_client_socket > 0) {
                printf("Conexão recebida de cliente.\n");
                handle_message(new_client_socket, "Client");
            }
        }
    
    }

    close(peer_socket);
    close(client_socket);
    return 0;
}

void handle_message(int socket, const char *source) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    ssize_t bytes_read = read(socket, buffer, BUFFER_SIZE);
    if (bytes_read > 0) {
        printf("[%s] Mensagem recebida: %s\n", source, buffer);
        send(socket, "Mensagem recebida.", 18, 0);
    } else if (bytes_read == 0) {
        printf("[%s] Conexão fechada pelo cliente.\n", source);
        close(socket);
    } else {
        error("Erro na leitura");
    }
}