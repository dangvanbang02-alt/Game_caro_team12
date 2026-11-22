#include "LoadGame.h"

#include <string.h>

static int lg_aiDepth(int botLevel)
{
    switch (botLevel) {
    case LEVEL_EASY: return 1;
    case LEVEL_HARD: return 4;
    default:         return 2;
    }
}

static Cell lg_toCell(int saveCell)
{
    if (saveCell == SAVE_CELL_X) return CELL_X;
    if (saveCell == SAVE_CELL_O) return CELL_O;
    return CELL_EMPTY;
}

SaveResult LoadGame_fromState(Game *game, const GameState *state)
{
    static Game temp; 
    int i;
    int row;
    int col;

    if (game == NULL || state == NULL) return SAVE_ERR_DATA;

 
    if (state->size != BOARD_SIZE || state->winLength != WIN_LENGTH) {
        return SAVE_ERR_DATA;
    }
    if (state->moveCount < 0 || state->moveCount > MAX_REPLAY_MOVES) {
        return SAVE_ERR_DATA;
    }

    Game_init(&temp,
              state->mode == MODE_PVE ? GAME_MODE_PVE : GAME_MODE_PVP,
              state->botLevel == LEVEL_HARD ? AI_DIFFICULTY_HARD : AI_DIFFICULTY_MEDIUM);
    temp.config.aiDepth = lg_aiDepth(state->botLevel);

    for (i = 0; i < state->moveCount; ++i) {
        const SaveMove *mv = &state->history[i];
        Cell piece = lg_toCell(mv->player);

        if (piece == CELL_EMPTY ||
            !Board_makeMove(&temp.board, mv->row, mv->col, piece) ||
            !MoveHistory_push(&temp.history, mv->row, mv->col, piece)) {
            return SAVE_ERR_DATA;
        }
        Stats_recordMove(piece == CELL_X ? &temp.statsX : &temp.statsO);
    }

    for (row = 0; row < BOARD_SIZE; ++row) {
        for (col = 0; col < BOARD_SIZE; ++col) {
            if (temp.board.cells[row][col] != lg_toCell(state->board[row][col])) {
                return SAVE_ERR_DATA;
            }
        }
    }

    temp.currentPlayer = lg_toCell(state->currentPlayer);
    temp.aiPlayer      = CELL_O;   
    temp.result        = WinCheck_check(&temp.board);

    *game = temp;
    return SAVE_OK;
}

SaveResult LoadGame_fromFile(Game *game, const char *filename)
{
    static GameState state;
    SaveResult result;

    if (game == NULL) return SAVE_ERR_DATA;

    result = loadGame(&state, filename);
    if (result != SAVE_OK) return result;

    return LoadGame_fromState(game, &state);
}

SaveResult LoadGame_fromSlot(Game *game, int slot)
{
    static GameState state;
    SaveResult result;

    if (game == NULL) return SAVE_ERR_DATA;

    result = loadFromSlot(&state, slot);
    if (result != SAVE_OK) return result;

    return LoadGame_fromState(game, &state);
}
