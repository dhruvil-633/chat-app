#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sqlite3.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048
#define SERVER_PORT 8080

typedef struct {
    int socket;
    struct sockaddr_in address;
    char username[32];
    pthread_t thread;
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
sqlite3 *db;

const char *http_response = 
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n"
"Connection: close\r\n\r\n"
"<html><head><title>Chat Server</title><style>"
"body {font-family: Arial; max-width: 800px; margin: 0 auto; padding: 20px;}"
"#messages {border: 1px solid #ddd; padding: 10px; height: 300px; overflow-y: scroll; margin-bottom: 10px; background: #f9f9f9;}"
"#msg {width: 70%; padding: 8px; border: 1px solid #ddd; border-radius: 4px;}"
"button {padding: 8px 15px; background: #4285f4; color: white; border: none; border-radius: 4px; cursor: pointer;}"
"</style></head><body>"
"<h1>Chat Server</h1>"
"<div id='messages'></div>"
"<input id='msg' placeholder='Type your message'>"
"<button onclick='send()'>Send</button>"
"<script>"
"const ws = new WebSocket('wss://'+location.host);"
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
    
    // Check if this is an HTTP request
    bytes = recv(cli->socket, buffer, BUFFER_SIZE, MSG_PEEK);
    if (bytes > 0 && strstr(buffer, "HTTP")) {
        // Send HTTP response
        send(cli->socket, http_response, strlen(http_response), 0);
        close(cli->socket);
        free(cli);
        return NULL;
    }
    
    // Otherwise handle as WebSocket connection
    bytes = recv(cli->socket, buffer, BUFFER_SIZE, 0);
    if (bytes <= 0) {
        close(cli->socket);
        free(cli);
        return NULL;
    }
    
    buffer[bytes] = '\0';
    strncpy(cli->username, buffer, sizeof(cli->username)-1);
    cli->username[sizeof(cli->username)-1] = '\0';
    
    while (1) {
        bytes = recv(cli->socket, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;
        
        buffer[bytes] = '\0';
        
        if (buffer[0] == '@') {
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
    server_addr.sin_port = htons(getenv("PORT") ? atoi(getenv("PORT")) : SERVER_PORT);
    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
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