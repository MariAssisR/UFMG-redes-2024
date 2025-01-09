#include "common.h"

#define MAX_PEERS 1
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
int this_peer_id = -1;
enum peer_connection_state { DISC = 0, CONN = 1 };
enum peer_connection_state peer_state;
int client_socket, peer_socket;

//functions
void handle_client_messages(int fd);
void handle_peer_connection(int peer_port, int *peer_socket);
void accept_new_client(int socket);
int create_socket(int port, int client);
int create_passive_connection(int port, int socket);
int create_active_connection(int port, int socket);

//aux functions
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
    int digit, msg_peer_id, client_loc;
    switch (msg->code) {
        case OK:
        {
            int digit = atoi(msg->payload); 
            switch (digit) {
                case 1:
                    close(peer_socket);
                    exit(EXIT_FAILURE);
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
                peer_id = (rand() % 100) + 10;
                printf("Peer %d connected\n", peer_id); 

                send_message_with_int_payload(socket, RES_CONNPEER, peer_id);
                break;
            }
            send_message(socket, ERROR, "1");
            break;

        case RES_CONNPEER:
            this_peer_id = atoi(msg->payload);  
            printf("New Peer ID: %d\n", this_peer_id);

            if(peer_id == -1){
                peer_id = (rand() % 100) + 10;
                printf("Peer %d connected\n", peer_id); 
                send_message_with_int_payload(socket, RES_CONNPEER, peer_id);
            }
            break;

        case REQ_DISCPEER:
            msg_peer_id = atoi(msg->payload); 
            if (msg_peer_id == peer_id){
                send_message(peer_socket, OK, "1");
                printf("Peer %d disconnected\n", peer_id);
                peer_id = -1;
                this_peer_id = -1;
                peer_state = DISC;
                sleep(1);
            }
            else 
                send_message(socket, ERROR, "2");
            
            break;

        case REQ_CONN:
            digit = atoi(msg->payload); 
            clients_sockets[digit] = socket;
            printf("%d\n", socket);
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
            client_loc = client_exists_and_pos(digit);
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
                char res_find_msg[BUFFER_SIZE];
                snprintf(res_find_msg, sizeof(res_find_msg), "%d", user_location[user_pos_find]);
                send_message(socket, RES_USRLOC, res_find_msg);
            }
            else{
                send_message(socket, ERROR, "18");
            }
            break;

        case REQ_USRACCESS:
            printf("req user access: %s\n", msg->payload);
        
            int uid_access = atoi(strtok(msg->payload, ","));
            char* direction = strtok(NULL, ",");
            client_loc = atoi(strtok(NULL, ","));

            int user_pos_access = user_exists_and_pos(uid_access);
            printf("ALOU: %d", user_pos_access);
            if(user_pos_access != -1){
                int locId = -1;
                if (strcmp(direction, "in") == 0) 
                    locId = client_loc;

                char res_access_msg[BUFFER_SIZE];
                snprintf(res_access_msg, sizeof(res_access_msg), "%d,%d,%d", uid_access, locId, client_loc);

                send_message(peer_socket, REQ_LOCREG, res_access_msg);
            }
            else{
                send_message(socket, ERROR, "18");
            }
            break;

        case REQ_LOCREG:
            printf("req loc reg: %s\n", msg->payload);

            int uid_loc = atoi(strtok(msg->payload, ","));
            int loc = atoi(strtok(NULL, ","));
            client_loc = atoi(strtok(NULL, ","));

            int user_pos_loc = user_exists_and_pos(uid_loc);
            printf("TETES: %d", user_pos_loc);
            if(user_pos_loc == -1){   
                int new_user_pos_loc = user_find_pos(uid_loc);
                user_ids[new_user_pos_loc] = uid_loc;
                user_location[new_user_pos_loc] = loc;
                char res_req_loc_msg[BUFFER_SIZE];
                snprintf(res_req_loc_msg, sizeof(res_req_loc_msg), "-1,%d", client_loc);
                send_message(peer_socket, RES_LOCREG, res_req_loc_msg);
            }
            else{
                user_location[user_pos_loc] = loc;
                char res_req_loc_msg[BUFFER_SIZE];
                snprintf(res_req_loc_msg, sizeof(res_req_loc_msg), "%d,%d", loc,client_loc);
                send_message(peer_socket, RES_LOCREG, res_req_loc_msg);
            }
                
            break;

        case RES_LOCREG:
            int loc_id = atoi(strtok(msg->payload, ","));
            client_loc = atoi(strtok(NULL, ","));

            char res_loc_msg[BUFFER_SIZE];
            snprintf(res_loc_msg, sizeof(res_loc_msg), "%d", loc_id);
            send_message(clients_sockets[client_loc], RES_USRACCESS, res_loc_msg);
                
            break;

        default:
            printf("Message with unknow code: %s\n", msg->payload);
            exit(EXIT_SUCCESS);
            break;
    }
}

