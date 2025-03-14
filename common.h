#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h> 
#include <sys/types.h>  
#include <sys/select.h> 
#include <errno.h>
#include <time.h>

#define REQ_CONNPEER  17
#define RES_CONNPEER  18
#define REQ_DISCPEER  19
#define REQ_CONN      20
#define RES_CONN      21
#define REQ_DISC      22

#define REQ_USRADD    33
#define REQ_USRACCESS 34
#define RES_USRACCESS 35
#define REQ_LOCREG    36
#define RES_LOCREG    37
#define REQ_USRLOC    38
#define RES_USRLOC    39
#define REQ_LOCLIST   40
#define RES_LOCLIST   41
#define REQ_USRAUTH   42
#define RES_USRAUTH   43

#define ERROR         255
#define OK            0

#define MAX_CLIENTS 10
#define MAX_USERS 30
#define NOT_SET_NUMBER -10

#define BUFFER_SIZE 500
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef struct {
    int code;     
    char payload[BUFFER_SIZE]; 
} Message;

void error(const char *msg);
int configure_ip_address(const char *ip, uint16_t port, struct sockaddr_storage *result);
void send_message_with_int_payload(int socket, int code, int payload);
void send_message(int socket, int code, const char *payload);
Message read_message(int socket);

#endif
