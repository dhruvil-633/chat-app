#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    char username[32];
} Client;

Client clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg) {
    Client *client = (Client *)arg;
    char buffer[BUFFER_SIZE];

    // Get username
    if (recv(client->socket, buffer, sizeof(buffer), 0) <= 0) {
        perror("Username receive failed");
        close(client->socket);
        return NULL;
    }
    strcpy(client->username, buffer);

    printf("%s connected\n", client->username);

    while (1) {
        int receive = recv(client->socket, buffer, sizeof(buffer), 0);
        if (receive <= 0) {
            printf("%s disconnected\n", client->username);
            close(client->socket);
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket == client->socket) {
                    clients[i].socket = 0;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            return NULL;
        }

        buffer[receive] = '\0';
        printf("%s: %s\n", client->username, buffer);

        // Check for private message
        if (buffer[0] == '@') {
            char *recipient = strtok(buffer + 1, " ");
            char *message = strtok(NULL, "\n");
            
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket != 0 && strcmp(clients[i].username, recipient) == 0) {
                    char private_msg[BUFFER_SIZE];
                    snprintf(private_msg, sizeof(private_msg), "[PM from %s] %s", client->username, message);
                    send(clients[i].socket, private_msg, strlen(private_msg), 0);
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
        } else {
            // Broadcast to all clients
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket != 0 && clients[i].socket != client->socket) {
                    char broadcast_msg[BUFFER_SIZE];
                    snprintf(broadcast_msg, sizeof(broadcast_msg), "%s: %s", client->username, buffer);
                    send(clients[i].socket, broadcast_msg, strlen(broadcast_msg), 0);
                }
            }
            pthread_mutex_unlock(&clients_mutex);
        }
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port 8080\n");

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == 0) {
                clients[i].socket = new_socket;
                pthread_t thread;
                pthread_create(&thread, NULL, handle_client, &clients[i]);
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    return 0;
}