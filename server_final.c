// ================= server.c =================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <time.h>

#define PORT 6969
#define ROUNDS 5
#define BUFFER_SIZE 1024
#define MAX_CLIENTS_PER_ROOM 2

int score_of_expression(const char* expr) {
    int score = 0;
    for (int i = 0; expr[i] != '\0'; i++) {
        if (isdigit(expr[i])) {
            score += 1;
        } else if (strchr("+-/()", expr[i])) {
            score += 1;
        } else if (expr[i] == '*') {
            score += 2;
        }
    }
    return score;
}

int contains_only_allowed_ops(const char* expr, const char* allowed_ops) {
    for (int i = 0; expr[i]; i++) {
        if (strchr("+-*/", expr[i]) && !strchr(allowed_ops, expr[i])) {
            return 0;
        }
    }
    return 1;
}

int evaluate_expression(const char* expr, const char* allowed_ops) {
    int a, b;
    char op;

    if (!contains_only_allowed_ops(expr, allowed_ops)) {
        return -2;
    }

    if (sscanf(expr, "%d%c%d", &a, &op, &b) == 3) {
        switch (op) {
            case '+': return a + b;
            case '-': return a - b;
            case '*': return a * b;
            case '/': return (b != 0) ? a / b : -1;
        }
    }
    return -1;
}

void play_game(int client1, int client2) {
    srand(time(NULL));
    char buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE];
    int total_score1 = 0, total_score2 = 0;
    const char* allowed_ops_sets[] = {"+-", "*/", "+*", "-/", "+-*/"};

    for (int round = 1; round <= ROUNDS; round++) {
        int target = rand() % 100;
        const char* allowed_ops = allowed_ops_sets[rand() % 5];

        char msg[BUFFER_SIZE];
        snprintf(msg, sizeof(msg),
                 "ROUND %d: Target = %d\nOnly two numbers allowed (e.g., 10 + 10)\nAllowed operators: %s\n",
                 round, target, allowed_ops);
        send(client1, msg, strlen(msg), 0);
        send(client2, msg, strlen(msg), 0);

        int valid1 = 0, valid2 = 0;

        while (!valid1) {
            memset(buffer1, 0, sizeof(buffer1));
            recv(client1, buffer1, BUFFER_SIZE, 0);
            buffer1[strcspn(buffer1, "\n")] = 0;

            int result = evaluate_expression(buffer1, allowed_ops);
            if (result == -2) {
                send(client1, "Invalid operator used.\n", 24, 0);
            } else if (result == target) {
                valid1 = 1;
                send(client1, "Correct\n", 8, 0);
            } else {
                send(client1, "Incorrect, try again\n", 22, 0);
            }
        }

        while (!valid2) {
            memset(buffer2, 0, sizeof(buffer2));
            recv(client2, buffer2, BUFFER_SIZE, 0);
            buffer2[strcspn(buffer2, "\n")] = 0;

            int result = evaluate_expression(buffer2, allowed_ops);
            if (result == -2) {
                send(client2, "Invalid operator used.\n", 24, 0);
            } else if (result == target) {
                valid2 = 1;
                send(client2, "Correct\n", 8, 0);
            } else {
                send(client2, "Incorrect, try again\n", 22, 0);
            }
        }

        int score1 = strlen(buffer1) > 0 ? score_of_expression(buffer1) : 0;
        int score2 = strlen(buffer2) > 0 ? score_of_expression(buffer2) : 0;

        total_score1 += score1;
        total_score2 += score2;

        snprintf(msg, sizeof(msg), "Your score this round: %d\n", score1);
        send(client1, msg, strlen(msg), 0);
        snprintf(msg, sizeof(msg), "Your score this round: %d\n", score2);
        send(client2, msg, strlen(msg), 0);

        printf("Round %d result: C1=%s (%d), C2=%s (%d)\n", round, buffer1, score1, buffer2, score2);
    }

    // รับเวลารวมจาก client ทั้งสอง ฝั่ง
    char time_buf1[BUFFER_SIZE], time_buf2[BUFFER_SIZE];
    send(client1, "Send your total time in ms:\n", 28, 0);
    send(client2, "Send your total time in ms:\n", 28, 0);

    memset(time_buf1, 0, sizeof(time_buf1));
    memset(time_buf2, 0, sizeof(time_buf2));

    recv(client1, time_buf1, BUFFER_SIZE, 0);
    recv(client2, time_buf2, BUFFER_SIZE, 0);
    printf("Received time1: %s\n", time_buf1);
    printf("Received time2: %s\n", time_buf2);
    time_buf1[strcspn(time_buf1, "\r\n")] = 0;
    time_buf2[strcspn(time_buf2, "\r\n")] = 0;

    long time1 = atol(time_buf1);
    long time2 = atol(time_buf2);

    char final[BUFFER_SIZE];
    snprintf(final, sizeof(final),
        "Final Score:\nClient 1: %d\nClient 2: %d\nClient 1 time: %ld ms\nClient 2 time: %ld ms\n",
        total_score1, total_score2, time1, time2);

    if (time1 < time2) {
        strcat(final, "Client 1 was faster and gets +5 bonus!\n");
        total_score1 += 5;
    } else if (time2 < time1) {
        strcat(final, "Client 2 was faster and gets +5 bonus!\n");
        total_score2 += 5;
    } else {
        strcat(final, "No time bonus awarded.\n");
    }

    strcat(final, (total_score1 > total_score2) ? "Client 1 wins!\n" :
                  (total_score1 < total_score2) ? "Client 2 wins!\n" : "It's a tie!\n");

    send(client1, final, strlen(final), 0);
    send(client2, final, strlen(final), 0);

    printf("%s", final);
}


int main() {
    int server_md;
    struct sockaddr_in ser_md_addr;
    int addrlen = sizeof(ser_md_addr);

    server_md = socket(AF_INET, SOCK_STREAM, 0);
    if (server_md == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&ser_md_addr, 0, sizeof(ser_md_addr));
    ser_md_addr.sin_family = AF_INET;
    ser_md_addr.sin_addr.s_addr = INADDR_ANY;
    ser_md_addr.sin_port = htons(PORT);

    if (bind(server_md, (struct sockaddr *)&ser_md_addr, sizeof(ser_md_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    listen(server_md, MAX_CLIENTS_PER_ROOM);
    printf("Server is running on port %d...\n", PORT);

    int room_clients[MAX_CLIENTS_PER_ROOM];
    int count = 0;

    while (count < MAX_CLIENTS_PER_ROOM) {
        int new_socket = accept(server_md, NULL, NULL);
        if (new_socket < 0) {
            perror("accept failed");
            continue;
        }
        room_clients[count++] = new_socket;
        printf("Client %d connected\n", count);
    }

    const char *start_msg = "Game started!\n";
    send(room_clients[0], start_msg, strlen(start_msg), 0);
    send(room_clients[1], start_msg, strlen(start_msg), 0);

    play_game(room_clients[0], room_clients[1]);

    close(server_md);
    return 0;
}
