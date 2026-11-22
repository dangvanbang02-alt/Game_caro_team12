#include "MoveHistory.h"
void MoveHistory_init(MoveHistory* h) { if(h){h->count=0;h->cursor=0;} }
void MoveHistory_clear(MoveHistory* h) { MoveHistory_init(h); }
int MoveHistory_push(MoveHistory* h,int r,int c,Cell p) {
    if(!h||h->cursor>=MAX_REPLAY_MOVES||r<0||r>=BOARD_SIZE||c<0||c>=BOARD_SIZE||(p!=CELL_X&&p!=CELL_O))return 0;
    h->count=h->cursor; h->moves[h->cursor].row=r;h->moves[h->cursor].col=c;h->moves[h->cursor].player=p;
    ++h->cursor;h->count=h->cursor;return 1;
}
int MoveHistory_undo(MoveHistory* h,Move* m) { if(!h||h->cursor<=0)return 0; --h->cursor;if(m)*m=h->moves[h->cursor];return 1; }
int MoveHistory_redo(MoveHistory* h,Move* m) { if(!h||h->cursor>=h->count)return 0;if(m)*m=h->moves[h->cursor];++h->cursor;return 1; }
