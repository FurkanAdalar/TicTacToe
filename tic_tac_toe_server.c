// Server Code: tic_tac_toe_server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

#define PORT 8081
#define MAX_CLIENTS 2
#define BOARD_SIZE 3

int board[BOARD_SIZE][BOARD_SIZE];
int current_player = 0;

void init_board() {
    memset(board, -1, sizeof(board));
}

void server_print_board() {
    printf("\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == -1) printf(" . ");
            else printf(" %c ", board[i][j] == 0 ? 'X' : 'O');
        }
        printf("\n");
    }
    printf("\n");
}

int check_winner() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        // Check rows and columns
        if (board[i][0] != -1 && board[i][0] == board[i][1] && board[i][1] == board[i][2]) return board[i][0];
        if (board[0][i] != -1 && board[0][i] == board[1][i] && board[1][i] == board[2][i]) return board[0][i];
    }
    // Check diagonals
    if (board[0][0] != -1 && board[0][0] == board[1][1] && board[1][1] == board[2][2]) return board[0][0];
    if (board[0][2] != -1 && board[0][2] == board[1][1] && board[1][1] == board[2][0]) return board[0][2];
    return -1;
}

int is_draw() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == -1) return 0;
        }
    }
    return 1;
}

void send_game_state(int client_fds[MAX_CLIENTS]) {
    char buffer[1024] = {0};
    memcpy(buffer, board, sizeof(board));

    if (check_winner() != -1) {
        buffer[sizeof(board)] = 'W';  // Winner
    } else if (is_draw()) {
        buffer[sizeof(board)] = 'D';  // Draw
    } else {
        buffer[sizeof(board)] = 'T';  // In Progress
    }

    // Send turn information (which player's turn it is)
    buffer[sizeof(board) + 1] = (current_player == 0) ? '1' : '2';

    // Send game state to both players
    for (int i = 0; i < MAX_CLIENTS; i++) {
        send(client_fds[i], buffer, sizeof(buffer), 0);
    }
}

int main() {
    int server_fd, client_fds[MAX_CLIENTS] = {0}, max_sd, activity;
    struct sockaddr_in address;
    fd_set readfds;

    // Create the socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {//stream sockets : uses tcp connection oriented .. AF_INET (ARPA Internet protocols)   ----Over the network via IPv4 
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket with an IP and Port number
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening on the socket
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Server created succesfully!");
    printf("Waiting for players to connect...\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Add the client sockets to the select() call
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] > 0) FD_SET(client_fds[i], &readfds);
            if (client_fds[i] > max_sd) max_sd = client_fds[i];
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        // Accept new connections
        if (FD_ISSET(server_fd, &readfds)) {
            int new_socket, addrlen = sizeof(address);
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {//Addrlen: The length of the field where the address information is kept
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            printf("Player connected.\n");

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_fds[i] == 0) {
                    client_fds[i] = new_socket;
                    break;
                }
            }

            // Start game if all players are connected
            if (client_fds[0] > 0 && client_fds[1] > 0) {
                printf("Both players connected. Starting the game...\n");
                init_board();
                send_game_state(client_fds);
            }
        }

        // Handle incoming messages
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_fds[i];
            if (FD_ISSET(sd, &readfds)) {
                char buffer[1024] = {0};
                int valread = read(sd, buffer, sizeof(buffer));

                if (valread == 0) {
                    printf("Player disconnected.\n");
                    close(sd); // closes the connection
                    client_fds[i] = 0;
                } else {
                    int row = buffer[0] - '0';
                    int col = buffer[1] - '0';

                    if (board[row][col] == -1 && i == current_player) {
                        board[row][col] = current_player;
                        current_player = (current_player + 1) % MAX_CLIENTS;
                        server_print_board();
                        send_game_state(client_fds);
                    }
                }
            }
        }
    }

    return 0;
}
