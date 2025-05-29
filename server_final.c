// ================= server.c ================= 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

#define PORT 6969
#define ROUNDS 5
#define BUFFER_SIZE 1024
#define MAX_CLIENTS_PER_ROOM 2
#define MAX_CONCURRENT_ROOMS 10

// Structure to pass data to game thread
typedef struct {
    int client1;
    int client2;
    int room_id;
} game_room_t;

// Mutex for thread-safe operations
pthread_mutex_t room_counter_mutex = PTHREAD_MUTEX_INITIALIZER;
int room_counter = 0;

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

void play_game(int client1, int client2, int room_id) {
    srand(time(NULL) + room_id); // Add room_id for better randomization
    char buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE];
    int total_score1 = 0, total_score2 = 0;
    const char* allowed_ops_sets[] = {"+-", "*/", "+*", "-/", "+-*/"};

    printf("[Room %d] Game started between clients %d and %d\n", room_id, client1, client2);

    for (int round = 1; round <= ROUNDS; round++) {
        int target = rand() % 100;
        const char* allowed_ops = allowed_ops_sets[rand() % 5];

        char msg[BUFFER_SIZE];
        snprintf(msg, sizeof(msg),
                 "ROUND %d: Target = %d\nOnly two numbers allowed (e.g., 10 + 10)\nAllowed operators: %s\nEnter your equation: ",
                 round, target, allowed_ops);
        
        if (send(client1, msg, strlen(msg), 0) <= 0 || send(client2, msg, strlen(msg), 0) <= 0) {
            printf("[Room %d] Client disconnected during round %d\n", room_id, round);
            return;
        }

        int valid1 = 0, valid2 = 0;

        // Handle client 1
        while (!valid1) {
            memset(buffer1, 0, sizeof(buffer1));
            int bytes_received = recv(client1, buffer1, BUFFER_SIZE - 1, 0);
            if (bytes_received <= 0) {
                printf("[Room %d] Client 1 disconnected\n", room_id);
                return;
            }
            buffer1[strcspn(buffer1, "\n\r")] = 0;

            int result = evaluate_expression(buffer1, allowed_ops);
            if (result == -2) {
                send(client1, "Invalid operator used. Try again: ", 35, 0);
            } else if (result == target) {
                valid1 = 1;
                send(client1, "Correct!\n", 9, 0);
            } else {
                send(client1, "Incorrect, try again: ", 22, 0);
            }
        }

        // Handle client 2
        while (!valid2) {
            memset(buffer2, 0, sizeof(buffer2));
            int bytes_received = recv(client2, buffer2, BUFFER_SIZE - 1, 0);
            if (bytes_received <= 0) {
                printf("[Room %d] Client 2 disconnected\n", room_id);
                return;
            }
            buffer2[strcspn(buffer2, "\n\r")] = 0;

            int result = evaluate_expression(buffer2, allowed_ops);
            if (result == -2) {
                send(client2, "Invalid operator used. Try again: ", 35, 0);
            } else if (result == target) {
                valid2 = 1;
                send(client2, "Correct!\n", 9, 0);
            } else {
                send(client2, "Incorrect, try again: ", 22, 0);
            }
        }

        int score1 = strlen(buffer1) > 0 ? score_of_expression(buffer1) : 0;
        int score2 = strlen(buffer2) > 0 ? score_of_expression(buffer2) : 0;

        total_score1 += score1;
        total_score2 += score2;

        snprintf(msg, sizeof(msg), "Your score this round: %d (Total: %d)\n", score1, total_score1);
        send(client1, msg, strlen(msg), 0);
        snprintf(msg, sizeof(msg), "Your score this round: %d (Total: %d)\n", score2, total_score2);
        send(client2, msg, strlen(msg), 0);

        printf("[Room %d] Round %d: C1=%s (%d), C2=%s (%d)\n", room_id, round, buffer1, score1, buffer2, score2);
    }

    // Get total time from both clients
    char time_buf1[BUFFER_SIZE], time_buf2[BUFFER_SIZE];
    send(client1, "Send your total time in ms: ", 29, 0);
    send(client2, "Send your total time in ms: ", 29, 0);

    memset(time_buf1, 0, sizeof(time_buf1));
    memset(time_buf2, 0, sizeof(time_buf2));

    if (recv(client1, time_buf1, BUFFER_SIZE - 1, 0) <= 0 || 
        recv(client2, time_buf2, BUFFER_SIZE - 1, 0) <= 0) {
        printf("[Room %d] Client disconnected during time submission\n", room_id);
        return;
    }

    time_buf1[strcspn(time_buf1, "\r\n")] = 0;
    time_buf2[strcspn(time_buf2, "\r\n")] = 0;

    long time1 = atol(time_buf1);
    long time2 = atol(time_buf2);

    char final[BUFFER_SIZE * 2];
    snprintf(final, sizeof(final),
        "\n=== FINAL RESULTS ===\nClient 1: %d points\nClient 2: %d points\nClient 1 time: %ld ms\nClient 2 time: %ld ms\n",
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

    strcat(final, (total_score1 > total_score2) ? "🎉 Client 1 wins!\n" :
                  (total_score1 < total_score2) ? "🎉 Client 2 wins!\n" : "🤝 It's a tie!\n");
    strcat(final, "Game over. Thanks for playing!\n");

    send(client1, final, strlen(final), 0);
    send(client2, final, strlen(final), 0);

    printf("[Room %d] Game completed: %s", room_id, final);
}

