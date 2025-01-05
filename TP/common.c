#include "common.h"

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void send_message(int socket, int code, const char *payload) {
    Message msg;
    msg.code = code;
    strncpy(msg.payload, payload, sizeof(msg.payload) - 1);

    if (send(socket, &msg, sizeof(msg), 0) == -1) {
        perror("Erro ao enviar mensagem");
    }
}

Message read_message(int socket){
    Message msg;
    msg.code = -1;
    int valread = read(socket, &msg, sizeof(msg));
    if (valread <= 0) {
        perror("Error on read client message");
    }
    return msg;
}
