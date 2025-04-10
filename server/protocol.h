#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <netinet/in.h>

typedef struct {
    int socket;
    struct sockaddr_in address;
    char username[32];
    pthread_t thread;
} client_t;

// Authentication codes
#define AUTH_SUCCESS 0
#define AUTH_FAILED 1

// Message types
#define MSG_BROADCAST 0
#define MSG_PRIVATE 1

#endif