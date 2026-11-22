#ifndef PLAYER_H
#define PLAYER_H
#include "Board.h"
#define PLAYER_NAME_MAX 36
typedef struct { char name[PLAYER_NAME_MAX]; Cell symbol; } Player;
void Player_init(Player* player, const char* name, Cell symbol);
int Player_setName(Player* player, const char* name);
#endif
