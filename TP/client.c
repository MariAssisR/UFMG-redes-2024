#include "common.h"

int location_id;
int client_user_id;
int client_loc_id;

void client_process_message(Message *msg) {
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
        }
            break;

        case RES_CONN:
            //printf("Mensagem RES_CONN: %s\n", msg->payload);
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
        close(user_socket);
        return;
    }
    client_process_message(&user_msg);
    valread = read(location_socket, &location_msg, sizeof(location_msg));
    if (valread <= 0) {
        perror("Error on read client message");
        close(location_socket);
        return;
    }
    client_process_message(&location_msg);

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

        char *command = strtok(msg.payload, " ");
        char *payload = command ? command + strlen(command) + 1 : NULL;
        if (payload && payload[0] == ' ')
            payload++;

        if (strcmp( command, "kill") != 0 && 
        strcmp( command, "add") != 0 &&
        strcmp( command, "find") != 0 ) 
        //strcmp( command, "in") != 0 && 
        //strcmp( command, "out") != 0 && 
        //strcmp( command, "inspect") != 0)
        {
            printf("Wrong command, options: kill, add\n");
            continue;
        }

        int sockets[] = {user_socket, location_socket};
        char *sockets_name[] = {"SU", "SL"};
        int locs[] = {client_user_id, client_loc_id};
        for (int i = 0; i < 2; i++) {
            Message msg_to_send = msg;
            if (strcmp(command, "kill") == 0) {
                msg_to_send.code = REQ_DISC;
                snprintf(msg_to_send.payload, sizeof(msg_to_send.payload), "%d", locs[i]);

                send_message(sockets[i], msg_to_send.code, msg_to_send.payload);
                memset(&response_msg, 0, sizeof(response_msg));
                int valread = read(sockets[i], &response_msg, sizeof(response_msg));
                if (valread <= 0) {
                    perror("Error on read client message");
                    close(sockets[i]);
                    return;
                }
                printf("%s ", sockets_name[i]);
                client_process_message(&response_msg);

                close(sockets[i]);

                if(i == 1)
                    exit(EXIT_SUCCESS);
            }
            else if (strcmp(command, "add") == 0) {
                if(i == 1)
                    continue;

                msg_to_send.code = REQ_USRADD;

                char uid[11];
                int number;
                if (payload == NULL || sscanf(payload, "%10s %d", uid, &number) != 2) {
                    printf("Comando 'add' requer um UID de 10 dígitos e um número inteiro. Exemplo: add 2021808080 42\n");
                    continue;
                }
                if (strlen(uid) != 10 || strspn(uid, "0123456789") != 10) {
                    printf("UID inválido. Deve conter exatamente 10 dígitos numéricos.\n");
                    continue;
                }

                snprintf(msg_to_send.payload, sizeof(msg_to_send.payload), "%s,%d", uid, number);

                send_message(sockets[i], msg_to_send.code, msg_to_send.payload);
                memset(&response_msg, 0, sizeof(response_msg));
                int valread = read(sockets[i], &response_msg, sizeof(response_msg));
                if (valread <= 0) {
                    perror("Error on read client message");
                    close(sockets[i]);
                    return;
                }
                client_process_message(&response_msg);

                continue;
                
            }
            else if (strcmp(command, "find") == 0) {
                if (i == 0) // Enviar somente para o servidor SL (índice 1)
                    continue;

                msg_to_send.code = REQ_USRLOC;

                if (payload == NULL || strlen(payload) != 10 || strspn(payload, "0123456789") != 10) {
                    printf("Comando 'find' requer um UID de 10 dígitos numéricos. Exemplo: find 2021808080\n");
                    continue;
                }

                strncpy(msg_to_send.payload, payload, sizeof(msg_to_send.payload) - 1);
                msg_to_send.payload[sizeof(msg_to_send.payload) - 1] = '\0';

                
                send_message(sockets[i], msg_to_send.code, msg_to_send.payload);
                memset(&response_msg, 0, sizeof(response_msg));
                int valread = read(sockets[i], &response_msg, sizeof(response_msg));
                if (valread <= 0) {
                    perror("Error on read client message");
                    close(sockets[i]);
                    return;
                }
                client_process_message(&response_msg);
            }
            else if (strcmp(command, "in") == 0) {
                //DEIXA ASSIM POR ENQUANTO!
                printf("AWUI\n");
                send_message(sockets[i], msg_to_send.code, msg_to_send.payload);

                memset(&response_msg, 0, sizeof(response_msg));
                int valread = read(sockets[i], &response_msg, sizeof(response_msg));
                if (valread <= 0) {
                    perror("Error on read client message");
                    close(sockets[i]);
                    return;
                }
                client_process_message(&response_msg);
            }
            else if (strcmp(command, "out") == 0) {
                //DEIXA ASSIM POR ENQUANTO!
                send_message(sockets[i], msg_to_send.code, msg_to_send.payload);

                memset(&response_msg, 0, sizeof(response_msg));
                int valread = read(sockets[i], &response_msg, sizeof(response_msg));
                if (valread <= 0) {
                    perror("Error on read client message");
                    close(sockets[i]);
                    return;
                }
                client_process_message(&response_msg);
            }
            else if (strcmp(command, "inspect") == 0) {
                //DEIXA ASSIM POR ENQUANTO!
                send_message(sockets[i], msg_to_send.code, msg_to_send.payload);

                memset(&response_msg, 0, sizeof(response_msg));
                int valread = read(sockets[i], &response_msg, sizeof(response_msg));
                if (valread <= 0) {
                    perror("Error on read client message");
                    close(sockets[i]);
                    return;
                }
                client_process_message(&response_msg);
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
