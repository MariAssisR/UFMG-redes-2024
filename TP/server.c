#include "common.h"

#define MAX_CLIENTS 10
#define MAX_USERS 30

int clients_sockets[MAX_CLIENTS] = {0};            // pos é LOC, num é SOCKET
int clients_loc_and_id[MAX_CLIENTS] = {0};         // pos é LOC, num é ID
int client_id;                                     // ID do cliente inicial (aleatório)
int client_count = 0;                              // Contador de clientes conectados
int user_ids[MAX_USERS] = {0};                     // IDs dos usuários
int user_special[MAX_USERS] = {0};                 // 0 ou 1 (especialidade do usuário)
int user_location[MAX_USERS] = {0};                // Localização do usuário (de -1, 1 a 10)
int user_count = 0; 

int peer_id = -1;                                       // ID do peer que vai ser conectado
int this_peer_id;
enum peer_connection_state { DISC = 0, CONN = 1 };
enum peer_connection_state peer_state;

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
    int digit, msg_peer_id;
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

        case REQ_CONNPEER:
            if(peer_id == -1){ 
                // not connected to other peer
                peer_id = (rand() % 100) + 10;
                printf("Peer %d connected\n", peer_id); 

                char peer_id_str[BUFFER_SIZE];
                snprintf(peer_id_str, sizeof(peer_id_str), "%d", peer_id);
                send_message(socket, RES_CONNPEER, peer_id_str);
                break;
            }
            send_message(socket, ERROR, "1");
            break;

        case RES_CONNPEER:
            msg_peer_id = atoi(msg->payload); 
            this_peer_id = msg_peer_id;
            printf("New Peer ID: %d\n", this_peer_id);

            if(peer_id == -1){
                peer_id = (rand() % 100) + 10;
                printf("Peer %d connected\n", peer_id); 
            }
            break;

        case REQ_DISCPEER:
            msg_peer_id = atoi(msg->payload); 
            if (msg_peer_id == peer_id){
                printf("Peer %d disconnected\n", peer_id);
                peer_id++; // change id of future new peer
                send_message(socket, OK, "1");
            }
            else {
                send_message(socket, ERROR, "2");
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

void send_peer_messages(int peer_socket, fd_set *read_fds) {
    Message msg;
    memset(&msg, 0, sizeof(msg));
    
    if (FD_ISSET(STDIN_FILENO, read_fds)) { 
        if (fgets(msg.payload, sizeof(msg.payload), stdin) == NULL) {
            perror("Erro ao ler entrada");
            return;
        }
        msg.payload[strcspn(msg.payload, "\n")] = '\0';
        msg.code = 1;

        char *command = strtok(msg.payload, " ");
        if (strcmp( command, "kill") != 0){
            printf("Wrong command, option: kill\n");
            return;
        }

        char this_peer_id_str[BUFFER_SIZE];
        snprintf(this_peer_id_str, sizeof(this_peer_id_str), "%d", this_peer_id);
        send_message(peer_socket, REQ_DISCPEER, this_peer_id_str);
        // Read message
        Message msg;
        int valread = read(peer_socket, &msg, sizeof(msg));
        if (valread <= 0) {
            perror("Error on read client message");
            close(peer_socket);
            return;
        }
        server_process_message(peer_socket, &msg);  
        exit(EXIT_SUCCESS);
    }
}

void handle_peer_messages(int peer_socket) {
    Message msg;
    int valread = read(peer_socket, &msg, sizeof(msg));

    if (valread == 0) {
        error("Erro igual a 0 ao ler do socket");
    } else if (valread < 0)
        perror("Erro ao ler do socket");
    else
        server_process_message(peer_socket, &msg);   
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

int create_socket(int port, int client){
    int sock;
    struct sockaddr_in server_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        error("Socket creation failed");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        close(sock);
        error("setsockopt failed");
    }

    if(client){
        if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
            close(sock);
            error("Bind failed");
        }

        if (listen(sock, MAX_CLIENTS) < 0){
            close(sock);
            error("Listen failed");
        }
    }
    return sock;
}

