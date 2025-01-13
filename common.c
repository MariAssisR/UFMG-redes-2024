#include "common.h"

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int configure_ip_address(const char *ip, uint16_t port, struct sockaddr_storage *result) {
    if (!ip || !result) 
        return -1;

    memset(result, 0, sizeof(*result));

    uint16_t net_port = htons(port); 

    if (strchr(ip, ':')) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)result;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = net_port;

        if (inet_pton(AF_INET6, ip, &addr6->sin6_addr) <= 0)
            return -2;
    } else {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)result;
        addr4->sin_family = AF_INET;
        addr4->sin_port = net_port;

        if (inet_pton(AF_INET, ip, &addr4->sin_addr) <= 0)
            return -3;
    }

    return 0;
}

void send_message_with_int_payload(int socket, int code, int payload) {
    Message msg;
    msg.code = code;

    char str_payload[BUFFER_SIZE];
    snprintf(str_payload, sizeof(str_payload), "%d", payload);

    strncpy(msg.payload, str_payload, sizeof(msg.payload) - 1);

    if (send(socket, &msg, sizeof(msg), 0) == -1) {
        perror("Error on send message");
    }
}

void send_message(int socket, int code, const char *payload) {
    Message msg;
    msg.code = code;
    strncpy(msg.payload, payload, sizeof(msg.payload) - 1);

    if (send(socket, &msg, sizeof(msg), 0) == -1) {
        perror("Error on send message");
    }
}

Message read_message(int socket){
    Message msg;
    msg.code = -1;
    int valread = read(socket, &msg, sizeof(msg));
    if (valread <= 0) {
        perror("Error on read message");
    }
    return msg;
}