#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 6969
#define BUF_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf("Connected to server. Waiting for game to start...\n");

    while (1) {
        memset(buffer, 0, BUF_SIZE);
        int valread = read(sock, buffer, BUF_SIZE - 1);

        if (valread <= 0) {
            printf("Server disconnected.\n");
            break;
        }

        buffer[valread] = '\0';
        printf("%s", buffer);

        // If server prompts for input (check "Enter your equation")
        if (strstr(buffer, "Enter your equation")) {
            printf("> ");
            fflush(stdout);

            fgets(buffer, BUF_SIZE, stdin);
            buffer[strcspn(buffer, "\n")] = '\0';

            send(sock, buffer, strlen(buffer), 0);
        }

        // Check if server said "Game over" to stop
        if (strstr(buffer, "Game over")) {
            break;
        }
    }

    close(sock);
    return 0;
}
