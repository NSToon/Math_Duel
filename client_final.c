#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h> 

#define PORT 6969
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    double total_time = 0.0;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf("Connected to the server.\n");

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("Disconnected from server.\n");
            break;
        }

        buffer[bytes_received] = '\0';
        printf("%s", buffer);

        if (strstr(buffer, "wins!") || strstr(buffer, "tie")) {
            break;
        }

        if (strstr(buffer, "Target") || strstr(buffer, "Incorrect") || strstr(buffer, "Invalid")) {
            printf("Enter your expression: ");
            fflush(stdout);

            struct timeval start, end;
            gettimeofday(&start, NULL);  // เริ่มจับเวลา

            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
                perror("Input error");
                break;
            }

            gettimeofday(&end, NULL);  // จบจับเวลา

            double time_this_round = (end.tv_sec - start.tv_sec) +
                                     (end.tv_usec - start.tv_usec) / 1000000.0;
            total_time += time_this_round;

            send(sock, buffer, strlen(buffer), 0);
        } 
        else if (strstr(buffer, "Send your total time in s:")) {
            int total_time_sec = (int)total_time;  // ใช้วินาทีตรงๆ
            char time_msg[64];
            snprintf(time_msg, sizeof(time_msg), "%d\n", total_time_sec);
            send(sock, time_msg, strlen(time_msg), 0);

            printf("Sending total time: %d s\n", total_time_sec);
        }
    }

    close(sock);
    return 0;
}