#ifndef STATS_H
#define STATS_H
typedef struct { int moves, wins, losses, draws; } PlayerStats;
void Stats_reset(PlayerStats* stats);
void Stats_recordMove(PlayerStats* stats);
void Stats_recordWin(PlayerStats* winner, PlayerStats* loser);
void Stats_recordDraw(PlayerStats* playerX, PlayerStats* playerO);
int Stats_save(const char* path, const PlayerStats* x, const PlayerStats* o);
int Stats_load(const char* path, PlayerStats* x, PlayerStats* o);
#endif