int create_passive_connection(int port){
    int socket = create_socket(port, 0);
    if (socket < 0) return -1;

    struct sockaddr_in server_addr;
    int addrlen = sizeof(server_addr);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        error("Bind failed");
        close(socket);
        return -1;
    }

    printf("No peer found, starting to listen...\n");
    if (listen(socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(socket);
        return -1;
    }

    // Accept new connections
    int new_socket;
    if ((new_socket = accept(socket, (struct sockaddr*)&server_addr, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        close(socket);
        return -1;
    }
    return new_socket;
}

int create_active_connection(int port, int socket){
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0)
        return 0;
    return -1;
}

int main(int argc, char *argv[]) {
    // Initial config
    if (argc != 3) {
        fprintf(stderr, "usage: %s <peer_port> <client_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int peer_port = atoi(argv[1]);  // Porta para peer-to-peer (40000)
    int client_port = atoi(argv[2]);  // Porta para clientes (50000 ou 60000)

    // Random ids
    srand(time(NULL));
    client_id = (rand() % 100) + 10;

    // Socket for connection
    int client_socket = create_socket(client_port, 1);
    int peer_socket = create_socket(peer_port, 0);

    peer_state = DISC;

    fd_set read_fds, client_read_fds;
    while (1) {
        if(peer_state == DISC){
            if(create_active_connection(peer_port,peer_socket) == 0){
                peer_state = CONN;

                send_message(peer_socket, REQ_CONNPEER, "");
                
                Message msg = read_message(peer_socket);
                server_process_message(peer_socket, &msg);

                
                char peer_id_str[BUFFER_SIZE];
                snprintf(peer_id_str, sizeof(peer_id_str), "%d", peer_id);
                send_message(peer_socket, RES_CONNPEER, peer_id_str);
                
                continue;
            }
            else {
                close(peer_socket);
                peer_socket = create_passive_connection(peer_port);
                if(peer_socket < 0)
                    continue;
                
                peer_state = CONN;

                Message msg;
                int valread = read(peer_socket, &msg, sizeof(msg));
                if (valread <= 0) {
                    close(peer_socket);
                    perror("Error on read peer message");
                }
                server_process_message(peer_socket, &msg);

                valread = read(peer_socket, &msg, sizeof(msg));
                if (valread <= 0) {
                    close(peer_socket);
                    perror("Error on read peer message");
                }
                server_process_message(peer_socket, &msg);
                
                continue;
            }
        }
        else {
            FD_ZERO(&read_fds);
            FD_ZERO(&client_read_fds);
            FD_SET(client_socket, &read_fds);
            FD_SET(peer_socket, &read_fds);
            FD_SET(STDIN_FILENO, &read_fds);

            int max_sd = MAX(MAX(client_socket, peer_socket), STDIN_FILENO);

            // Add active clients to the fd_set
            for (int i = 0; i < MAX_CLIENTS; i++) {
                int sd = clients_sockets[i];
                if (sd > 0) {
                    FD_SET(sd, &read_fds);
                    FD_SET(sd, &client_read_fds);
                    max_sd = MAX(max_sd, sd);
                }
            }

            int activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);
            if (activity < 0 && errno != EINTR)
                error("Error in select");

            // Handle new client connection
            if (FD_ISSET(client_socket, &read_fds)) {
                accept_new_client(client_socket);
            }
            
            // Send message to server
            if (FD_ISSET(STDIN_FILENO, &read_fds)) {
                send_peer_messages(peer_socket, &read_fds);
            }

            // Handle server messages
            if (FD_ISSET(peer_socket, &read_fds)) {
                handle_peer_messages(peer_socket);
            }

            // Handle client messages
            handle_client_messages(&client_read_fds);
        }
    }

    close(client_socket);
    close(peer_socket);

    return 0;
}