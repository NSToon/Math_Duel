// ================= client.c ================= 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 6969
#define BUF_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE];
    clock_t start_time, end_time;
    double total_time = 0;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf("🔗 Connected to server. Waiting for game to start...\n");
    start_time = clock();

    while (1) {
        memset(buffer, 0, BUF_SIZE);
        int valread = read(sock, buffer, BUF_SIZE - 1);

        if (valread <= 0) {
            printf("❌ Server disconnected.\n");
            break;
        }

        buffer[valread] = '\0';
        printf("%s", buffer);

        // If server asks for equation input
        if (strstr(buffer, "Enter your equation:") || 
            strstr(buffer, "try again:") || 
            strstr(buffer, "Try again:")) {
            
            if (!fgets(buffer, BUF_SIZE, stdin)) {
                printf("Input error\n");
                break;
            }
            
            // Remove newline
            buffer[strcspn(buffer, "\n")] = '\0';
            
            if (send(sock, buffer, strlen(buffer), 0) <= 0) {
                printf("Failed to send data\n");
                break;
            }
        }
        
        // If server asks for total time
        else if (strstr(buffer, "Send your total time in ms:")) {
            end_time = clock();
            total_time = ((double)(end_time - start_time) / CLOCKS_PER_SEC) * 1000;
            
            snprintf(buffer, BUF_SIZE, "%.0f", total_time);
            
            if (send(sock, buffer, strlen(buffer), 0) <= 0) {
                printf("Failed to send time data\n");
                break;
            }
        }

        // Check if game is over
        if (strstr(buffer, "Game over") || strstr(buffer, "Thanks for playing")) {
            printf("\n🎯 Your total time: %.2f ms\n", total_time);
            break;
        }
    }

    close(sock);
    printf("👋 Disconnected from server.\n");
    return 0;
}