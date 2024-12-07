#include "common.h"

#define MAX_CLIENTS 10
#define MAX_USERS 30

int clients_sockets[MAX_CLIENTS] = {0};            // pos é LOC, num é SOCKET
int clients_loc_and_id[MAX_CLIENTS] = {0};         // pos é LOC, num é ID
int client_id;               // ID do cliente inicial (aleatório)
int client_count = 0;                              // Contador de clientes conectados
int user_ids[MAX_USERS] = {0};                     // IDs dos usuários
int user_special[MAX_USERS] = {0};                 // 0, 1 (especialidade do usuário)
int user_location[MAX_USERS] = {0};                // Localização do usuário (de -1, 1 a 10)

int create_socket(int port) {
    int server_socket;
    struct sockaddr_in server_addr;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        error("Socket creation failed");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        error("Bind failed");

    if (listen(server_socket, MAX_CLIENTS) < 0)
        error("Listen failed");

    return server_socket;
}

void accept_new_client(int socket) {
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int new_client_socket = accept(socket, (struct sockaddr *)&client_addr, &addrlen);

    if (new_client_socket < 0){
        perror("Error accepting connection");
        return;
    }

    if (client_count >= MAX_CLIENTS) {
        send_message(new_client_socket, ERROR, "09");
        printf("Client limit exceeded\n");
        close(new_client_socket);
        return;
    }

    // Read message
    Message msg;
    process_message(new_client_socket, &msg);

    int digit = atoi(msg.payload); 
    clients_sockets[digit] = new_client_socket;
    clients_loc_and_id[digit] = client_id;
    client_count++;
    printf("Client [%d] added (%d)\n", client_id,digit);

    // Envia a resposta de conexão com o ID do cliente
    char res_conn_msg[BUFFER_SIZE];
    snprintf(res_conn_msg, sizeof(res_conn_msg), "%d", client_id);
    send_message(new_client_socket, RES_CONN, res_conn_msg);
    
    client_id++;
}

void process_client_messages(fd_set *read_fds) {
    Message msg; // Variável para armazenar a mensagem recebida

    for (int i = 0; i < MAX_CLIENTS; i++) {
        int sd = clients_sockets[i];
        if (FD_ISSET(sd, read_fds)) {
            int valread = read(sd, &msg, sizeof(msg));  // Ler a estrutura Message
            if (valread == 0) {
                printf("Client [%d] disconnected\n", clients_loc_and_id[i]);
                close(sd);
                clients_sockets[i] = 0;
                clients_loc_and_id[i] = 0;
                client_count--;
            } else {
                printf("Message from client %d: Code=%d, Payload=%s\n", sd, msg.code, msg.payload);

                // Processar a mensagem de acordo com o código
                switch (msg.code) {
                    case REQ_CONN:
                        // Responder com uma mensagem de sucesso para a conexão
                        snprintf(msg.payload, sizeof(msg.payload), "Connection request received!");
                        send(sd, &msg, sizeof(msg), 0);
                        break;
                    default:
                        // Caso o código seja desconhecido, enviar uma mensagem de erro
                        snprintf(msg.payload, sizeof(msg.payload), "Unknown request");
                        send(sd, &msg, sizeof(msg), 0);
                        break;
                }
            }
        }
    }
}
int main(int argc, char *argv[]) {
    // Initial config
    if (argc != 3) {
        fprintf(stderr, "usage: %s <peer_port> <client_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    //int peer_port = atoi(argv[1]);  // Porta para peer-to-peer (40000)
    int client_port = atoi(argv[2]);  // Porta para clientes (50000 ou 60000)

    // Socket for client connection
    client_id = (rand() % 100) + 10;
    int client_socket = create_socket(client_port);

    fd_set read_fds;
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(client_socket, &read_fds);
        int max_sd = client_socket;

        // Add active clients to the fd_set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &read_fds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        int activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR)
            error("Error in select");

        // Handle new client connection
        if (FD_ISSET(client_socket, &read_fds)) {
            accept_new_client(client_socket);
        }

        // Handle client messages
        process_client_messages(&read_fds);
    }

    close(client_socket);
    //close(peer_socket);

    return 0;
}