#ifndef MOVE_HISTORY_H
#define MOVE_HISTORY_H
#include "Board.h"
typedef struct { int row, col; Cell player; } Move;
typedef struct { Move moves[MAX_REPLAY_MOVES]; int count; int cursor; } MoveHistory;
void MoveHistory_init(MoveHistory* h);
int MoveHistory_push(MoveHistory* h, int row, int col, Cell player);
int MoveHistory_undo(MoveHistory* h, Move* undone);
int MoveHistory_redo(MoveHistory* h, Move* redone);
void MoveHistory_clear(MoveHistory* h);
#endif