// Thread function for handling a game room
void* game_thread(void* arg) {
    game_room_t* room = (game_room_t*)arg;
    
    // Send start message to both clients
    const char *start_msg = "🎮 Game started! Get ready for 5 rounds of math challenges!\n";
    send(room->client1, start_msg, strlen(start_msg), 0);
    send(room->client2, start_msg, strlen(start_msg), 0);

    // Play the game
    play_game(room->client1, room->client2, room->room_id);

    // Clean up
    close(room->client1);
    close(room->client2);
    free(room);
    
    printf("[Room %d] Thread finished and cleaned up\n", room->room_id);
    pthread_exit(NULL);
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr;
    int opt = 1;

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to reuse address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 20) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("🚀 Math Game Server is running on port %d...\n", PORT);
    printf("Waiting for players to connect (2 players per game room)...\n");

    while (1) {
        int clients[MAX_CLIENTS_PER_ROOM];
        int count = 0;

        // Wait for 2 clients to connect
        printf("\n--- Waiting for 2 players for new game room ---\n");
        
        while (count < MAX_CLIENTS_PER_ROOM) {
            int new_socket = accept(server_fd, NULL, NULL);
            if (new_socket < 0) {
                perror("Accept failed");
                continue;
            }
            
            clients[count] = new_socket;
            count++;
            printf("Player %d connected (socket %d)\n", count, new_socket);
            
            // Send waiting message
            if (count == 1) {
                const char *wait_msg = "⏳ Waiting for another player to join...\n";
                send(new_socket, wait_msg, strlen(wait_msg), 0);
            }
        }

        // Create room structure
        game_room_t* room = malloc(sizeof(game_room_t));
        if (!room) {
            perror("Failed to allocate memory for game room");
            close(clients[0]);
            close(clients[1]);
            continue;
        }

        // Get unique room ID
        pthread_mutex_lock(&room_counter_mutex);
        room->room_id = ++room_counter;
        pthread_mutex_unlock(&room_counter_mutex);
        
        room->client1 = clients[0];
        room->client2 = clients[1];

        // Create thread for this game room
        pthread_t game_tid;
        if (pthread_create(&game_tid, NULL, game_thread, room) != 0) {
            perror("Failed to create game thread");
            close(clients[0]);
            close(clients[1]);
            free(room);
            continue;
        }

        // Detach thread so it cleans up automatically
        pthread_detach(game_tid);
        
        printf("🎮 Game room %d started with players %d and %d\n", 
               room->room_id, clients[0], clients[1]);
    }

    close(server_fd);
    pthread_mutex_destroy(&room_counter_mutex);
    return 0;
}