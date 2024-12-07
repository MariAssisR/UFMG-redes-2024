#include "common.h"

int connect_to_server(const char *server_ip, int port) {
    int socket_fd;
    struct sockaddr_in server_addr;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error("Socket creation failed");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
        error("Invalid address");

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        error("Connection failed");

    return socket_fd;
}

void handle_initial_registration(int user_socket, int location_socket, int location_id) {
    char location_str[BUFFER_SIZE];
    snprintf(location_str, sizeof(location_str), "%d", location_id);
    send_message(user_socket, REQ_CONN, location_str);
    send_message(location_socket, REQ_CONN, location_str);

    Message user_msg;
    Message location_msg;
    process_message(user_socket, &user_msg);
    process_message(location_socket, &location_msg);

    printf("SU New ID: %s\n", user_msg.payload);
    printf("SL New ID: %s\n", location_msg.payload);
}

void handle_command_loop(int user_socket, int location_socket, int location_id) {
    Message msg;  // Variável para armazenar a mensagem a ser enviada
    Message response_msg;  // Variável para armazenar a resposta recebida
    char response[BUFFER_SIZE];

    while (1) {
        memset(&msg, 0, sizeof(msg));  // Limpar a mensagem

        // Ler mensagem do terminal
        printf("> ");
        if (fgets(msg.payload, sizeof(msg.payload), stdin) == NULL) {
            perror("Erro ao ler entrada");
            break;
        }

        // Remover o '\n' no final da mensagem
        msg.payload[strcspn(msg.payload, "\n")] = '\0';

        // Verificar comando para sair
        if (strcmp(msg.payload, "exit") == 0) {
            printf("Encerrando comunicação.\n");
            break;
        }

        // Definir o código da mensagem
        msg.code = REQ_CONN;  // Defina o código conforme a necessidade, por exemplo, REQ_CONN

        // Enviar a mensagem para ambos os servidores
        int sockets[] = {user_socket, location_socket};
        for (int i = 0; i < 2; i++) {
            // Enviar a mensagem
            if (send(sockets[i], &msg, sizeof(msg), 0) < 0) {
                perror("Erro ao enviar mensagem");
                break;
            }

            // Limpar a resposta
            memset(&response_msg, 0, sizeof(response_msg));

            // Receber a resposta
            int bytes_received = recv(sockets[i], &response_msg, sizeof(response_msg), 0);
            if (bytes_received < 0) {
                perror("Erro ao receber resposta");
                break;
            } else if (bytes_received == 0) {
                printf("Servidor %d desconectado.\n", i + 1);
                break;
            }

            // Exibir resposta do servidor
            printf("Resposta do servidor %d: Code=%d, Payload=%s\n", i + 1, response_msg.code, response_msg.payload);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <server_ip> <user_server_port> <location_server_port> <location_id>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    int user_server_port = atoi(argv[2]);
    int location_server_port = atoi(argv[3]);
    int location_id = atoi(argv[4]);

    if (location_id < 1 || location_id > 10)
        error("Invalid argument\n");

    // Connect to servers
    int user_socket = connect_to_server(server_ip, user_server_port);
    int location_socket = connect_to_server(server_ip, location_server_port);

    // Register client to servers
    handle_initial_registration(user_socket, location_socket, location_id);

    // Loop for commands
    handle_command_loop(user_socket, location_socket, location_id);

    // Fechar conexões
    close(user_socket);
    close(location_socket);

    return 0;
}
