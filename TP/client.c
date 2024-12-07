#include "common.h"

int location_id;
int client_user_id;
int client_loc_id;

void client_process_message(int socket, Message *msg) {
    switch (msg->code) {
        case OK:
        {
            int digit = atoi(msg->payload); 
            switch (digit) {
                case 1:
                    printf("Successful disconnect\n");
                    break;
                case 2:
                    printf("Successful create\n");
                    break;
                case 3:
                    printf("Successful update\n");
                    break;
                default: 
                    printf("Mensagem OK recebida: %s\n", msg->payload);
                    break;
            }
        }
            break;

        case ERROR:
        {
            int digit = atoi(msg->payload); 
            switch (digit) {
                case 1:
                    printf("Peer limit exceeded\n");
                    break;
                case 2:
                    printf("Peer not found\n");
                    break;
                case 9:
                    printf("Client limit exceeded\n");
                    break;
                case 10:
                    printf("Client not found\n");
                    break;
                case 17:
                    printf("User limit exceeded\n");
                    break;
                case 18:
                    printf("User not found\n");
                    break;
                case 19:
                    printf("Permission denied\n");
                    break;
                default: 
                    printf("Mensagem de erro recebida: %s\n", msg->payload);
                    break;
            }
            exit(EXIT_FAILURE);
        }
            break;

        default:
            // Caso a mensagem tenha um código desconhecido
            printf("Mensagem desconhecida recebida: %s\n", msg->payload);
            break;
    }
}

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

void handle_initial_registration(int user_socket, int location_socket) {
    char location_str[BUFFER_SIZE];
    snprintf(location_str, sizeof(location_str), "%d", location_id);
    send_message(user_socket, REQ_CONN, location_str);
    send_message(location_socket, REQ_CONN, location_str);

    Message user_msg;
    Message location_msg;
    int valread = read(user_socket, &user_msg, sizeof(user_msg));
    if (valread <= 0) {
        perror("Error on read client message");
        close(socket);
        return;
    }
    client_process_message(user_socket, &user_msg);
    valread = read(location_socket, &location_msg, sizeof(location_msg));
    if (valread <= 0) {
        perror("Error on read client message");
        close(socket);
        return;
    }
    client_process_message(location_socket, &location_msg);

    printf("SU New ID: %s\n", user_msg.payload);
    printf("SL New ID: %s\n", location_msg.payload);
    client_user_id = atoi(user_msg.payload);
    client_loc_id = atoi(location_msg.payload);
}

void handle_command_loop(int user_socket, int location_socket) {
    Message msg; 
    Message response_msg; 

    while (1) {
        memset(&msg, 0, sizeof(msg));

        printf("> ");
        if (fgets(msg.payload, sizeof(msg.payload), stdin) == NULL) {
            perror("Erro ao ler entrada");
            break;
        }
        msg.payload[strcspn(msg.payload, "\n")] = '\0';
        msg.code = 1;

        if (strcmp(msg.payload, "kill") != 0 && 
        strcmp(msg.payload, "add") != 0 && 
        strcmp(msg.payload, "find") != 0 && 
        strcmp(msg.payload, "in") != 0 && 
        strcmp(msg.payload, "out") != 0 && 
        strcmp(msg.payload, "inspect") != 0){
            printf("Wrong command, options: kill, add, find, in, out, inspect\n");
            continue;
        }

        int sockets[] = {user_socket, location_socket};
        int locs[] = {client_user_id, client_loc_id};
        for (int i = 0; i < 2; i++) {
            Message msg_to_send = msg;
            if (strcmp(msg_to_send.payload, "kill") == 0) {
                msg_to_send.code = REQ_DISC;
                char id[BUFFER_SIZE];
                snprintf(id, sizeof(id), "%d", locs[i]);
                strncpy(msg_to_send.payload, id, sizeof(msg_to_send.payload) - 1);
                msg_to_send.payload[sizeof(msg_to_send.payload) - 1] = '\0';

                send_message(sockets[i], msg_to_send.code, msg_to_send.payload);

                memset(&response_msg, 0, sizeof(response_msg));
                client_process_message(sockets[i], &response_msg);

                printf("SU Successful disconnect\n");
                close(sockets[i]);
                if(i == 1)
                    exit(EXIT_SUCCESS);;
            }
            else if (strcmp(msg_to_send.payload, "add") == 0) {
                msg_to_send.code = REQ_USRADD;
                char id[BUFFER_SIZE];
                snprintf(id, sizeof(id), "%d", locs[i]);
                strncpy(msg_to_send.payload, id, sizeof(msg_to_send.payload) - 1);
                msg_to_send.payload[sizeof(msg_to_send.payload) - 1] = '\0';

                send_message(sockets[i], msg_to_send.code, msg_to_send.payload);

                memset(&response_msg, 0, sizeof(response_msg));
                client_process_message(sockets[i], &response_msg);

                printf("SU Successful disconnect\n");
                close(sockets[i]);
                if(i == 1)
                    exit(EXIT_SUCCESS);;
            }
            else if (strcmp(msg_to_send.payload, "find") == 0) {
                msg_to_send.code = REQ_USRLOC;
                char id[BUFFER_SIZE];
                snprintf(id, sizeof(id), "%d", locs[i]);
                strncpy(msg_to_send.payload, id, sizeof(msg_to_send.payload) - 1);
                msg_to_send.payload[sizeof(msg_to_send.payload) - 1] = '\0';

                send_message(sockets[i], msg_to_send.code, msg_to_send.payload);

                memset(&response_msg, 0, sizeof(response_msg));
                client_process_message(sockets[i], &response_msg);

                printf("SU Successful disconnect\n");
                close(sockets[i]);
                if(i == 1)
                    exit(EXIT_SUCCESS);;
            }
            else{
                send_message(sockets[i], msg_to_send.code, msg_to_send.payload);

                memset(&response_msg, 0, sizeof(response_msg));
                client_process_message(sockets[i], &response_msg);
            }
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
    location_id = atoi(argv[4]);

    if (location_id < 1 || location_id > 10)
        error("Invalid argument\n");

    // Connect to servers
    int user_socket = connect_to_server(server_ip, user_server_port);
    int location_socket = connect_to_server(server_ip, location_server_port);

    // Register client to servers
    handle_initial_registration(user_socket, location_socket);

    // Loop for commands
    handle_command_loop(user_socket, location_socket);

    // Fechar conexões
    close(user_socket);
    close(location_socket);

    return 0;
}
