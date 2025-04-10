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
#define HTTP_PORT 8081  // Different port for HTTP

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
sqlite3 *db;

// Simple HTTP response with chat interface
const char *http_response = 
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n"
"Connection: close\r\n\r\n"
"<html><head><title>Chat Server</title><style>"
"body {font-family: Arial; max-width: 800px; margin: 0 auto; padding: 20px;}"
"#messages {border: 1px solid #ccc; padding: 10px; height: 300px; overflow-y: scroll; margin-bottom: 10px;}"
"#msg {width: 70%; padding: 8px;}"
"button {padding: 8px 15px; background: #4CAF50; color: white; border: none;}"
"</style></head><body>"
"<h1>Chat Server</h1>"
"<div id='messages'></div>"
"<input id='msg' placeholder='Type your message'>"
"<button onclick='send()'>Send</button>"
"<script>"
"const ws = new WebSocket('ws://'+location.hostname+':%d');"
"ws.onmessage = e => {"
"  const msg = JSON.parse(e.data);"
"  const messages = document.getElementById('messages');"
"  messages.innerHTML += `<div><strong>${msg.sender}:</strong> ${msg.text}</div>`;"
"  messages.scrollTop = messages.scrollHeight;"
"};"
"function send() {"
"  const input = document.getElementById('msg');"
"  if (input.value.trim()) {"
"    ws.send(input.value);"
"    input.value = '';"
"  }"
"}"
"document.getElementById('msg').addEventListener('keypress', e => {"
"  if (e.key === 'Enter') send();"
"});"
"</script>"
"</body></html>";

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
            // Format message as JSON for frontend
            char json_msg[BUFFER_SIZE];
            snprintf(json_msg, sizeof(json_msg), 
                    "{\"sender\":\"%s\",\"text\":\"%s\"}", 
                    clients[sender_fd]->username, message);
            send(clients[i]->socket, json_msg, strlen(json_msg), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void send_private_message(int sender_fd, const char *recipient, const char *message) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && strcmp(clients[i]->username, recipient) == 0) {
            char formatted_msg[BUFFER_SIZE];
            snprintf(formatted_msg, sizeof(formatted_msg), 
                    "{\"sender\":\"%s (PM)\",\"text\":\"%s\"}", 
                    clients[sender_fd]->username, message);
            send(clients[i]->socket, formatted_msg, strlen(formatted_msg), 0);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void* handle_client(void *arg) {
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
    // Store username (simple auth for demo)
    strncpy(cli->username, buffer, sizeof(cli->username)-1);
    cli->username[sizeof(cli->username)-1] = '\0';
    
    // Main message loop
    while (1) {
        bytes = recv(cli->socket, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;
        
        buffer[bytes] = '\0';
        
        if (buffer[0] == '@') {
            // Private message
            char *recipient = strtok(buffer+1, " ");
            char *message = strtok(NULL, "\n");
            if (recipient && message) {
                send_private_message(cli->socket, recipient, message);
            }
        } else {
            broadcast_message(cli->socket, buffer);
        }
    }
    
    close(cli->socket);
    remove_client(cli->socket);
    return NULL;
}

void* http_server(void* arg) {
    int http_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in http_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(HTTP_PORT)
    };
    
    bind(http_fd, (struct sockaddr*)&http_addr, sizeof(http_addr));
    listen(http_fd, 10);
    
    printf("HTTP server running on port %d\n", HTTP_PORT);
    
    while (1) {
        int client = accept(http_fd, NULL, NULL);
        char response[2048];
        snprintf(response, sizeof(response), http_response, ntohs(http_addr.sin_port));
        write(client, response, strlen(response));
        close(client);
    }
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
    
    // Create chat socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    // Configure chat server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(getenv("PORT") ? atoi(getenv("PORT")) : 8080);
    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return 1;
    }
    
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        return 1;
    }
    
    printf("Chat server running on port %d\n", ntohs(server_addr.sin_port));
    
    // Start HTTP server thread
    pthread_t http_thread;
    pthread_create(&http_thread, NULL, http_server, NULL);
    
    // Main accept loop for chat server
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