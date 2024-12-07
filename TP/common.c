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

        case RES_CONN:
            // Processar a resposta de conexão (RES_CONN - código 21)
            printf("Cliente conectado com ID: %s\n", msg->payload);
            break;

        case REQ_CONN:
            break;

        case REQ_DISC:
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
