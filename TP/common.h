#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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


#define BUFFER_SIZE 500

typedef struct {
    int code;     
    char payload[BUFFER_SIZE]; 
} Message;

void error(const char *msg);
void send_message(int socket, int code, const char *payload);

#endif
