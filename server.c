#include <stdio.h>
#include <stdlib.h>
#include <string.h>     
#include <unistd.h>     
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>

#define PORT 6969
#define BUF_SIZE 1024
#define MAX_CLIENTS_PER_ROOM 2

int check_answer(char *equation){
    int len = strlen(equation);
    char digits_used[10] = {0};
    int equal_sign_count = 0;

    for (int i = 0; i < len; i++) {
        char c = equation[i];

        if (isdigit(c)) {
            int d = c - '0';
            if (digits_used[d]) {
                return 0; // duplicate digit found
            }
            digits_used[d] = 1;
        } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '=') {
            if (c == '=') equal_sign_count++;
        } else if (!isspace(c)) {
            return 0; // invalid character
        }
    }

    if (equal_sign_count != 1) {
        return 0; // must contain exactly one '='
    }

    char left[100], right[100];
    char *eq_pos = strchr(equation, '=');
    if (!eq_pos) return 0;

    strncpy(left, equation, eq_pos - equation);
    left[eq_pos - equation] = '\0';
    strcpy(right, eq_pos + 1);

    int left_result = 0, right_result = 0;
    char op;
    int a, b;

    if (sscanf(left, "%d%c%d", &a, &op, &b) != 3) {
        return 0;
    }

    switch (op) {
        case '+': left_result = a + b; break;
        case '-': left_result = a - b; break;
        case '*': left_result = a * b; break;
        case '/':
            if (b == 0) return 0;
            if (a % b != 0) return 0; // must divide evenly
            left_result = a / b;
            break;
        default:
            return 0;
    }

    if (sscanf(right, "%d", &right_result) != 1) {
        return 0;
    }

    return left_result == right_result;
}

void play_game(int client1, int client2) {
    const char *question = "Question 1: Create an equation using the digits 0 - 9.\n";
    const char *prompt = "Enter your equation:\n";
    char answer[512];

    int clients[2] = {client1, client2};
    int correct[2] = {0, 0};

    // Send the question and initial prompt to both clients
    for (int i = 0; i < 2; i++) {
        send(clients[i], question, strlen(question), 0);
        send(clients[i], prompt, strlen(prompt), 0);
    }

    while (!correct[0] || !correct[1]) {
        for (int i = 0; i < 2; i++) {
            if (correct[i]) continue; // skip if already correct

            memset(answer, 0, sizeof(answer));
            int valread = recv(clients[i], answer, sizeof(answer) - 1, 0);
            if (valread <= 0) {
                printf("Client %d disconnected.\n", i + 1);
                return;
            }
            answer[valread] = '\0';

            printf("Client %d answered: %s\n", i + 1, answer);

            if (check_answer(answer)) {
                send(clients[i], "Correct answer!\n", strlen("Correct answer!\n"), 0);
                correct[i] = 1;
            } else {
                send(clients[i], "Incorrect answer, please try again.\n", strlen("Incorrect answer, please try again.\n"), 0);
                send(clients[i], prompt, strlen(prompt), 0);
            }
        }
    }

    const char *end_msg = "Game over. Thank you for playing!\n";
    send(client1, end_msg, strlen(end_msg), 0);
    send(client2, end_msg, strlen(end_msg), 0);
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

    printf("Both clients connected. Starting the game...\n");
    const char *start_msg = "Game started!\n";
    for (int i = 0; i < MAX_CLIENTS_PER_ROOM; i++) {
        send(room_clients[i], start_msg, strlen(start_msg), 0);
    }

    play_game(room_clients[0], room_clients[1]);

    close(server_md);
    return 0;
}
