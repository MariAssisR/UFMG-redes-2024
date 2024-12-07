#include "common.h"

#define MAX_CLIENTS 10
#define MAX_USERS 30

int clients_sockets[MAX_CLIENTS] = {0};            // pos é LOC, num é SOCKET
int clients_loc_and_id[MAX_CLIENTS] = {0};         // pos é LOC, num é ID
int client_id;                                     // ID do cliente inicial (aleatório)
int client_count = 0;                              // Contador de clientes conectados
int user_ids[MAX_USERS] = {0};                     // IDs dos usuários
int user_special[MAX_USERS] = {0};                 // 0, 1 (especialidade do usuário)
int user_location[MAX_USERS] = {0};                // Localização do usuário (de -1, 1 a 10)
int user_count = 0; 

int user_find_pos(){
    if(user_count < MAX_USERS){
        for (int i = 0; i < MAX_USERS; i++){
            if (user_ids[i] == 0)
                return i;
        }
    }
    perror("Client limit exceeded\n");
    return -1;
}

int user_exists_and_pos(int id){
    for (int i = 0; i < MAX_USERS; i++){
        if (user_ids[i] == id)
            return i;
    }
    return -1;
}

int client_exists_and_pos(int id){
    for (int i = 0; i < MAX_CLIENTS; i++){
        if (clients_loc_and_id[i] == id)
            return i;
    }
    return -1;
}

void client_remove(int loc_id){
    close(clients_sockets[loc_id]);
    clients_sockets[loc_id] = 0;
    clients_loc_and_id[loc_id] = 0;
    client_count--;
}

void server_process_message(int socket, Message *msg) {
    int digit;
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

        case REQ_CONN:
            digit = atoi(msg->payload); 
            clients_sockets[digit] = socket;
            clients_loc_and_id[digit] = client_id;
            client_count++;
            printf("Client %d added (Loc %d)\n", client_id,digit);

            // Envia a resposta de conexão com o ID do cliente
            char res_conn_msg[BUFFER_SIZE];
            snprintf(res_conn_msg, sizeof(res_conn_msg), "%d", client_id);
            send_message(socket, RES_CONN, res_conn_msg);
            
            client_id++;

            break;

        case REQ_DISC:
            digit = atoi(msg->payload); 
            int client_loc = client_exists_and_pos(digit);
            if(client_loc != -1){
                printf("Client %d removed (Loc %d)\n", digit, client_loc);
                send_message(socket, OK, "1");
                client_remove(client_loc);
            }
            else{
                send_message(socket, ERROR, "10");
            }
            break;

        case REQ_USRADD:
            int uid_add = atoi(strtok(msg->payload, ","));
            digit = atoi(strtok(NULL, ","));

            int user_pos_add = user_exists_and_pos(uid_add);
            if(user_pos_add != -1){
                user_special[user_pos_add] = digit;
                send_message(socket, OK, "03");
            }
            else{
                if(user_count >= MAX_USERS)
                    send_message(socket, ERROR, "17");

                int user_new_pos_add = user_find_pos();
                user_ids[user_new_pos_add] = uid_add;
                user_special[user_new_pos_add] = digit;
                user_count++;
                send_message(socket, OK, "02");
            }
            break;
            
        case REQ_USRLOC:
            int uid_find = atoi(msg->payload);
            int user_pos_find = user_exists_and_pos(uid_find);

            if(user_pos_find != -1){
                char res_conn_msg[BUFFER_SIZE];
                snprintf(res_conn_msg, sizeof(res_conn_msg), "%d", user_location[user_pos_find]);
                send_message(socket, RES_USRLOC, res_conn_msg);
            }
            else{
                send_message(socket, ERROR, "18");
            }
            break;

        default:
            // Caso a mensagem tenha um código desconhecido
            printf("Mensagem desconhecida recebida: %s\n", msg->payload);
            break;
    }
}

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
    int valread = read(new_client_socket, &msg, sizeof(msg));
    if (valread <= 0) {
        perror("Error on read client message");
        close(socket);
        return;
    }
    server_process_message(new_client_socket, &msg);
}

void handle_client_messages(fd_set *read_fds) {
    Message msg; // Variável para armazenar a mensagem recebida

    for (int i = 0; i < MAX_CLIENTS; i++) {
        int sd = clients_sockets[i];
        if (FD_ISSET(sd, read_fds)) {
            int valread = read(sd, &msg, sizeof(msg));  // Ler a estrutura Message
            if (valread == 0) {
                printf("Client %d disconnected\n", clients_loc_and_id[i]);
                client_remove(i);
            } else {
                server_process_message(sd, &msg);
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
    srand(time(NULL));
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
        handle_client_messages(&read_fds);
    }

    close(client_socket);
    //close(peer_socket);

    return 0;
}