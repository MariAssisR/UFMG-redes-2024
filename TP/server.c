#include "common.h"

#define MAX_CLIENTS 10
#define MAX_USERS 30
#define NOT_SET_NUMBER -10

int clients_sockets[MAX_CLIENTS] = {0};            // pos é LOC, num é SOCKET
int clients_loc_and_id[MAX_CLIENTS] = {0};         // pos é LOC, num é ID
int client_id;                                     // ID do cliente inicial (aleatório)
int client_count = 0;                              // Contador de clientes conectados

int user_ids[MAX_USERS] = {NOT_SET_NUMBER};                     // IDs dos usuários
int user_special[MAX_USERS] = {NOT_SET_NUMBER};                 // 0 ou 1 (especialidade do usuário)
int user_location[MAX_USERS] = {NOT_SET_NUMBER};                // Localização do usuário (de -1, 1 a 10)
int user_count = 0; 

int peer_id = -1;                                       // ID do peer que vai ser conectado
int this_peer_id = -1;
enum peer_connection_state { DISC = 0, CONN = 1 };
enum peer_connection_state peer_state;
int client_socket, peer_socket;
int passive_peer_socket = -1;

char list_of_users[BUFFER_SIZE] = "";

//functions
void accept_new_client(int socket);
void accept_new_peer(int socket);
void handle_peer_connection(int peer_port, int *peer_socket);
void create_passive_connection(int port, int socket, int is_client);
int create_active_connection(int port, int socket);
int create_socket();

//aux functions
int user_find_pos(){
    if(user_count < MAX_USERS){
        for (int i = 0; i < MAX_USERS; i++){
            if (user_ids[i] == 0)
                return i;
        }
    }
    perror("Client limit exceeded\n");
    return NOT_SET_NUMBER;
}

int user_exists_and_pos(int id){
    for (int i = 0; i < MAX_USERS; i++){
        if (user_ids[i] == id)
            return i;
    }
    return NOT_SET_NUMBER;
}

void get_list_of_users(int loc_id){
    int list_of_positions_in_array[MAX_USERS];
    int count = 0;
    list_of_users[0] = '\0';

    for(int i = 0; i < MAX_USERS; i++){
        if(user_location[i] == loc_id){
            list_of_positions_in_array[count] = i;
            count++;
        }
    }

    for(int i = 0; i < count; i++){
        int user_id = user_ids[list_of_positions_in_array[i]];
        char user_id_str[12];
        sprintf(user_id_str, "%d", user_id);
        strcat(list_of_users, user_id_str);
        if (i < count - 1) {
            strcat(list_of_users, ",");
        }
    }
    strcat(list_of_users, "\0");
}

int client_exists_and_pos(int id){
    for (int i = 0; i < MAX_CLIENTS; i++){
        if (clients_loc_and_id[i] == id)
            return i;
    }
    return NOT_SET_NUMBER;
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
                    printf("Successful disconnect\n");
                    printf("Peer %d disconnect\n", peer_id);
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
                    fprintf(stderr, "OK message received in server: %s\n", msg->payload);
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
                    fprintf(stderr, "ERROR message received in server: %s\n", msg->payload);
                    break;
            }
            exit(EXIT_FAILURE);
        }
            break;

        case REQ_CONNPEER:
            if(peer_id == -1){ 
                peer_id = (rand() % 100) + 10;
                printf("Peer %d connected\n", peer_id); 

                peer_socket = socket;

                send_message_with_int_payload(socket, RES_CONNPEER, peer_id);
                break;
            }
            send_message(socket, ERROR, "1");
            close(socket);
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
                close(passive_peer_socket);
                close(peer_socket);
                passive_peer_socket = -1;
                sleep(1);
            }
            else 
                send_message(socket, ERROR, "2");
            
            break;

        case REQ_CONN:
            digit = atoi(msg->payload); 
            clients_sockets[digit] = socket;
            clients_loc_and_id[digit] = client_id;
            client_count++;
            printf("Client %d added (Loc %d)\n", client_id,digit);

            send_message_with_int_payload(socket, RES_CONN, client_id);
            
            client_id++;

            break;

        case REQ_DISC:
            digit = atoi(msg->payload); 
            client_loc = client_exists_and_pos(digit);
            if(client_loc != NOT_SET_NUMBER){
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
            if(user_pos_add != NOT_SET_NUMBER){
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

            if(user_pos_find != NOT_SET_NUMBER){
                send_message_with_int_payload(socket, RES_USRLOC, user_location[user_pos_find]);
            }
            else{
                send_message(socket, ERROR, "18");
            }
            break;

        case REQ_USRACCESS:
            int uid_access = atoi(strtok(msg->payload, ","));
            char* direction = strtok(NULL, ",");
            client_loc = atoi(strtok(NULL, ","));

            int user_pos_access = user_exists_and_pos(uid_access);
            if(user_pos_access != NOT_SET_NUMBER){
                int locId = -1;
                if (strcmp(direction, "in") == 0) 
                    locId = client_loc;

                char res_access_msg[BUFFER_SIZE];
                snprintf(res_access_msg, sizeof(res_access_msg), "%d,%d", uid_access, locId);

                send_message(peer_socket, REQ_LOCREG, res_access_msg);

                Message msg_receive; 
                msg_receive = read_message(peer_socket);
                server_process_message(socket, &msg_receive);
            }
            else{
                send_message(socket, ERROR, "18");
            }
            break;

        case REQ_LOCREG:
            int uid_loc = atoi(strtok(msg->payload, ","));
            int loc = atoi(strtok(NULL, ","));

            int user_pos_loc = user_exists_and_pos(uid_loc);
            if(user_pos_loc == NOT_SET_NUMBER){   
                int new_user_pos_loc = user_find_pos(uid_loc);
                user_ids[new_user_pos_loc] = uid_loc;
                user_location[new_user_pos_loc] = loc;
                send_message(socket, RES_LOCREG, "-1");
            }
            else{
                send_message_with_int_payload(socket, RES_LOCREG, user_location[user_pos_loc]);
                user_location[user_pos_loc] = loc;
            }
                
            break;

        case RES_LOCREG:
            int loc_id = atoi(strtok(msg->payload, ","));

            send_message_with_int_payload(socket, RES_USRACCESS, loc_id);
                
            break;

        case REQ_LOCLIST:
            int uid_inspect = atoi(strtok(msg->payload, ","));
            client_loc = atoi(strtok(NULL, ","));

            send_message_with_int_payload(peer_socket, REQ_USRAUTH, uid_inspect);
            
            Message msg_receive; 
            msg_receive = read_message(peer_socket);
            int payload_msg_receive = atoi(msg_receive.payload); 
            if(payload_msg_receive == 1){
                get_list_of_users(client_loc);
            }

            server_process_message(socket, &msg_receive);
                
            break;

        case REQ_USRAUTH:
            int uid_inspect_auth = atoi(strtok(msg->payload, ","));

            int user_pos_inspect = user_exists_and_pos(uid_inspect_auth);
            if(user_pos_inspect != NOT_SET_NUMBER){
                if(user_special[user_pos_inspect])
                    send_message_with_int_payload(socket, RES_USRAUTH, 1);
                else
                    send_message_with_int_payload(socket, RES_USRAUTH, 0);
            }
            else{
                send_message(socket, ERROR, "18");
            }
               
            break;

        case RES_USRAUTH:
            int is_special = atoi(strtok(msg->payload, ","));
            if(!is_special)
                send_message(socket, ERROR, "19");
            else
                send_message(socket, RES_LOCLIST, list_of_users);

                
            break;

        default:
            fprintf(stderr, "Message with unknow code: %s\n", msg->payload);
            exit(EXIT_SUCCESS);
            break;
    }
}

