#ifndef SAVE_GAME_H
#define SAVE_GAME_H

#include <stddef.h>

#define MAX_SIZE 20
#define MAX_MOVES (MAX_SIZE * MAX_SIZE)
#define SAVE_DIR "saves"
#define SAVE_MAGIC     "CARO_SAVE"
#define SAVE_VERSION   1
#define SAVE_CELL_EMPTY 0
#define SAVE_CELL_X     1
#define SAVE_CELL_O     2
#define MODE_PVP 0
#define MODE_PVE 1
#define LEVEL_EASY   0
#define LEVEL_MEDIUM 1
#define LEVEL_HARD   2

typedef struct {
    int row;
    int col;
    int player;
} SaveMove;

typedef struct {
    int size;
    int board[MAX_SIZE][MAX_SIZE];
    int currentPlayer;
    int mode;
    int botLevel;
    int winLength;
    int moveCount;
    SaveMove history[MAX_MOVES];
} GameState;

typedef enum {
    SAVE_OK = 0,
    SAVE_ERR_OPEN,
    SAVE_ERR_FORMAT,
    SAVE_ERR_VERSION,
    SAVE_ERR_DATA,
    SAVE_ERR_NOT_FOUND
} SaveResult;

void initGame(GameState *game, int size, int winLength, int mode, int botLevel);
int applyMove(GameState *game, int row, int col);

SaveResult saveGame(const GameState *game, const char *filename);
SaveResult loadGame(GameState *game, const char *filename);

SaveResult saveToSlot(const GameState *game, int slot);
SaveResult loadFromSlot(GameState *game, int slot);
int slotExists(int slot);

const char *saveResultMessage(SaveResult result);

#endif