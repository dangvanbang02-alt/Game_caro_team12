#ifndef MATCH_HISTORY_H
#define MATCH_HISTORY_H
#include "Config.h"
#include "Checkwin.h"
typedef struct { char playerX[PLAYER_NAME_MAX], playerO[PLAYER_NAME_MAX]; GameResult result; int moveCount; unsigned long durationMs; } MatchRecord;
typedef struct { MatchRecord records[MAX_MATCH_HISTORY]; int count; } MatchHistory;
void MatchHistory_init(MatchHistory* history);
int MatchHistory_add(MatchHistory* history,const MatchRecord* record);
int MatchHistory_save(const char* path,const MatchHistory* history);
int MatchHistory_load(const char* path,MatchHistory* history);
#endif
