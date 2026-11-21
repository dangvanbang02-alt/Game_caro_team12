#include "Board.h"

void Board_init(Board* board) { Board_clear(board); }
void Board_clear(Board* board) {
    int r, c;
    if (!board) return;
    for (r = 0; r < BOARD_SIZE; ++r)
        for (c = 0; c < BOARD_SIZE; ++c) board->cells[r][c] = CELL_EMPTY;
}
int Board_isInside(int row, int col) {
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
}
int Board_isValidMove(const Board* board, int row, int col) {
    return board && Board_isInside(row, col) && board->cells[row][col] == CELL_EMPTY;
}
int Board_makeMove(Board* board, int row, int col, Cell player) {
    if (!Board_isValidMove(board, row, col) || (player != CELL_X && player != CELL_O)) return 0;
    board->cells[row][col] = player;
    return 1;
}
int Board_isFull(const Board* board) {
    int r, c;
    if (!board) return 0;
    for (r = 0; r < BOARD_SIZE; ++r)
        for (c = 0; c < BOARD_SIZE; ++c)
            if (board->cells[r][c] == CELL_EMPTY) return 0;
    return 1;
}
int Board_countMoves(const Board* board) {
    int r, c, count = 0;
    if (!board) return 0;
    for (r = 0; r < BOARD_SIZE; ++r)
        for (c = 0; c < BOARD_SIZE; ++c)
            if (board->cells[r][c] != CELL_EMPTY) ++count;
    return count;
}
