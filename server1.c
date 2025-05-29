#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#define PORT 8080
#define MAX_PLAYERS 2

// This holds info about each game
struct Game {
    int player1_socket;
    int player2_socket;
    int game_number;
};

// Function to get current time in seconds
double get_current_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// This function runs the math game between 2 players
void* play_math_game(void* game_info) {
    struct Game* game = (struct Game*)game_info;
    int p1 = game->player1_socket;  // Player 1
    int p2 = game->player2_socket;  // Player 2
    int game_num = game->game_number;
    
    printf("Game %d started!\n", game_num);
    
    // Tell both players the game started
    char msg[100];
    sprintf(msg, "Welcome to Math Game %d! Let's play!\n", game_num);
    send(p1, msg, strlen(msg), 0);
    send(p2, msg, strlen(msg), 0);
    
    int p1_score = 0;
    int p2_score = 0;
    double p1_total_time = 0;  // Total time for player 1
    double p2_total_time = 0;  // Total time for player 2
    
    // Play 3 rounds
    for(int round = 1; round <= 3; round++) {
        // Make a simple math problem
        int num1 = rand() % 10 + 1;  // Numbers 1-10
        int num2 = rand() % 10 + 1;
        int answer = num1 + num2;
        
        // Send problem to both players and record start time
        sprintf(msg, "Round %d: What is %d + %d? ", round, num1, num2);
        double start_time = get_current_time();
        send(p1, msg, strlen(msg), 0);
        send(p2, msg, strlen(msg), 0);
        
        // Get answers from both players
        char p1_answer[10], p2_answer[10];
        recv(p1, p1_answer, sizeof(p1_answer), 0);
        double p1_end_time = get_current_time();
        double p1_time = p1_end_time - start_time;
        
        recv(p2, p2_answer, sizeof(p2_answer), 0);
        double p2_end_time = get_current_time();
        double p2_time = p2_end_time - start_time;
        
        // Add to total times
        p1_total_time += p1_time;
        p2_total_time += p2_time;
        
        // Check who got it right
        int p1_num = atoi(p1_answer);  // Convert string to number
        int p2_num = atoi(p2_answer);
        
        if(p1_num == answer) {
            p1_score++;
            sprintf(msg, "Correct! +1 point (%.2f seconds)\n", p1_time);
            send(p1, msg, strlen(msg), 0);
        } else {
            sprintf(msg, "Wrong answer! (%.2f seconds)\n", p1_time);
            send(p1, msg, strlen(msg), 0);
        }
        
        if(p2_num == answer) {
            p2_score++;
            sprintf(msg, "Correct! +1 point (%.2f seconds)\n", p2_time);
            send(p2, msg, strlen(msg), 0);
        } else {
            sprintf(msg, "Wrong answer! (%.2f seconds)\n", p2_time);
            send(p2, msg, strlen(msg), 0);
        }
        
        printf("Game %d Round %d: P1=%d(%.2fs) P2=%d(%.2fs) (answer was %d)\n", 
               game_num, round, p1_num, p1_time, p2_num, p2_time, answer);
    }
    
    // Tell players the final scores with time
    sprintf(msg, "GAME OVER! Your score: %d | Total time: %.2f seconds\n", p1_score, p1_total_time);
    send(p1, msg, strlen(msg), 0);
    
    sprintf(msg, "GAME OVER! Your score: %d | Total time: %.2f seconds\n", p2_score, p2_total_time);
    send(p2, msg, strlen(msg), 0);
    
    // Check for speed bonus (faster player gets +1 if close scores)
    int speed_bonus_p1 = 0, speed_bonus_p2 = 0;
    if(abs(p1_score - p2_score) <= 1) {  // Close game
        if(p1_total_time < p2_total_time) {
            speed_bonus_p1 = 1;
            p1_score++;
            send(p1, "SPEED BONUS! +1 point for being faster!\n", 41, 0);
            send(p2, "Other player got speed bonus for being faster!\n", 47, 0);
        } else if(p2_total_time < p1_total_time) {
            speed_bonus_p2 = 1;
            p2_score++;
            send(p2, "SPEED BONUS! +1 point for being faster!\n", 41, 0);
            send(p1, "Other player got speed bonus for being faster!\n", 47, 0);
        }
    }
    
    // Announce winner
    if(p1_score > p2_score) {
        send(p1, "You WIN! Great job!\n", 20, 0);
        send(p2, "You lose. Try again!\n", 21, 0);
        printf("Game %d: Player 1 wins %d-%d (Times: %.2fs vs %.2fs)\n", 
               game_num, p1_score, p2_score, p1_total_time, p2_total_time);
    } else if(p2_score > p1_score) {
        send(p1, "You lose. Try again!\n", 21, 0);
        send(p2, "You WIN! Great job!\n", 20, 0);
        printf("Game %d: Player 2 wins %d-%d (Times: %.2fs vs %.2fs)\n", 
               game_num, p2_score, p1_score, p2_total_time, p1_total_time);
    } else {
        send(p1, "It's a TIE! Good game!\n", 23, 0);
        send(p2, "It's a TIE! Good game!\n", 23, 0);
        printf("Game %d: Tie %d-%d (Times: %.2fs vs %.2fs)\n", 
               game_num, p1_score, p2_score, p1_total_time, p2_total_time);
    }
    
    // Close connections and clean up
    close(p1);
    close(p2);
    free(game);
    printf("Game %d finished!\n", game_num);
    
    return NULL;
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr;
    int game_counter = 0;
    
    printf("Starting Math Game Server for Kids!\n");
    
    // Seed random number generator
    srand(time(NULL));
    
    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0) {
        printf("Error: Can't create socket\n");
        return -1;
    }
    
    // Allow socket reuse (helpful for development)
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Accept from any IP
    server_addr.sin_port = htons(PORT);        // Use port 8080
    
    // Bind socket to address
    if(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Error: Can't bind to port\n");
        return -1;
    }
    
    // Listen for connections
    listen(server_socket, 10);
    printf("Server listening on port %d...\n", PORT);
    printf("Waiting for players to join!\n");
    
    // Main server loop - keep accepting players forever
    while(1) {
        int player_sockets[MAX_PLAYERS];
        
        // Wait for 2 players to connect
        printf("\nWaiting for 2 players...\n");
        for(int i = 0; i < MAX_PLAYERS; i++) {
            player_sockets[i] = accept(server_socket, NULL, NULL);
            printf("Player %d connected!\n", i + 1);
            
            char welcome[50];
            sprintf(welcome, "Hello Player %d! Waiting for another player...\n", i + 1);
            send(player_sockets[i], welcome, strlen(welcome), 0);
        }
        
        // Create new game info
        struct Game* new_game = malloc(sizeof(struct Game));
        new_game->player1_socket = player_sockets[0];
        new_game->player2_socket = player_sockets[1];
        new_game->game_number = ++game_counter;
        
        // Start new thread for this game
        pthread_t game_thread;
        pthread_create(&game_thread, NULL, play_math_game, new_game);
        pthread_detach(game_thread);  // Let thread clean up itself
        
        printf("Started new game thread #%d with timing features!\n", game_counter);
    }
    
    close(server_socket);
    return 0;
}