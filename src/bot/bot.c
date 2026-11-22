#include <stdlib.h>
#include "bot.h"

#define HARD_DEPTH   4        
#define HARD_BRANCH  8       
#define CAND_RADIUS  2       
#define WIN_SCORE    1000000

typedef struct {
    int row;
    int col;
    int score;
} Cand;

static const int DR[4] = { 0, 1, 1,  1 };
static const int DC[4] = { 1, 0, 1, -1 };

static int in_bounds(int r, int c)
{
    return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE;
}

static int opponent_of(int player)
{
    return (player == CELL_X) ? CELL_O : CELL_X;
}

static int count_stones(int board[BOARD_SIZE][BOARD_SIZE])
{
    int r, c, n = 0;
    for (r = 0; r < BOARD_SIZE; r++)
        for (c = 0; c < BOARD_SIZE; c++)
            if (board[r][c] != CELL_EMPTY)
                n++;
    return n;
}

int bot_board_full(int board[BOARD_SIZE][BOARD_SIZE])
{
    return count_stones(board) == BOARD_SIZE * BOARD_SIZE;
}

static int has_neighbor(int board[BOARD_SIZE][BOARD_SIZE], int r, int c, int radius)
{
    int i, j;
    for (i = r - radius; i <= r + radius; i++)
        for (j = c - radius; j <= c + radius; j++)
            if (in_bounds(i, j) && board[i][j] != CELL_EMPTY)
                return 1;
    return 0;
}

static int check_win_at(int board[BOARD_SIZE][BOARD_SIZE], int r, int c, int player)
{
    int d, k, cnt, nr, nc;
    for (d = 0; d < 4; d++) {
        cnt = 1;
        for (k = 1; k < WIN_LEN; k++) {
            nr = r + DR[d] * k;
            nc = c + DC[d] * k;
            if (!in_bounds(nr, nc) || board[nr][nc] != player) break;
            cnt++;
        }
        for (k = 1; k < WIN_LEN; k++) {
            nr = r - DR[d] * k;
            nc = c - DC[d] * k;
            if (!in_bounds(nr, nc) || board[nr][nc] != player) break;
            cnt++;
        }
        if (cnt >= WIN_LEN) return 1;
    }
    return 0;
}

int bot_check_win(int board[BOARD_SIZE][BOARD_SIZE], int player)
{
    int r, c;
    for (r = 0; r < BOARD_SIZE; r++)
        for (c = 0; c < BOARD_SIZE; c++)
            if (board[r][c] == player && check_win_at(board, r, c, player))
                return 1;
    return 0;
}

static int line_score(int count, int openEnds)
{
    if (count >= WIN_LEN) return 100000;
    if (openEnds == 0)    return 0;          
    switch (count) {
    case 4:  return (openEnds == 2) ? 10000 : 1000;
    case 3:  return (openEnds == 2) ? 1000  : 100;
    case 2:  return (openEnds == 2) ? 100   : 10;
    default: return (openEnds == 2) ? 10    : 1;
    }
}

static int cell_score(int board[BOARD_SIZE][BOARD_SIZE], int r, int c, int player)
{
    int d, k, nr, nc, cnt, open, total = 0;
    for (d = 0; d < 4; d++) {
        cnt = 1;
        open = 0;
        for (k = 1; ; k++) {
            nr = r + DR[d] * k;
            nc = c + DC[d] * k;
            if (!in_bounds(nr, nc)) break;
            if (board[nr][nc] == player) { cnt++; continue; }
            if (board[nr][nc] == CELL_EMPTY) open++;
            break;
        }
        for (k = 1; ; k++) {
            nr = r - DR[d] * k;
            nc = c - DC[d] * k;
            if (!in_bounds(nr, nc)) break;
            if (board[nr][nc] == player) { cnt++; continue; }
            if (board[nr][nc] == CELL_EMPTY) open++;
            break;
        }
        total += line_score(cnt, open);
    }
    return total;
}

