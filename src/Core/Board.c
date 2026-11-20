#include "Board.h"

void Board_init(Board* board) {
    Board_clear(board);
}

void Board_clear(Board* board) {
    int row, col;
    if (board == 0) return;

    for (row = 0; row < BOARD_SIZE; ++row) {
        for (col = 0; col < BOARD_SIZE; ++col) {
            board->cells[row][col] = CELL_EMPTY;
        }
    }
}

int Board_isInside(int row, int col) {
    return row >= 0 && row < BOARD_SIZE &&
        col >= 0 && col < BOARD_SIZE;
}

int Board_isValidMove(const Board* board, int row, int col) {
    return board != 0 && Board_isInside(row, col) &&
        board->cells[row][col] == CELL_EMPTY;
}

int Board_makeMove(Board* board, int row, int col, Cell player) {
    if (!Board_isValidMove(board, row, col)) return 0;
    if (player != CELL_X && player != CELL_O) return 0;

    board->cells[row][col] = player;
    return 1;
}

int Board_isFull(const Board* board) {
    int row, col;
    if (board == 0) return 0;

    for (row = 0; row < BOARD_SIZE; ++row) {
        for (col = 0; col < BOARD_SIZE; ++col) {
            if (board->cells[row][col] == CELL_EMPTY) return 0;
        }
    }
    return 1;
}

int Board_countMoves(const Board* board) {
    int row, col, count = 0;
    if (board == 0) return 0;

    for (row = 0; row < BOARD_SIZE; ++row) {
        for (col = 0; col < BOARD_SIZE; ++col) {
            if (board->cells[row][col] != CELL_EMPTY) ++count;
        }
    }
    return count;
}
