#include "common.h"

int location_id;
int client_user_id = -1;
int client_loc_id = -1;

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
                    printf("New user added: ");
                    break;
                case 3:
                    printf("User updated: ");
                    break;
                default: 
                    printf("OK message received in client: %s\n", msg->payload);
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
                    printf("ERROR message received in client: %s\n", msg->payload);
                    break;
            }
        }
            break;

        case RES_CONN:
            int client_id = atoi(msg->payload);
            if(client_user_id == -1){
                printf("SU New ID: %d\n", client_id);
                client_user_id = client_id;
            }
            else {
                printf("SL New ID: %d\n", client_id);
                client_loc_id = client_id;
            }
            break;
        
        case RES_USRLOC:
            int current_loc_id = atoi(msg->payload);
            printf("Current location: %d\n", current_loc_id);
            break;

        case RES_USRACCESS:
            int loc_id = atoi(msg->payload);
            printf("OK, Last location: %d\n", loc_id);
            break;

        case RES_LOCLIST:
            char* loc_list = msg->payload;
            printf(" List of people at the specified location:: %s\n", loc_list);
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
    send_message_with_int_payload(user_socket, REQ_CONN, location_id);
    send_message_with_int_payload(location_socket, REQ_CONN, location_id);
    
    Message user_msg = read_message(user_socket);
    client_process_message(&user_msg);
    
    Message location_msg = read_message(location_socket);
    client_process_message(&location_msg);
}

void handle_command_loop(int user_socket, int location_socket) {
    Message msg; 
    Message response_msg; 
    Message msg_to_send;

    while (1) {
        memset(&msg, 0, sizeof(msg));

        fprintf(stderr, "> ");
        if (fgets(msg.payload, sizeof(msg.payload), stdin) == NULL) 
            continue;
        msg.payload[strcspn(msg.payload, "\n")] = '\0';
        msg.code = 1;

        char *command = strtok(msg.payload, " ");
        char *payload = command ? command + strlen(command) + 1 : NULL;
        if (payload && payload[0] == ' ')
            payload++;

        if (strcmp( command, "kill") != 0 && 
        strcmp( command, "add") != 0 &&
        strcmp( command, "find") != 0 &&
        strcmp( command, "in") != 0 && 
        strcmp( command, "out") != 0 && 
        strcmp( command, "inspect") != 0)
        {
            fprintf(stderr, "Wrong command, options: kill, add, find, in, out, inspect\n");
            continue;
        }

        int sockets[] = {user_socket, location_socket};
        char *sockets_name[] = {"SU", "SL"};
        int locs[] = {client_user_id, client_loc_id};
        for (int i = 0; i < 2; i++) {
            if (strcmp(command, "kill") == 0) {
                send_message_with_int_payload(sockets[i], REQ_DISC, locs[i]);
                
                response_msg = read_message(sockets[i]);
                printf("%s ", sockets_name[i]);
                client_process_message(&response_msg);

                close(sockets[i]);
                if(i == 1)
                    exit(EXIT_SUCCESS);
            }
            else if (strcmp(command, "add") == 0) {
                if(i == 1)
                    continue;

                char uid[11];
                int number;
                if (payload == NULL || sscanf(payload, "%10s %d", uid, &number) != 2) {
                    fprintf(stderr, "The 'add' command requires a 10-digit UID and an boolean value. Example: add 2021808080 0\n");
                    continue;
                }
                if (strlen(uid) != 10 || strspn(uid, "0123456789") != 10) {
                    fprintf(stderr, "Invalid UID. Must contain exactly 10 numeric digits.\n");
                    continue;
                }
                snprintf(msg_to_send.payload, sizeof(msg_to_send.payload), "%s,%d", uid, number);

                send_message(sockets[i], REQ_USRADD, msg_to_send.payload);
                response_msg = read_message(sockets[i]);
                client_process_message(&response_msg);

                if(response_msg.code != ERROR)
                    printf("%s\n",uid);
            }
            else if (strcmp(command, "find") == 0) {
                if (i == 0)
                    continue;

                if (payload == NULL || strlen(payload) != 10 || strspn(payload, "0123456789") != 10) {
                    fprintf(stderr, "The 'find' command requires a 10-digit UID. Example: find 2021808080\n");
                    continue;
                }
                snprintf(msg_to_send.payload, sizeof(msg_to_send.payload), "%s", payload);
                
                send_message(sockets[i], REQ_USRLOC, msg_to_send.payload);
                response_msg = read_message(sockets[i]);
                client_process_message(&response_msg);
            }
            else if (strcmp(command, "in") == 0) {
                if(i == 1)
                    continue;
                
                if (payload == NULL || strlen(payload) != 10 || strspn(payload, "0123456789") != 10) {
                    fprintf(stderr, "The 'in' command requires a 10-digit UID. Example: in 2021808080\n");
                    continue;
                }
                snprintf(msg_to_send.payload, sizeof(msg_to_send.payload), "%s,in,%d", payload,location_id);
                
                send_message(sockets[i], REQ_USRACCESS, msg_to_send.payload);
                response_msg = read_message(sockets[i]);
                client_process_message(&response_msg);
            }
            else if (strcmp(command, "out") == 0) {
                if(i == 1)
                    continue;
                
                if (payload == NULL || strlen(payload) != 10 || strspn(payload, "0123456789") != 10) {
                    fprintf(stderr, "The 'out' command requires a 10-digit UID. Example: out 2021808080\n");
                    continue;
                }
                snprintf(msg_to_send.payload, sizeof(msg_to_send.payload), "%s,out,%d", payload,location_id);
                
                send_message(sockets[i], REQ_USRACCESS, msg_to_send.payload);
                response_msg = read_message(sockets[i]);
                client_process_message(&response_msg);
            }
            else if (strcmp(command, "inspect") == 0) {
                if(i == 0)
                    continue;

                char uid[11];
                int locId;
                if (payload == NULL || sscanf(payload, "%10s %d", uid, &locId) != 2) {
                    fprintf(stderr, "The 'inspect' command requires a 10-digit UID and a location id. Example: inspect 2021808080 2\n");
                    continue;
                }
                if (strlen(uid) != 10 || strspn(uid, "0123456789") != 10) {
                    fprintf(stderr, "Invalid UID. Must contain exactly 10 numeric digits.\n");
                    continue;
                }
                snprintf(msg_to_send.payload, sizeof(msg_to_send.payload), "%s,%d", uid, locId);

                send_message(sockets[i], REQ_LOCLIST, msg_to_send.payload);
                response_msg = read_message(sockets[i]);
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

    int user_socket = connect_to_server(server_ip, user_server_port);
    int location_socket = connect_to_server(server_ip, location_server_port);

    handle_initial_registration(user_socket, location_socket);

    handle_command_loop(user_socket, location_socket);

    close(user_socket);
    close(location_socket);

    return 0;
}