void accept_new_client(int socket) {
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int new_client_socket = accept(socket, (struct sockaddr *)&client_addr, &addrlen);

    if (new_client_socket < 0){
        error("Error accepting connection");
        return;
    }

    if (client_count >= MAX_CLIENTS) {
        send_message(new_client_socket, ERROR, "09");
        printf("Client limit exceeded\n");
        close(new_client_socket);
        return;
    }
    
    Message msg = read_message(new_client_socket);
    server_process_message(new_client_socket, &msg);
}

void handle_client_messages(int fd) {
    Message msg;
    int valread = read(fd, &msg, sizeof(msg));
    if (valread == 0) {
        printf("Client %d disconnected\n", clients_loc_and_id[fd]);
        client_remove(fd);
    } else {
        server_process_message(fd, &msg);
    }
}

void handle_peer_connection(int peer_port, int *peer_socket) {
    if (peer_state == DISC) {
        // Try active connection
        if (create_active_connection(peer_port, *peer_socket) == 0) {
            peer_state = CONN;
            send_message(*peer_socket, REQ_CONNPEER, "");
        } else {
            // Fallback to passive connection
            close(*peer_socket);

            *peer_socket = create_socket(peer_port,0);
            int new_socket = create_passive_connection(peer_port, *peer_socket);
            if (new_socket >= 0) {
                close(*peer_socket);
                *peer_socket = new_socket;
                peer_state = CONN;

            }
        }
    }
}

int create_socket(int port, int client){
    int sock;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        error("Socket creation failed");

    if(client){
        struct sockaddr_in server_addr;
    
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
            error("Bind failed");

        if (listen(sock, MAX_CLIENTS) < 0)
            error("Listen failed");
    }
    
    printf("debbuger: SOCK no CREATE: %d\n", sock);
    return sock;
}

int create_passive_connection(int port, int socket){
    struct sockaddr_in server_addr;
    int addrlen = sizeof(server_addr);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        close(socket);
        error("Bind failed");
    }

    printf("No peer found, starting to listen...\n");
    if (listen(socket, MAX_PEERS) < 0) {
        close(socket);
        error("Listen failed");
    }

    int new_socket;
    if ((new_socket = accept(socket, (struct sockaddr*)&server_addr, (socklen_t*)&addrlen)) < 0) {
        close(socket);
        error("Accept");
    }
    return new_socket;
}

int create_active_connection(int port, int socket){
    struct sockaddr_in server_addr;

    printf("debbuger: port %d socket %d\n", port, socket);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (connect(socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("Connection failed");
        return -1;
    }
    return 0;
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
    client_socket = create_socket(client_port, 1);
    peer_socket = create_socket(peer_port, 0);

    peer_state = DISC;
    fd_set read_fds;
    while (1) {
        handle_peer_connection(peer_port, &peer_socket);
        
        if(peer_state == CONN){
            FD_ZERO(&read_fds);
            FD_SET(client_socket, &read_fds);
            FD_SET(peer_socket, &read_fds);
            FD_SET(STDIN_FILENO, &read_fds);

            int max_fd = MAX(MAX(client_socket, peer_socket), STDIN_FILENO);

            // Add active clients to the fd_set
            for (int i = 0; i < MAX_CLIENTS; i++) {
                int fd = clients_sockets[i];
                if (fd > 0) {
                    FD_SET(fd, &read_fds);
                    max_fd = MAX(max_fd, fd);
                }
            }

            int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
            if (activity < 0 && errno != EINTR)
                error("Error in select");

            for (int fd = 0; fd <= max_fd; fd++) {
                if (FD_ISSET(fd, &read_fds)) {
                    if (fd == client_socket) {
                        // Nova conexão de cliente
                        accept_new_client(client_socket);

                    } else if (fd == STDIN_FILENO) {
                        // Entrada do teclado (stdin)
                        char command[BUFFER_SIZE];
                        memset(&command, 0, sizeof(command));

                        if (fgets(command, sizeof(command), stdin) == NULL)
                            continue;
                
                        command[strcspn(command, "\n")] = '\0';

                        if (strcmp( command, "kill") != 0){
                            fprintf(stderr, "Wrong command, option: kill\n");
                            continue;
                        }

                        send_message_with_int_payload(peer_socket, REQ_DISCPEER, this_peer_id);

                    } else if (fd == peer_socket) {
                        // Mensagens do peer
                        Message msg = read_message(peer_socket);
                        server_process_message(peer_socket, &msg);

                    } else {
                        // Mensagens de clientes conectados
                        handle_client_messages(fd);

                    }
                }
            }
        }
    }

    close(client_socket);
    close(peer_socket);

    return 0;
}