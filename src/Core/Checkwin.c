#include "WinCheck.h"

static const int DIRECTIONS[4][2] = {
    { 0, 1 },   /* ngang */
    { 1, 0 },   /* doc */
    { 1, 1 },   /* cheo */
    { 1, -1 }   /* cheo */
};

static int countDirection(const Board* board, int row, int col,
    int dr, int dc, Cell player) {
    int count = 0;
    row += dr;
    col += dc;

    while (Board_isInside(row, col) &&
        board->cells[row][col] == player) {
        ++count;
        row += dr;
        col += dc;
    }
    return count;
}

int WinCheck_hasFiveFrom(const Board* board, int row, int col) {
    int direction;
    Cell player;

    if (board == 0 || !Board_isInside(row, col)) return 0;
    player = board->cells[row][col];
    if (player == CELL_EMPTY) return 0;

    for (direction = 0; direction < 4; ++direction) {
        int dr = DIRECTIONS[direction][0];
        int dc = DIRECTIONS[direction][1];
        int total = 1
            + countDirection(board, row, col, dr, dc, player)
            + countDirection(board, row, col, -dr, -dc, player);

        if (total >= WIN_LENGTH) return 1;
    }
    return 0;
}

GameResult WinCheck_checkFrom(const Board* board, int row, int col) {
    if (board == 0 || !Board_isInside(row, col)) return GAME_RESULT_ONGOING;
    if (board->cells[row][col] == CELL_X &&
        WinCheck_hasFiveFrom(board, row, col)) return GAME_RESULT_X_WINS;
    if (board->cells[row][col] == CELL_O &&
        WinCheck_hasFiveFrom(board, row, col)) return GAME_RESULT_O_WINS;
    if (Board_isFull(board)) return GAME_RESULT_DRAW;
    return GAME_RESULT_ONGOING;
}

GameResult WinCheck_check(const Board* board) {
    int row, col;
    if (board == 0) return GAME_RESULT_ONGOING;

    for (row = 0; row < BOARD_SIZE; ++row) {
        for (col = 0; col < BOARD_SIZE; ++col) {
            if (board->cells[row][col] != CELL_EMPTY &&
                WinCheck_hasFiveFrom(board, row, col)) {
                return board->cells[row][col] == CELL_X
                    ? GAME_RESULT_X_WINS : GAME_RESULT_O_WINS;
            }
        }
    }

    return Board_isFull(board) ? GAME_RESULT_DRAW : GAME_RESULT_ONGOING;
}