static int window_score(int cnt)
{
    int s = 1, i;
    if (cnt >= WIN_LEN) return WIN_SCORE;
    for (i = 1; i < cnt; i++) s *= 10;       
    return s;
}

static int evaluate(int board[BOARD_SIZE][BOARD_SIZE], int me)
{
    int opp = opponent_of(me);
    int r, c, d, k, nr, nc, mine, theirs, score = 0;

    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            for (d = 0; d < 4; d++) {
                nr = r + DR[d] * (WIN_LEN - 1);
                nc = c + DC[d] * (WIN_LEN - 1);
                if (!in_bounds(nr, nc)) continue;

                mine = 0;
                theirs = 0;
                for (k = 0; k < WIN_LEN; k++) {
                    int v = board[r + DR[d] * k][c + DC[d] * k];
                    if (v == me)       mine++;
                    else if (v == opp) theirs++;
                }
                if (mine > 0 && theirs == 0)      score += window_score(mine);
                else if (theirs > 0 && mine == 0) score -= window_score(theirs);
            }
        }
    }
    return score;
}

static int gather_candidates(int board[BOARD_SIZE][BOARD_SIZE], Cand *out)
{
    int r, c, n = 0;

    if (count_stones(board) == 0) {
        out[0].row = BOARD_SIZE / 2;
        out[0].col = BOARD_SIZE / 2;
        out[0].score = 0;
        return 1;
    }

    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            if (board[r][c] == CELL_EMPTY && has_neighbor(board, r, c, CAND_RADIUS)) {
                out[n].row = r;
                out[n].col = c;
                out[n].score = 0;
                n++;
            }
        }
    }
    return n;
}

static int cmp_cand_desc(const void *a, const void *b)
{
    const Cand *x = (const Cand *)a;
    const Cand *y = (const Cand *)b;
    return y->score - x->score;
}

static void score_and_sort(int board[BOARD_SIZE][BOARD_SIZE], Cand *cands, int n, int player)
{
    int i, opp = opponent_of(player);
    for (i = 0; i < n; i++) {
        int atk = cell_score(board, cands[i].row, cands[i].col, player);
        int def = cell_score(board, cands[i].row, cands[i].col, opp);
        cands[i].score = atk * 10 + def * 9;
    }
    qsort(cands, (size_t)n, sizeof(Cand), cmp_cand_desc);
}

static BotMove find_winning_move(int board[BOARD_SIZE][BOARD_SIZE], int player)
{
    BotMove m = { -1, -1 };
    int r, c;
    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            if (board[r][c] != CELL_EMPTY) continue;
            board[r][c] = player;
            if (check_win_at(board, r, c, player)) {
                board[r][c] = CELL_EMPTY;
                m.row = r;
                m.col = c;
                return m;
            }
            board[r][c] = CELL_EMPTY;
        }
    }
    return m;
}

static BotMove move_easy(int board[BOARD_SIZE][BOARD_SIZE], int bot)
{
    Cand cands[BOARD_SIZE * BOARD_SIZE];
    BotMove m;
    int n;

    m = find_winning_move(board, bot);
    if (m.row >= 0) return m;

    n = gather_candidates(board, cands);
    if (n == 0) {
        m.row = -1;
        m.col = -1;
        return m;
    }
    n = rand() % n;
    m.row = cands[n].row;
    m.col = cands[n].col;
    return m;
}

static BotMove move_medium(int board[BOARD_SIZE][BOARD_SIZE], int bot)
{
    Cand cands[BOARD_SIZE * BOARD_SIZE];
    BotMove m;
    int opp = opponent_of(bot);
    int n, i, best, bestCount, pick;

    m = find_winning_move(board, bot);
    if (m.row >= 0) return m;

    m = find_winning_move(board, opp);
    if (m.row >= 0) return m;

    n = gather_candidates(board, cands);
    if (n == 0) {
        m.row = -1;
        m.col = -1;
        return m;
    }
    score_and_sort(board, cands, n, bot);

    best = cands[0].score;
    bestCount = 0;
    for (i = 0; i < n && cands[i].score == best; i++)
        bestCount++;
    pick = rand() % bestCount;

    m.row = cands[pick].row;
    m.col = cands[pick].col;
    return m;
}

