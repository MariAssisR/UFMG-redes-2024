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
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    while (1) {
        // memset(buffer, 0, BUFFER_SIZE);
        // memset(user_response, 0, BUFFER_SIZE);
        // memset(location_response, 0, BUFFER_SIZE);

        // printf("Enter a command to send (or 'kill' to disconnect): ");
        // fgets(buffer, BUFFER_SIZE, stdin);

        // if (strcmp(buffer, "kill") == 0) {
        //     handle_disconnection(user_socket, location_socket, location_id);
        //     break;
        // }

        // char *buf2 = buffer;
        // send_message_2(user_socket, buf2);
        // send_message_2(location_socket, buffer);

        // printf("aqui");
        // receive_response(user_socket, user_response);
        // receive_response(location_socket, location_response);

        // printf("SU Response: %s\n", user_response);
        // printf("SL Response: %s\n", location_response);

        // Limpar buffer de entrada
        memset(buffer, 0, BUFFER_SIZE);

        // Ler mensagem do terminal
        printf("> ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            perror("Erro ao ler entrada");
            break;
        }

        // Remover o '\n' no final da mensagem
        buffer[strcspn(buffer, "\n")] = '\0';
        printf("1\n");

        // Verificar comando para sair
        if (strcmp(buffer, "exit") == 0) {
            printf("Encerrando comunicação.\n");
            break;
        }
        printf("2\n");

        // Enviar a mensagem para ambos os servidores
        int sockets[] = {user_socket, location_socket};
        for (int i = 0; i < 2; i++) {
            printf("3\n");
            if (send(sockets[i], buffer, strlen(buffer), 0) < 0) {
                perror("Erro ao enviar mensagem");
                break;
            }
            printf("4\n");

            // Receber resposta do servidor atual
            printf("5\n");
            memset(response, 0, BUFFER_SIZE);
            int bytes_received = recv(sockets[i], response, BUFFER_SIZE - 1, 0);
            if (bytes_received < 0) {
                perror("Erro ao receber resposta");
                break;
            } else if (bytes_received == 0) {
                printf("Servidor %d desconectado.\n", i + 1);
                break;
            }
            printf("6\n");

            // Exibir resposta do servidor
            response[bytes_received] = '\0';
            printf("Servidor %d: %s\n", i + 1, response);
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
        error("Invalid location ID. Must be between 1 and 10.\n");

    // Conectar aos servidores
    int user_socket = connect_to_server(server_ip, user_server_port);
    printf("Connected to user server at %s:%d\n", server_ip, user_server_port);

    int location_socket = connect_to_server(server_ip, location_server_port);
    printf("Connected to location server at %s:%d\n", server_ip, location_server_port);

    // Registrar cliente nos servidores
    handle_initial_registration(user_socket, location_socket, location_id);

    // Iniciar loop de envio de comandos
    handle_command_loop(user_socket, location_socket, location_id);

    // Fechar conexões
    close(user_socket);
    close(location_socket);

    return 0;
}
