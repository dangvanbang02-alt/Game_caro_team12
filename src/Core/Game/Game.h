#ifndef GAME_H
#define GAME_H
#include "Board.h"
#include "Stats.h"
#include "Checkwin.h"
#include "Player.h"
#include "MoveHistory.h"
#include "MatchHistory.h"
#include "GameConfig.h"
#include "GameTimer.h"

typedef enum { GAME_MODE_PVP=0, GAME_MODE_PVE=1 } GameMode;
typedef enum { AI_DIFFICULTY_MEDIUM=0, AI_DIFFICULTY_HARD=1 } AIDifficulty;
typedef struct {
    Board board;
    Player players[2]; 
    Cell currentPlayer;
    GameMode mode;
    AIDifficulty difficulty;
    GameResult result;
    PlayerStats statsX, statsO;
    GameConfig config;
    GameTimer timer;
    MoveHistory history;
    MatchHistory matchHistory;
    unsigned long elapsedMs;
    int paused;
    int resultRecorded;
} Game;

void Game_init(Game* game, GameMode mode, AIDifficulty difficulty);
void Game_resetRound(Game* game);
int Game_playMove(Game* game, int row, int col);
int Game_undo(Game* game);
int Game_redo(Game* game);
int Game_tick(Game* game, unsigned int elapsedMs);
void Game_pause(Game* game);
void Game_resume(Game* game);
int Game_setPlayerName(Game* game, Cell symbol, const char* name);
int Game_setConfig(Game* game, const GameConfig* config);
int Game_playAIMove(Game* game);
int Game_isFinished(const Game* game);
Cell Game_getCurrentPlayer(const Game* game);
const PlayerStats* Game_getStatsX(const Game* game);
const PlayerStats* Game_getStatsO(const Game* game);
typedef struct { Board board; const MoveHistory* source; int cursor; } GameReplay;
void Replay_init(GameReplay* replay, const MoveHistory* history);
int Replay_step(GameReplay* replay);
int Replay_reset(GameReplay* replay);
#endif
