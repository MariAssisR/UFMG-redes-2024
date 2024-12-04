#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 500

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
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

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <server_ip> <user_server_port> <location_server_port> <location_id>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];
    int user_server_port = atoi(argv[2]);
    int location_server_port = atoi(argv[3]);
    int location_id = atoi(argv[4]);

    if (location_id < 1 || location_id > 10) {
        fprintf(stderr, "Invalid location ID. Must be between 1 and 10.\n");
        exit(EXIT_FAILURE);
    }

    // connect to servers
    int user_socket = connect_to_server(server_ip, user_server_port);
    printf("Connected to user server at %s:%d\n", server_ip, user_server_port);

    int location_socket = connect_to_server(server_ip, location_server_port);
    printf("Connected to location server at %s:%d\n", server_ip, location_server_port);

    // send message to servers
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "Hello from location %d", location_id);
    send(user_socket, buffer, strlen(buffer), 0);
    send(location_socket, buffer, strlen(buffer), 0);

    printf("Messages sent to servers\n");

    close(user_socket);
    close(location_socket);
    return 0;
}