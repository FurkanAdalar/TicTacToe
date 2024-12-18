// Client Code: tic_tac_toe_client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8081

void client_print_board(int board[3][3]) {
    printf("\n");
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == -1) printf(" . ");
            else printf(" %c ", board[i][j] == 0 ? 'X' : 'O');
        }
        printf("\n");
    }
    printf("\n");
}

int main() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed\n");
        return -1;
    }

    printf("Connected to the game server.\n");

    while (1) {
        int board[3][3];
        printf("Waiting for the game state...\n");

        valread = read(sock, buffer, sizeof(buffer));
        if (valread <= 0) {
            printf("Connection lost.\n");
            break;
        }

        memcpy(board, buffer, sizeof(board));
        client_print_board(board);

        char game_state = buffer[sizeof(board)];
        if (game_state == 'W') {
            printf("You win!\n");
            break;
        } else if (game_state == 'L') {
            printf("You lose.\n");
            break;
        } else if (game_state == 'D') {
            printf("It's a draw!\n");
            break;
        }

        if (game_state == 'T') {
            char player_turn = buffer[sizeof(board) + 1];
            if (player_turn == '1') {
                int row, col;
                printf("Your turn! Enter your move (row and column): ");
                scanf("%d %d", &row, &col);

                char move[2] = {row + '0', col + '0'};
                send(sock, move, sizeof(move), 0);
            } else {
                printf("Waiting for the opponent's move...\n");
            }
        }
    }

    close(sock);
    return 0;
}
