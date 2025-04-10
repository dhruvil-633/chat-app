#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 2048

int sock;
char username[32];

void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) {
            printf("\nConnection lost\n");
            exit(1);
        }
        buffer[bytes] = '\0';
        printf("\n%s\n> ", buffer);
        fflush(stdout);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        return 1;
    }
    
    struct sockaddr_in server_addr;
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }
    
    printf("Username: ");
    fgets(username, 32, stdin);
    username[strcspn(username, "\n")] = '\0';
    
    printf("Password: ");
    char password[32];
    fgets(password, 32, stdin);
    password[strcspn(password, "\n")] = '\0';
    
    // Send auth packet
    char auth_packet[256];
    sprintf(auth_packet, "AUTH %s %s", username, password);
    send(sock, auth_packet, strlen(auth_packet), 0);
    
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, NULL);
    
    char buffer[BUFFER_SIZE];
    printf("> ");
    while (fgets(buffer, BUFFER_SIZE, stdin)) {
        buffer[strcspn(buffer, "\n")] = '\0';
        
        if (strcmp(buffer, "/quit") == 0) break;
        
        send(sock, buffer, strlen(buffer), 0);
        printf("> ");
    }
    
    close(sock);
    return 0;
}