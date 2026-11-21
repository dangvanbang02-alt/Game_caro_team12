#ifndef BOT_H
#define BOT_H

#ifndef BOARD_SIZE
#define BOARD_SIZE 15  
#endif

#ifndef WIN_LEN
#define WIN_LEN 5   
#endif

#define CELL_EMPTY 0
#define CELL_X     1
#define CELL_O     2

typedef enum {
    BOT_EASY   = 1,   
    BOT_MEDIUM = 2,   
    BOT_HARD   = 3 
} BotLevel;

typedef struct {
    int row;
    int col;
} BotMove;

BotMove bot_get_move(int board[BOARD_SIZE][BOARD_SIZE], int botPlayer, BotLevel level);

int bot_check_win(int board[BOARD_SIZE][BOARD_SIZE], int player);

int bot_board_full(int board[BOARD_SIZE][BOARD_SIZE]);

#endif 
