#include "Stats.h"

void Stats_reset(PlayerStats* stats) {
    if (stats == 0) return;
    stats->moves = 0;
    stats->wins = 0;
    stats->losses = 0;
    stats->draws = 0;
}

void Stats_recordMove(PlayerStats* stats) {
    if (stats != 0) ++stats->moves;
}

void Stats_recordWin(PlayerStats* winner, PlayerStats* loser) {
    if (winner != 0) ++winner->wins;
    if (loser != 0) ++loser->losses;
}

void Stats_recordDraw(PlayerStats* playerX, PlayerStats* playerO) {
    if (playerX != 0) ++playerX->draws;
    if (playerO != 0) ++playerO->draws;
}
