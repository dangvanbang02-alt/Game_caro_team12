#include "SaveGame.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <direct.h>
    #define SG_MKDIR(path) _mkdir(path)
#else
    #include <sys/stat.h>
    #define SG_MKDIR(path) mkdir(path, 0755)
#endif

static FILE *sg_open(const char *name, const char *mode)
{
    FILE *file = NULL;

#ifdef _MSC_VER
    if (fopen_s(&file, name, mode) != 0) {
        return NULL;
    }
#else
    file = fopen(name, mode);
#endif

    return file;
}

static int sg_validPlayer(int player)
{
    return player == SAVE_CELL_X || player == SAVE_CELL_O;
}

static int sg_validate(const GameState *game)
{
    int i;
    int j;
    int k;

    if (game == NULL) return 0;
    if (game->size < 3 || game->size > MAX_SIZE) return 0;
    if (game->winLength < 3 || game->winLength > game->size) return 0;
    if (!sg_validPlayer(game->currentPlayer)) return 0;
    if (game->mode != MODE_PVP && game->mode != MODE_PVE) return 0;
    if (game->botLevel < LEVEL_EASY || game->botLevel > LEVEL_HARD) return 0;
    if (game->moveCount < 0 || game->moveCount > game->size * game->size) return 0;

    for (i = 0; i < game->size; ++i) {
        for (j = 0; j < game->size; ++j) {
            int cell = game->board[i][j];

            if (cell != SAVE_CELL_EMPTY &&
                cell != SAVE_CELL_X &&
                cell != SAVE_CELL_O) {
                return 0;
            }
        }
    }

    for (k = 0; k < game->moveCount; ++k) {
        const Move *move = &game->history[k];

        if (move->row < 0 || move->row >= game->size) return 0;
        if (move->col < 0 || move->col >= game->size) return 0;
        if (!sg_validPlayer(move->player)) return 0;
    }

    return 1;
}

static void sg_slotPath(char *buffer, size_t bufferSize, int slot)
{
    (void)snprintf(buffer, bufferSize, "%s/slot%d.sav", SAVE_DIR, slot);
}

void initGame(GameState *game, int size, int winLength, int mode, int botLevel)
{
    if (game == NULL) return;

    memset(game, 0, sizeof(*game));
    game->size = size;
    game->winLength = winLength;
    game->mode = mode;
    game->botLevel = botLevel;
    game->currentPlayer = SAVE_CELL_X;
}

int applyMove(GameState *game, int row, int col)
{
    if (game == NULL) return 0;
    if (row < 0 || row >= game->size || col < 0 || col >= game->size) return 0;
    if (game->board[row][col] != SAVE_CELL_EMPTY) return 0;
    if (game->moveCount >= MAX_MOVES || game->moveCount >= game->size * game->size) return 0;
    game->board[row][col] = game->currentPlayer;
    game->history[game->moveCount].row = row;
}