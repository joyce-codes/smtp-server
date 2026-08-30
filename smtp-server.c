#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>

#define PORT 25
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 1000

int client_sockets[MAX_CLIENTS] = {0};
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("Received: %s", buffer);

        // Simple response
        const char *response = "220 SMTP Server Ready\r\n";
        send(client_socket, response, strlen(response), 0);
    }

    close(client_socket);
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == client_socket) {
            client_sockets[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&lock);
    free(arg);
    pthread_exit(NULL);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t client_thread;
    fd_set read_fds, master_fds;
    int fdmax;

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Prepare the sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("SMTP server is running on port %d\n", PORT);

    FD_ZERO(&master_fds);
    FD_SET(server_socket, &master_fds);
    fdmax = server_socket;

    while (1) {
        read_fds = master_fds;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_socket) {
                    client_addr_len = sizeof(client_addr);
                    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
                    if (client_socket == -1) {
                        perror("accept");
                    } else {
                        FD_SET(client_socket, &master_fds);
                        if (client_socket > fdmax) {
                            fdmax = client_socket;
                        }
                        pthread_mutex_lock(&lock);
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (client_sockets[j] == 0) {
                                client_sockets[j] = client_socket;
                                break;
                            }
                        }
                        pthread_mutex_unlock(&lock);
                    }
                } else {
                    int *client_socket_ptr = malloc(sizeof(int));
                    *client_socket_ptr = i;
                    if (pthread_create(&client_thread, NULL, handle_client, client_socket_ptr) < 0) {
                        perror("Could not create thread");
                        close(i);
                        FD_CLR(i, &master_fds);
                    } else {
                        pthread_detach(client_thread);
                    }
                }
            }
        }
    }

    close(server_socket);
    return 0;
}