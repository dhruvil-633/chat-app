#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sqlite3.h>
#include "protocol.h"

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
sqlite3 *db;

void add_client(client_t *cli) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i]) {
            clients[i] = cli;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && clients[i]->socket == sock) {
            free(clients[i]);
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void broadcast_message(int sender_fd, const char *message) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && clients[i]->socket != sender_fd) {
            send(clients[i]->socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    client_t *cli = (client_t *)arg;
    char buffer[BUFFER_SIZE];
    int bytes;
    
    // Authentication
    bytes = recv(cli->socket, buffer, BUFFER_SIZE, 0);
    if (bytes <= 0) {
        close(cli->socket);
        free(cli);
        return NULL;
    }
    
    buffer[bytes] = '\0';
    // Parse auth packet and authenticate
    
    // Main message loop
    while (1) {
        bytes = recv(cli->socket, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;
        
        buffer[bytes] = '\0';
        
        if (buffer[0] == '@') {
            // Private message
            char *recipient = strtok(buffer+1, " ");
            char *message = strtok(NULL, "\n");
            // Find recipient and send
        } else {
            broadcast_message(cli->socket, buffer);
        }
    }
    
    close(cli->socket);
    remove_client(cli->socket);
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr;
    
    // Initialize database
    if (sqlite3_open("chat.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    // Configure server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(getenv("PORT") ? atoi(getenv("PORT")) : 8080);
    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) {
        perror("Bind failed");
        return 1;
    }
    
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        return 1;
    }
    
    printf("Server running on port %d\n", ntohs(server_addr.sin_port));
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }
        
        client_t *cli = malloc(sizeof(client_t));
        cli->socket = client_fd;
        cli->address = client_addr;
        
        pthread_create(&cli->thread, NULL, handle_client, cli);
    }
    
    sqlite3_close(db);
    return 0;
}