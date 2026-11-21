#include "Checkwin.h"
static const int DIRS[4][2] = {{0,1},{1,0},{1,1},{1,-1}};
static int countDirection(const Board* b, int r, int c, int dr, int dc, Cell p) {
    int n = 0; r += dr; c += dc;
    while (Board_isInside(r,c) && b->cells[r][c] == p) { ++n; r += dr; c += dc; }
    return n;
}
int WinCheck_hasFiveFrom(const Board* b, int r, int c) {
    int d; Cell p;
    if (!b || !Board_isInside(r,c)) return 0;
    p = b->cells[r][c]; if (p == CELL_EMPTY) return 0;
    for (d=0; d<4; ++d) {
        int dr=DIRS[d][0], dc=DIRS[d][1];
        if (1 + countDirection(b,r,c,dr,dc,p) + countDirection(b,r,c,-dr,-dc,p) >= WIN_LENGTH) return 1;
    }
    return 0;
}
GameResult WinCheck_checkFrom(const Board* b, int r, int c) {
    if (!b || !Board_isInside(r,c)) return GAME_RESULT_ONGOING;
    if (b->cells[r][c] == CELL_X && WinCheck_hasFiveFrom(b,r,c)) return GAME_RESULT_X_WINS;
    if (b->cells[r][c] == CELL_O && WinCheck_hasFiveFrom(b,r,c)) return GAME_RESULT_O_WINS;
    return Board_isFull(b) ? GAME_RESULT_DRAW : GAME_RESULT_ONGOING;
}
GameResult WinCheck_check(const Board* b) {
    int r,c; if (!b) return GAME_RESULT_ONGOING;
    for (r=0;r<BOARD_SIZE;++r) for(c=0;c<BOARD_SIZE;++c)
        if (b->cells[r][c] != CELL_EMPTY && WinCheck_hasFiveFrom(b,r,c))
            return b->cells[r][c] == CELL_X ? GAME_RESULT_X_WINS : GAME_RESULT_O_WINS;
    return Board_isFull(b) ? GAME_RESULT_DRAW : GAME_RESULT_ONGOING;
}