void accept_new_client(int socket) {
    struct sockaddr_storage client_addr;
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

void accept_new_peer(int socket) {
    struct sockaddr_storage client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int new_peer_socket = accept(socket, (struct sockaddr *)&client_addr, &addrlen);

    if (new_peer_socket < 0){
        error("Error accepting connection");
        return;
    }
    
    Message msg = read_message(new_peer_socket);
    server_process_message(new_peer_socket, &msg);
}

void handle_peer_connection(int peer_port, int *peer_socket) {
    // Try active connection
    if (create_active_connection(peer_port, *peer_socket) == 0) {
        peer_state = CONN;
        send_message(*peer_socket, REQ_CONNPEER, "");
    } else {
        // Fallback to passive connection
        close(*peer_socket);
        *peer_socket = create_socket();
        create_passive_connection(peer_port, *peer_socket, 0);
        passive_peer_socket = *peer_socket;
        peer_state = CONN;
    }
}

void create_passive_connection(int port, int socket, int is_client){
    struct sockaddr_storage server_addr;

    if (configure_ip_address("::", port, &server_addr) != 0) {
        close(socket);
        error("Failed to configure address for passive connection");
    }

    socklen_t addr_len = (server_addr.ss_family == AF_INET) ? 
                         sizeof(struct sockaddr_in) : 
                         sizeof(struct sockaddr_in6);

    if (bind(socket, (struct sockaddr *)&server_addr, addr_len) < 0){
        close(socket);
        error("Bind failed");
    }

    if(!is_client)
        printf("No peer found, starting to listen...\n");

    if (listen(socket, MAX_CLIENTS) < 0) {
        close(socket);
        error("Listen failed");
    }
}

int create_active_connection(int port, int socket){
    struct sockaddr_storage server_addr;

    if (configure_ip_address("::", port, &server_addr) != 0)
        error("Failed to configure address for passive connection");

    socklen_t addr_len = (server_addr.ss_family == AF_INET) ? 
                         sizeof(struct sockaddr_in) : 
                         sizeof(struct sockaddr_in6);

    if (connect(socket, (struct sockaddr*)&server_addr, addr_len) < 0){
        return -1;
    }
    return 0;
}

int create_socket(){
    int sock;
    
    if ((sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0)
        error("Socket creation failed");

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        close(sock);
        error("setsockopt failed");
    }
    
    return sock;
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
    client_socket = create_socket();
    create_passive_connection(client_port, client_socket, 1);
    peer_socket = create_socket();
    peer_state = DISC;
    fd_set read_fds;
    while (1) {
        if(peer_state == CONN){
            FD_ZERO(&read_fds);
            FD_SET(client_socket, &read_fds);
            FD_SET(peer_socket, &read_fds);
            if(passive_peer_socket != -1)
                FD_SET(passive_peer_socket, &read_fds);
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
                    if (fd == STDIN_FILENO) {
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

                    } else if (fd == client_socket) {
                        // Nova conexão de cliente
                        accept_new_client(client_socket);

                    } else if (fd == passive_peer_socket ) {
                        // Nova conexão de peer
                        accept_new_peer(passive_peer_socket);

                    } else if (fd == peer_socket) {
                        // Mensagens do peer
                        Message msg = read_message(peer_socket);
                        server_process_message(peer_socket, &msg);

                    } else {
                        // Mensagens de clientes conectados
                        Message msg = read_message(fd);
                        server_process_message(fd, &msg);

                    }
                }
            }
        }
        else {
            handle_peer_connection(peer_port, &peer_socket);
        }
    }

    close(client_socket);
    close(peer_socket);

    return 0;
}