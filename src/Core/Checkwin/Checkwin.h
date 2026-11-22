#ifndef CHECKWIN_H
#define CHECKWIN_H
#include <stdio.h>
#include "Board.h"
typedef enum {
    GAME_RESULT_ONGOING = 2,
    GAME_RESULT_DRAW = 0,
    GAME_RESULT_X_WINS = -1,
    GAME_RESULT_O_WINS = 1
} GameResult;
GameResult WinCheck_check(const Board* board);
GameResult WinCheck_checkFrom(const Board* board, int row, int col);
int WinCheck_hasFiveFrom(const Board* board, int row, int col);
#endif
