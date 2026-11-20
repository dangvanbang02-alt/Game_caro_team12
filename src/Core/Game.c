#include "Game.h"

void Game_init(Game* game, GameMode mode, AIDifficulty difficulty) {
    if (game == 0) return;

    Board_init(&game->board);
    game->currentPlayer = CELL_X;
    game->mode = mode;
    game->difficulty = difficulty;
    game->result = GAME_RESULT_ONGOING;
    Stats_reset(&game->statsX);
    Stats_reset(&game->statsO);
}

void Game_resetRound(Game* game) {
    if (game == 0) return;

    Board_clear(&game->board);
    game->currentPlayer = CELL_X;
    game->result = GAME_RESULT_ONGOING;
    Stats_reset(&game->statsX);
    Stats_reset(&game->statsO);
}

int Game_playMove(Game* game, int row, int col) {
    PlayerStats* currentStats;
    PlayerStats* otherStats;
    GameResult result;

    if (game == 0 || game->result != GAME_RESULT_ONGOING) return 0;

    if (!Board_makeMove(&game->board, row, col, game->currentPlayer))
        return 0;

    currentStats = game->currentPlayer == CELL_X
        ? &game->statsX : &game->statsO;
    otherStats = game->currentPlayer == CELL_X
        ? &game->statsO : &game->statsX;

    Stats_recordMove(currentStats);
    result = WinCheck_checkFrom(&game->board, row, col);

    if (result != GAME_RESULT_ONGOING) {
        game->result = result;
        if (result == GAME_RESULT_X_WINS)
            Stats_recordWin(&game->statsX, &game->statsO);
        else if (result == GAME_RESULT_O_WINS)
            Stats_recordWin(&game->statsO, &game->statsX);
        else
            Stats_recordDraw(&game->statsX, &game->statsO);
        return 1;
    }

    (void)otherStats;
    game->currentPlayer = game->currentPlayer == CELL_X ? CELL_O : CELL_X;
    return 1;
}

int Game_isFinished(const Game* game) {
    return game != 0 && game->result != GAME_RESULT_ONGOING;
}

Cell Game_getCurrentPlayer(const Game* game) {
    return game != 0 ? game->currentPlayer : CELL_EMPTY;
}

const PlayerStats* Game_getStatsX(const Game* game) {
    return game != 0 ? &game->statsX : 0;
}

const PlayerStats* Game_getStatsO(const Game* game) {
    return game != 0 ? &game->statsO : 0;
}
