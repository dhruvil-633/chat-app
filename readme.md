Here's a detailed Markdown documentation explaining the code structure and implementation:

```markdown
# Socket Programming Assignment - Code Documentation

## Table of Contents
1. [Project Structure](#project-structure)
2. [Server Implementation](#server-implementation)
   - [Data Structures](#server-data-structures)
   - [Thread Handling](#thread-handling)
   - [Message Processing](#message-processing)
3. [Client Implementation](#client-implementation)
   - [Connection Setup](#connection-setup)
   - [Message Handling](#message-handling)
4. [Key Functions](#key-functions)
5. [Flow Diagrams](#flow-diagrams)
6. [Compilation & Execution](#compilation--execution)

## Project Structure

```
chat-app/
├── server.c        # Multi-threaded chat server
├── client.c        # Client implementation
└── README.md       # Documentation
```

## Server Implementation

### Server Data Structures
```c
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int socket;            
    char username[32];     
} Client;

Client clients[MAX_CLIENTS];  
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;  
```

### Thread Handling
1. **Main Server Loop**:
   - Creates socket and binds to port 8080
   - Listens for incoming connections
   - Accepts new connections and spawns threads

```c
while (1) {
    
    new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    
        pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == 0) {
            clients[i].socket = new_socket;
            pthread_create(&thread, NULL, handle_client, &clients[i]);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}
```

### Message Processing
- **Group Messages**: Broadcast to all clients
- **Private Messages**: Format `@username message`

```c
if (buffer[0] == '@') {
    char *recipient = strtok(buffer+1, " ");
    char *message = strtok(NULL, "\n");
    
} else {
        for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != 0) {
            send(clients[i].socket, buffer, strlen(buffer), 0);
        }
    }
}
```

## Client Implementation

### Connection Setup
```c

sock = socket(AF_INET, SOCK_STREAM, 0);


serv_addr.sin_family = AF_INET;
serv_addr.sin_port = htons(8080);
inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);


connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
```

### Message Handling
- Dedicated thread for receiving messages
- Main thread handles user input

```c
void *receive_handler(void *arg) {
    while (1) {
        int receive = recv(sock, buffer, BUFFER_SIZE, 0);
        if (receive <= 0) break;
        buffer[receive] = '\0';
        printf("%s\n", buffer);
    }
}


pthread_create(&recv_thread, NULL, receive_handler, NULL);
```

## Key Functions

| Function | Description |
|----------|-------------|
| `handle_client()` | Server-side thread function for each client |
| `receive_handler()` | Client-side thread for incoming messages |
| `send()`/`recv()` | Socket communication functions |
| `pthread_create()` | Creates new threads |
| `pthread_mutex_lock()` | Thread synchronization |

## Flow Diagrams

### Server Workflow
```
1. Initialize server socket
2. Bind to port 8080
3. Listen for connections
4. For each new connection:
   a. Add client to array
   b. Create handler thread
5. In handler thread:
   a. Receive messages
   b. Process (broadcast/private)
   c. Cleanup on disconnect
```

### Client Workflow
```
1. Connect to server
2. Send username
3. Start receive thread
4. Main thread:
   a. Read user input
   b. Send to server
5. Receive thread:
   a. Display incoming messages
```

## Compilation & Execution

```bash
gcc server.c -o server -lpthread
gcc client.c -o client -lpthread

./server

./client
```

## Important Notes
1. **Thread Safety**: All client array access is protected by mutex
2. **Error Handling**: Basic connection error checks included
3. **Limitations**: 
   - No authentication
   - No message persistence
   - Fixed maximum clients (10)
```

This documentation provides:
- Complete code structure explanation
- Key algorithm details
- Flow visualizations
- Compilation instructions
- Important implementation notes

Would you like me to add any specific details about particular functions or the protocol design?
```

## ScreenShots
