#ifndef GAME_H
#define GAME_H

#include "Board.h"
#include "Stats.h"
#include "WinCheck.h"

typedef enum {
    GAME_MODE_PVP = 0,
    GAME_MODE_PVE = 1
} GameMode;

typedef enum {
    AI_DIFFICULTY_MEDIUM = 0,
    AI_DIFFICULTY_HARD = 1
} AIDifficulty;

typedef struct {
    Board board;
    Cell currentPlayer;
    GameMode mode;
    AIDifficulty difficulty;
    GameResult result;
    PlayerStats statsX;
    PlayerStats statsO;
} Game;

void Game_init(Game* game, GameMode mode, AIDifficulty difficulty);
void Game_resetRound(Game* game);
int Game_playMove(Game* game, int row, int col);
int Game_isFinished(const Game* game);
Cell Game_getCurrentPlayer(const Game* game);
const PlayerStats* Game_getStatsX(const Game* game);
const PlayerStats* Game_getStatsO(const Game* game);

#endif
