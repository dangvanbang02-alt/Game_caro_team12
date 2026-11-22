#include "Checkwin.h"
#include <stddef.h>


static const int DIRS[4][2] = {
    {0, 1},
    {1, 0},
    {1, 1},
    {1, -1}
};

static int countDirection(
    const Board* board,
    int row,
    int col,
    int dRow,
    int dCol,
    Cell player
) {
    int count = 0;

    row += dRow;
    col += dCol;

    while (Board_isInside(row, col) &&
           board->cells[row][col] == player) {
        ++count;
        row += dRow;
        col += dCol;
    }

    return count;
}

int WinCheck_hasFiveFrom(const Board* board, int row, int col) {
    int d;
    Cell player;

    if (board == NULL || !Board_isInside(row, col)) {
        return 0;
    }

    player = board->cells[row][col];

    if (player == CELL_EMPTY) {
        return 0;
    }

    for (d = 0; d < 4; ++d) {
        int dRow = DIRS[d][0];
        int dCol = DIRS[d][1];

        int countForward = countDirection(
            board, row, col, dRow, dCol, player
        );

        int countBackward = countDirection(
            board, row, col, -dRow, -dCol, player
        );

        int total = 1 + countForward + countBackward;

        if (total >= WIN_LENGTH) {
            return 1;
        }
    }
    return 0;
}

GameResult WinCheck_checkFrom(const Board* board, int row, int col) {
    if (board == NULL || !Board_isInside(row, col)) {
        return GAME_RESULT_ONGOING;
    }

    if (board->cells[row][col] == CELL_X &&
        WinCheck_hasFiveFrom(board, row, col)) {
        return GAME_RESULT_X_WINS;
    }

    if (board->cells[row][col] == CELL_O &&
        WinCheck_hasFiveFrom(board, row, col)) {
        return GAME_RESULT_O_WINS;
    }

    return Board_isFull(board)
        ? GAME_RESULT_DRAW
        : GAME_RESULT_ONGOING;
}

GameResult WinCheck_check(const Board* board) {
    int row;
    int col;

    if (board == NULL) {
        return GAME_RESULT_ONGOING;
    }

    for (row = 0; row < BOARD_SIZE; ++row) {
        for (col = 0; col < BOARD_SIZE; ++col) {
            if (board->cells[row][col] != CELL_EMPTY &&
                WinCheck_hasFiveFrom(board, row, col)) {
                return board->cells[row][col] == CELL_X
                    ? GAME_RESULT_X_WINS
                    : GAME_RESULT_O_WINS;
            }
        }
    }

    return Board_isFull(board)
        ? GAME_RESULT_DRAW
        : GAME_RESULT_ONGOING;
}
