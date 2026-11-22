#include "SaveGame.h"

#include <stdio.h>
#include <stdlib.h>
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
        const SaveMove *move = &game->history[k];

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
    game->history[game->moveCount].col = col;
    game->history[game->moveCount].player = game->currentPlayer;
    game->moveCount++;
    game->currentPlayer = (game->currentPlayer == SAVE_CELL_X) ? SAVE_CELL_O : SAVE_CELL_X;

    return 1;
}

SaveResult saveGame(const GameState *game, const char *filename)
{
    FILE *file;
    int i;
    int j;
    int failed = 0;

    if (filename == NULL || !sg_validate(game)) return SAVE_ERR_DATA;

    file = sg_open(filename, "w");
    if (file == NULL) return SAVE_ERR_OPEN;

    fprintf(file, "%s %d\n", SAVE_MAGIC, SAVE_VERSION);
    fprintf(file, "%d %d %d %d %d %d\n",
            game->size, game->winLength, game->mode,
            game->botLevel, game->currentPlayer, game->moveCount);

    for (i = 0; i < game->size; ++i) {
        for (j = 0; j < game->size; ++j) {
            fprintf(file, j + 1 < game->size ? "%d " : "%d", game->board[i][j]);
        }
        fputc('\n', file);
    }

    for (i = 0; i < game->moveCount; ++i) {
        fprintf(file, "%d %d %d\n",
                game->history[i].row, game->history[i].col, game->history[i].player);
    }

    if (ferror(file)) failed = 1;
    if (fclose(file) != 0) failed = 1;

    return failed ? SAVE_ERR_OPEN : SAVE_OK;
}

static int sg_readInts(FILE *file, int *out, int count)
{
    char line[512];
    char *cursor;
    int i;

    if (fgets(line, (int)sizeof(line), file) == NULL) return 0;

    cursor = line;
    for (i = 0; i < count; ++i) {
        char *end = NULL;
        long value = strtol(cursor, &end, 10);

        if (end == cursor) return 0;
        out[i] = (int)value;
        cursor = end;
    }

    return 1;
}

SaveResult loadGame(GameState *game, const char *filename)
{
    static GameState temp; 
    FILE *file;
    char line[128];
    size_t magicLen = strlen(SAVE_MAGIC);
    int header[6];
    int version;
    int i;
    int j;
    int stones = 0;

    if (game == NULL || filename == NULL) return SAVE_ERR_DATA;

    file = sg_open(filename, "r");
    if (file == NULL) return SAVE_ERR_NOT_FOUND;

    if (fgets(line, (int)sizeof(line), file) == NULL ||
        strncmp(line, SAVE_MAGIC, magicLen) != 0 ||
        (line[magicLen] != ' ' && line[magicLen] != '\t')) {
        fclose(file);
        return SAVE_ERR_FORMAT;
    }

    version = (int)strtol(line + magicLen, NULL, 10);
    if (version != SAVE_VERSION) {
        fclose(file);
        return SAVE_ERR_VERSION;
    }

    if (!sg_readInts(file, header, 6)) {
        fclose(file);
        return SAVE_ERR_FORMAT;
    }

    memset(&temp, 0, sizeof(temp));
    temp.size          = header[0];
    temp.winLength     = header[1];
    temp.mode          = header[2];
    temp.botLevel      = header[3];
    temp.currentPlayer = header[4];
    temp.moveCount     = header[5];

    if (temp.size < 3 || temp.size > MAX_SIZE ||
        temp.moveCount < 0 || temp.moveCount > temp.size * temp.size) {
        fclose(file);
        return SAVE_ERR_DATA;
    }

    /* Ban co */
    for (i = 0; i < temp.size; ++i) {
        int row[MAX_SIZE];

        if (!sg_readInts(file, row, temp.size)) {
            fclose(file);
            return SAVE_ERR_FORMAT;
        }
        for (j = 0; j < temp.size; ++j) {
            temp.board[i][j] = row[j];
            if (row[j] != SAVE_CELL_EMPTY) stones++;
        }
    }

    for (i = 0; i < temp.moveCount; ++i) {
        int mv[3];

        if (!sg_readInts(file, mv, 3)) {
            fclose(file);
            return SAVE_ERR_FORMAT;
        }
        temp.history[i].row    = mv[0];
        temp.history[i].col    = mv[1];
        temp.history[i].player = mv[2];
    }

    fclose(file);

    if (!sg_validate(&temp) || stones != temp.moveCount) return SAVE_ERR_DATA;

    *game = temp;  
    return SAVE_OK;
}

SaveResult saveToSlot(const GameState *game, int slot)
{
    char path[128];

    if (slot < 0) return SAVE_ERR_DATA;

    (void)SG_MKDIR(SAVE_DIR);   
    sg_slotPath(path, sizeof(path), slot);

    return saveGame(game, path);
}

SaveResult loadFromSlot(GameState *game, int slot)
{
    char path[128];

    if (slot < 0) return SAVE_ERR_NOT_FOUND;

    sg_slotPath(path, sizeof(path), slot);
    return loadGame(game, path);
}

int slotExists(int slot)
{
    char path[128];
    FILE *file;

    if (slot < 0) return 0;

    sg_slotPath(path, sizeof(path), slot);
    file = sg_open(path, "r");
    if (file == NULL) return 0;

    fclose(file);
    return 1;
}
const char *saveResultMessage(SaveResult result)
{
    switch (result) {
    case SAVE_OK:            return "Thanh cong";
    case SAVE_ERR_OPEN:      return "Khong mo/ghi duoc file";
    case SAVE_ERR_FORMAT:    return "File sai dinh dang";
    case SAVE_ERR_VERSION:   return "Phien ban file save khong ho tro";
    case SAVE_ERR_DATA:      return "Du lieu trong file khong hop le";
    case SAVE_ERR_NOT_FOUND: return "Khong tim thay file save";
    default:                 return "Loi khong xac dinh";
    }
}