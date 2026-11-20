#ifndef STATS_H
#define STATS_H

#include "Board.h"

typedef struct {
    int moves;
    int wins;
    int losses;
    int draws;
} PlayerStats;

void Stats_reset(PlayerStats* stats);
void Stats_recordMove(PlayerStats* stats);
void Stats_recordWin(PlayerStats* winner, PlayerStats* loser);
void Stats_recordDraw(PlayerStats* playerX, PlayerStats* playerO);

#endif
