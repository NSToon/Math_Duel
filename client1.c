// ================= client.c ================= 
// Simple Math Game Client for Kids!
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int my_socket;
    struct sockaddr_in server_addr;
    char message[100];
    char my_answer[10];
    
    printf("=== KIDS MATH GAME ===\n");
    printf("Connecting to game server...\n");
    
    // Create socket
    my_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(my_socket < 0) {
        printf("Error: Can't create socket\n");
        return -1;
    }
    
    // Set up server address (connect to localhost)
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    // Connect to server
    if(connect(my_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Error: Can't connect to server\n");
        return -1;
    }
    
    printf("Connected! Waiting for game to start...\n\n");
    
    // Main game loop
    while(1) {
        // Clear message buffer
        memset(message, 0, sizeof(message));
        
        // Get message from server
        int bytes_received = recv(my_socket, message, sizeof(message) - 1, 0);
        
        if(bytes_received <= 0) {
            printf("Server disconnected. Goodbye!\n");
            break;
        }
        
        // Print what server sent us
        printf("%s", message);

        
        // If server is asking a math question, get our answer
        if(strstr(message, "What is")) {  // Look for math questions
            printf("Your answer: ");
            fgets(my_answer, sizeof(my_answer), stdin);
            
            // Remove the newline character
            my_answer[strcspn(my_answer, "\n")] = 0;
            
            // Send our answer to server
            send(my_socket, my_answer, strlen(my_answer), 0);
        }

        if (strstr(message, "You WIN!")) {
            printf("🎉 Congratulations! You are the winner!\n");
        } else if (strstr(message, "You lose")) {
            printf("😢 Better luck next time!\n");
        } else if (strstr(message, "It's a TIE")) {
            printf("🤝 It's a tie! Good game!\n");
        }
        
        // If game is over, quit
        if(strstr(message, "GAME OVER")) {
            printf("\nThanks for playing!\n");
            sleep(2);  // Wait 2 seconds before closing
            break;
        }

    }
    
    close(my_socket);
    return 0;
}