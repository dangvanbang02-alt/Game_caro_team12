#ifndef BOARD_H
#define BOARD_H

#include "Config.h"

typedef enum {
    CELL_EMPTY = 0,
    CELL_X = -1,
    CELL_O = 1
} Cell;

typedef struct {
    Cell cells[BOARD_SIZE][BOARD_SIZE];
} Board;

void Board_init(Board* board);
void Board_clear(Board* board);
int Board_isInside(int row, int col);
int Board_isValidMove(const Board* board, int row, int col);
int Board_makeMove(Board* board, int row, int col, Cell player);
int Board_isFull(const Board* board);
int Board_countMoves(const Board* board);

#endif
