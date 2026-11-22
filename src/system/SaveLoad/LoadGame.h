#ifndef LOAD_GAME_H
#define LOAD_GAME_H


#include "Game.h"
#include "SaveGame.h"

SaveResult LoadGame_fromFile(Game *game, const char *filename);

SaveResult LoadGame_fromSlot(Game *game, int slot);

SaveResult LoadGame_fromState(Game *game, const GameState *state);

#endif