static int minimax(int board[BOARD_SIZE][BOARD_SIZE], int depth, int alpha, int beta,
                   int maximizing, int bot, int lastR, int lastC, int lastPlayer)
{
    Cand cands[BOARD_SIZE * BOARD_SIZE];
    int opp = opponent_of(bot);
    int current = maximizing ? bot : opp;
    int n, i, limit, val, best;

    if (lastR >= 0 && check_win_at(board, lastR, lastC, lastPlayer))
        return (lastPlayer == bot) ? (WIN_SCORE + depth) : -(WIN_SCORE + depth);

    if (depth == 0)
        return evaluate(board, bot);

    n = gather_candidates(board, cands);
    if (n == 0)
        return 0;                      
    score_and_sort(board, cands, n, current);
    limit = (n < HARD_BRANCH) ? n : HARD_BRANCH;

    if (maximizing) {
        best = -2 * WIN_SCORE;
        for (i = 0; i < limit; i++) {
            board[cands[i].row][cands[i].col] = current;
            val = minimax(board, depth - 1, alpha, beta, 0, bot,
                          cands[i].row, cands[i].col, current);
            board[cands[i].row][cands[i].col] = CELL_EMPTY;

            if (val > best) best = val;
            if (best > alpha) alpha = best;
            if (alpha >= beta) break;
        }
    } else {
        best = 2 * WIN_SCORE;
        for (i = 0; i < limit; i++) {
            board[cands[i].row][cands[i].col] = current;
            val = minimax(board, depth - 1, alpha, beta, 1, bot,
                          cands[i].row, cands[i].col, current);
            board[cands[i].row][cands[i].col] = CELL_EMPTY;

            if (val < best) best = val;
            if (best < beta) beta = best;
            if (alpha >= beta) break;
        }
    }
    return best;
}

static BotMove move_hard(int board[BOARD_SIZE][BOARD_SIZE], int bot)
{
    Cand cands[BOARD_SIZE * BOARD_SIZE];
    BotMove m;
    int opp = opponent_of(bot);
    int n, i, limit, val, best, alpha;

    m = find_winning_move(board, bot);
    if (m.row >= 0) return m;
    m = find_winning_move(board, opp);
    if (m.row >= 0) return m;

    n = gather_candidates(board, cands);
    if (n == 0) {
        m.row = -1;
        m.col = -1;
        return m;
    }

    score_and_sort(board, cands, n, bot);
    limit = (n < HARD_BRANCH) ? n : HARD_BRANCH;

    m.row = cands[0].row;
    m.col = cands[0].col;
    best = -2 * WIN_SCORE;
    alpha = -2 * WIN_SCORE;

    for (i = 0; i < limit; i++) {
        board[cands[i].row][cands[i].col] = bot;
        val = minimax(board, HARD_DEPTH - 1, alpha, 2 * WIN_SCORE, 0, bot,
                      cands[i].row, cands[i].col, bot);
        board[cands[i].row][cands[i].col] = CELL_EMPTY;

        if (val > best) {
            best = val;
            m.row = cands[i].row;
            m.col = cands[i].col;
        }
        if (best > alpha) alpha = best;
    }
    return m;
}

BotMove bot_get_move(int board[BOARD_SIZE][BOARD_SIZE], int botPlayer, BotLevel level)
{
    BotMove none = { -1, -1 };

    if (bot_board_full(board)) return none;

    switch (level) {
    case BOT_EASY:   return move_easy(board, botPlayer);
    case BOT_MEDIUM: return move_medium(board, botPlayer);
    case BOT_HARD:   return move_hard(board, botPlayer);
    default:         return move_medium(board, botPlayer);
    }
}
