#include "Game.h"
#include <string.h>


static int playerIndex(Cell p){return p==CELL_X?0:1;}
static PlayerStats* statsFor(Game*g,Cell p){return p==CELL_X?&g->statsX:&g->statsO;}
static void recordResult(Game*g){MatchRecord rec;if(!g||g->result==GAME_RESULT_ONGOING||g->resultRecorded)return;
    if(g->result==GAME_RESULT_X_WINS)Stats_recordWin(&g->statsX,&g->statsO);
    else if(g->result==GAME_RESULT_O_WINS)Stats_recordWin(&g->statsO,&g->statsX);
    else Stats_recordDraw(&g->statsX,&g->statsO);
    memset(&rec,0,sizeof(rec));strncpy(rec.playerX,g->players[0].name,PLAYER_NAME_MAX-1);strncpy(rec.playerO,g->players[1].name,PLAYER_NAME_MAX-1);rec.result=g->result;rec.moveCount=Board_countMoves(&g->board);rec.durationMs=g->elapsedMs;MatchHistory_add(&g->matchHistory,&rec);g->resultRecorded=1;
}
void Game_init(Game*g,GameMode mode,AIDifficulty difficulty){
    if(!g)return;
    memset(g,0,sizeof(*g));
    Board_init(&g->board);
    Player_init(&g->players[0],"Player 1",CELL_X);
    Player_init(&g->players[1],mode==GAME_MODE_PVE?"Computer":"Player 2",CELL_O);
    g->currentPlayer=CELL_X;
    g->mode=mode;
    g->difficulty=difficulty;
    g->result=GAME_RESULT_ONGOING;
    Stats_reset(&g->statsX);
    Stats_reset(&g->statsO);
    GameConfig_default(&g->config);
    g->config.aiDepth=difficulty==AI_DIFFICULTY_HARD?4:2;
    Timer_init(&g->timer,0,0);
    MoveHistory_init(&g->history);
    MatchHistory_init(&g->matchHistory);
}
void Game_resetRound(Game*g){
    if(!g)return;
    Board_clear(&g->board);
    g->currentPlayer=CELL_X;
    g->result=GAME_RESULT_ONGOING;
    g->elapsedMs=0;
    g->paused=0;
    g->resultRecorded=0;
    MoveHistory_clear(&g->history);
    Timer_init(&g->timer,g->config.turnTimeLimitMs,g->config.matchTimeLimitMs);
}
int Game_playMove(Game*g,int r,int c){
    Cell p;
    GameResult result;
    if(!g||g->paused||g->result!=GAME_RESULT_ONGOING||Timer_isTurnExpired(&g->timer)||Timer_isMatchExpired(&g->timer))
        return 0;
    p=g->currentPlayer;
    if(!Board_makeMove(&g->board,r,c,p))
        return 0;
    if(!MoveHistory_push(&g->history,r,c,p)){
        g->board.cells[r][c]=CELL_EMPTY;
        return 0;
    }
    Stats_recordMove(statsFor(g,p));
    result=WinCheck_checkFrom(&g->board,r,c);
    if(result!=GAME_RESULT_ONGOING){
        g->result=result;
        recordResult(g);
        return 1;
    }
    g->currentPlayer=p==CELL_X?CELL_O:CELL_X;
    Timer_startTurn(&g->timer);
    return 1;
}
static void rebuild(Game*g){
    int i;
    Board_clear(&g->board);
    for(i=0;i<g->history.cursor;++i){
        Move*m=&g->history.moves[i];
        g->board.cells[m->row][m->col]=m->player;
    }
    g->currentPlayer=(g->history.cursor%2==0)?CELL_X:CELL_O;
    g->result=WinCheck_check(&g->board);
}
int Game_undo(Game*g){
    Move m;
	if(!g||g->paused||g->result!=GAME_RESULT_ONGOING||!MoveHistory_undo(&g->history,&m))
		return 0;
	if(statsFor(g,m.player)->moves>0)
		--statsFor(g,m.player)->moves;
	rebuild(g);
	Timer_startTurn(&g->timer);
	return 1;
}
int Game_redo(Game*g){
    Move m;
	if(!g||g->paused||g->result!=GAME_RESULT_ONGOING||!MoveHistory_redo(&g->history,&m))
		return 0;
	if(!Board_makeMove(&g->board,m.row,m.col,m.player))
		return 0;
	Stats_recordMove(statsFor(g,m.player));
	g->currentPlayer=m.player==CELL_X?CELL_O:CELL_X;
	g->result=WinCheck_checkFrom(&g->board,m.row,m.col);
	if(g->result!=GAME_RESULT_ONGOING)
		recordResult(g);
	else
		Timer_startTurn(&g->timer);
	return 1;
}
int Game_tick(Game* g, unsigned int ms)
{
    if (!g || g->paused ||
        g->result != GAME_RESULT_ONGOING)
    {
        return 0;
    }
    g->elapsedMs += ms;
    Timer_tick(&g->timer, ms);
    if (Timer_isMatchExpired(&g->timer))
    {
        g->result = GAME_RESULT_DRAW;
        recordResult(g);
        return 1;
    }
    if (Timer_isTurnExpired(&g->timer))
    {
        g->result = (g->currentPlayer == CELL_X)
            ? GAME_RESULT_O_WINS
            : GAME_RESULT_X_WINS;

        recordResult(g);
        return 1;
    }

    return 0;
}
void Game_pause(Game*g){
    if(g&&g->result==GAME_RESULT_ONGOING){
        g->paused=1;
    }
}
void Game_resume(Game*g){
    if(g)g->paused=0;
}
int Game_setPlayerName(Game*g,Cell s,const char*n){if(!g)return 0;return Player_setName(&g->players[playerIndex(s)],n);}
int Game_setConfig(Game*g,const GameConfig*c){
    if(!g||!GameConfig_isValid(c)||Board_countMoves(&g->board)>0)return 0;g->config=*c;Timer_init(&g->timer,c->turnTimeLimitMs,c->matchTimeLimitMs);return 1;}
int Game_playAIMove(Game*g){int r,c;if(!g||g->mode!=GAME_MODE_PVE||g->currentPlayer!=CELL_O||g->result!=GAME_RESULT_ONGOING)return 0;if(!AI_chooseMove(&g->board,CELL_O,g->config.aiDepth,&r,&c))return 0;return Game_playMove(g,r,c);}
int Game_isFinished(const Game*g){return g&&g->result!=GAME_RESULT_ONGOING;}
Cell Game_getCurrentPlayer(const Game*g){return g?g->currentPlayer:CELL_EMPTY;}
const PlayerStats* Game_getStatsX(const Game*g){return g?&g->statsX:0;}
const PlayerStats* Game_getStatsO(const Game*g){return g?&g->statsO:0;}
void Replay_init(GameReplay*r,const MoveHistory*h){if(!r)return;Board_init(&r->board);r->source=h;r->cursor=0;}
int Replay_step(GameReplay*r){const Move*m;if(!r||!r->source||r->cursor>=r->source->count)return 0;m=&r->source->moves[r->cursor++];return Board_makeMove(&r->board,m->row,m->col,m->player);}
int Replay_reset(GameReplay*r){if(!r||!r->source)return 0;Board_clear(&r->board);r->cursor=0;return 1;}
