#include "common.h"

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
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