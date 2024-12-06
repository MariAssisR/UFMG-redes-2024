#include "common.h"

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void process_message(int socket, Message *msg) {
    int valread = read(socket, msg, sizeof(*msg));
    if (valread <= 0) {
        perror("Error on read client message");
        close(socket);
        return;
    }

    switch (msg->code) {
        case OK:
            // Processar a resposta de sucesso
            printf("Mensagem OK recebida: %s\n", msg->payload);
            break;

        case ERROR:
            // Processar mensagem de erro
            printf("Mensagem de erro recebida: %s\n", msg->payload);
            break;

        case REQ_CONN:
            // Processar a solicitação de conexão (REQ_CONN - código 20)
            
            break;

        case RES_CONN:
            // Processar a resposta de conexão (RES_CONN - código 21)
            printf("Cliente conectado com ID: %s\n", msg->payload);
            break;

        default:
            // Caso a mensagem tenha um código desconhecido
            printf("Mensagem desconhecida recebida: %s\n", msg->payload);
            break;
    }
}

void send_message(int socket, int code, const char *payload) {
    Message msg;
    msg.code = code;
    strncpy(msg.payload, payload, sizeof(msg.payload) - 1);

    if (send(socket, &msg, sizeof(msg), 0) == -1) {
        perror("Erro ao enviar mensagem");
    }
}
