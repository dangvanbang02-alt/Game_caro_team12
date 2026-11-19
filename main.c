/*
 * CARO - VALORANT THEME
 * Pure C + Win32 API (GDI). No C++ classes, no external graphics library.
 * Build with MSVC:
 *   cl /nologo /W4 /O2 /utf-8 Caro_Valorant.c user32.lib gdi32.lib comdlg32.lib winmm.lib
 * Keep menu_bg.bmp and select_bg.bmp in the same folder as the .exe.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mmsystem.h>

#define APP_W 1280
#define APP_H 720
#define N 12
#define CELL 42
#define BOARD_X 72
#define BOARD_Y 92
#define TURN_DEFAULT 15
#define SAVE_VERSION 2

#define C_BG RGB(5, 7, 16)
#define C_RED RGB(255, 70, 85)
#define C_RED_DARK RGB(170, 30, 48)
#define C_WHITE RGB(240, 244, 250)
#define C_MUTED RGB(145, 158, 178)
#define C_CYAN RGB(42, 220, 230)
#define C_PANEL RGB(13, 19, 33)

typedef struct { int moves, wins, losses, draws; } PLAYER_STATS;
typedef struct {
    int version;
    int board[N][N];
    int turn, turnSeconds, secondsLeft;
    int charX, charO;
    PLAYER_STATS sx, so;
} SAVE_DATA;

enum { SCREEN_MENU, SCREEN_SELECT, SCREEN_GAME, SCREEN_STATS, SCREEN_SETTINGS };

static HWND g_hwnd;
static HINSTANCE g_instance;
static HBITMAP g_menuBmp, g_selectBmp;
static int g_screen = SCREEN_MENU;
static int g_board[N][N];
static int g_turn = 1; /* 1 = X, -1 = O */
static int g_charX = -1, g_charO = -1, g_selectingPlayer = 1;
static int g_turnSeconds = TURN_DEFAULT, g_secondsLeft = TURN_DEFAULT;
static DWORD g_turnStart = 0;
static PLAYER_STATS g_statsX = {0,0,0,0}, g_statsO = {0,0,0,0};
static int g_gameOver = 0, g_winner = 0;
static HFONT g_fontTitle, g_fontBig, g_font, g_fontSmall;
static const char *g_agents[3] = { "PHOENIX", "JETT", "SAGE" };
static int g_hoverAgent = -1;
static int g_hoverMenu = -1;
static int g_soundEnabled = 1;

static RECT MakeRect(int left, int top, int right, int bottom) {
    RECT r; r.left=left; r.top=top; r.right=right; r.bottom=bottom; return r;
}

/* ===== AUDIO: place WAV files in the sounds folder beside the EXE ===== */
static void PlaySFX(const char *name) {
    char path[MAX_PATH];
    if (!g_soundEnabled) return;
    sprintf(path, "sounds\\%s.wav", name);
    PlaySoundA(path, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}
static void PlayBGM(const char *name) {
    char path[MAX_PATH];
    if (!g_soundEnabled) return;
    sprintf(path, "sounds\\%s.wav", name);
    PlaySoundA(path, NULL, SND_FILENAME | SND_ASYNC | SND_LOOP | SND_NODEFAULT);
}
static void StopAudio(void) {
    PlaySoundA(NULL, NULL, 0);
}
static RECT MenuButtonRect(int index) {
    static const int top[3] = {390, 492, 595};
    return MakeRect(850, top[index], 1165, top[index] + 75);
}
static RECT AgentCardRect(int index) {
    static const int left[3] = {78, 473, 869};
    static const int right[3] = {392, 787, 1182};
    return MakeRect(left[index], 143, right[index], 625);
}
static int PointInRect(int x, int y, RECT r) {
    return x >= r.left && x < r.right && y >= r.top && y < r.bottom;
}

static void FillRectColor(HDC dc, RECT r, COLORREF color) {
    HBRUSH b = CreateSolidBrush(color); FillRect(dc, &r, b); DeleteObject(b);
}
static void DrawTextAt(HDC dc, const char *s, int x, int y, int size, COLORREF color, int bold, int center) {
    HFONT f = bold ? g_fontBig : g_font;
    HFONT old = (HFONT)SelectObject(dc, f);
    SetBkMode(dc, TRANSPARENT); SetTextColor(dc, color);
    RECT r = {x, y, x + 800, y + size + 22};
    if (center) DrawTextA(dc, s, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    else TextOutA(dc, x, y, s, (int)strlen(s));
    SelectObject(dc, old);
}
static void DrawCentered(HDC dc, const char *s, RECT r, COLORREF color, int size, int weight) {
    HFONT f = weight ? g_fontBig : g_font;
    HFONT old = (HFONT)SelectObject(dc, f);
    SetBkMode(dc, TRANSPARENT); SetTextColor(dc, color);
    DrawTextA(dc, s, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, old); (void)size;
}
static void DrawButton(HDC dc, RECT r, const char *label, int hot) {
    HBRUSH b = CreateSolidBrush(hot ? C_WHITE : C_RED); HPEN p = CreatePen(PS_SOLID, 2, C_WHITE);
    HBRUSH oldB = (HBRUSH)SelectObject(dc, b); HPEN oldP = (HPEN)SelectObject(dc, p);
    Rectangle(dc, r.left, r.top, r.right, r.bottom);
    SetBkMode(dc, TRANSPARENT); SetTextColor(dc, hot ? C_RED : RGB(8,10,18));
    SelectObject(dc, g_fontBig);
    DrawTextA(dc, label, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, oldB); SelectObject(dc, oldP); DeleteObject(b); DeleteObject(p);
}
static void DrawBitmapCover(HDC dc, HBITMAP bmp) {
    if (!bmp) return;
    BITMAP bm; GetObject(bmp, sizeof(bm), &bm);
    HDC mem = CreateCompatibleDC(dc); HBITMAP old = (HBITMAP)SelectObject(mem, bmp);
    SetStretchBltMode(dc, HALFTONE);
    StretchBlt(dc, 0, 0, APP_W, APP_H, mem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    SelectObject(mem, old); DeleteDC(mem);
}
static void ResetGame(void) {
    memset(g_board, 0, sizeof(g_board));
    g_turn = 1; g_gameOver = 0; g_winner = 0;
    g_secondsLeft = g_turnSeconds; g_turnStart = GetTickCount();
    g_statsX.moves = g_statsO.moves = 0;
}
static void StartNewGame(void) {
    ResetGame(); g_screen = SCREEN_GAME; PlayBGM("game"); InvalidateRect(g_hwnd, NULL, FALSE);
    SetTimer(g_hwnd, 1, 250, NULL);
}
static int CheckWinner(void) {
    int r,c,d,k,dr[4]={0,1,1,1},dc[4]={1,0,1,-1};
    int empty=0;
    for(r=0;r<N;r++) for(c=0;c<N;c++) {
        int p=g_board[r][c];
        if(!p) { empty++; continue; }
        for(d=0;d<4;d++) {
            int count=1;
            for(k=1;k<5;k++) {
                int rr=r+dr[d]*k, cc=c+dc[d]*k;
                if(rr>=0 && rr<N && cc>=0 && cc<N && g_board[rr][cc]==p) count++;
                else break;
            }
            if(count>=5) return p;
        }
    }
    return empty ? 0 : 2;
}
static void FinishGame(int result) {
    g_gameOver=1; g_winner=result; KillTimer(g_hwnd,1);
    StopAudio();
    if (result == 1 || result == -1) PlaySFX(result == g_turn ? "win" : "lose");
    else PlaySFX("draw");
    if(result==1) { g_statsX.wins++; g_statsO.losses++; }
    else if(result==-1) { g_statsO.wins++; g_statsX.losses++; }
    else { g_statsX.draws++; g_statsO.draws++; }
    InvalidateRect(g_hwnd,NULL,FALSE);
}
static void NextTurn(void) {
    g_turn=-g_turn; g_secondsLeft=g_turnSeconds; g_turnStart=GetTickCount(); PlaySFX("turn");
}
static void SaveGame(void) {
    OPENFILENAMEA ofn; char path[MAX_PATH]="caro_valorant.sav"; FILE *fp;
    SAVE_DATA data; memset(&ofn,0,sizeof(ofn)); memset(&data,0,sizeof(data));
    ofn.lStructSize=sizeof(ofn); ofn.hwndOwner=g_hwnd; ofn.lpstrFilter="Caro save (*.sav)\0*.sav\0All files\0*.*\0";
    ofn.lpstrFile=path; ofn.nMaxFile=MAX_PATH; ofn.lpstrDefExt="sav"; ofn.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;
    if(!GetSaveFileNameA(&ofn)) return;
    data.version=SAVE_VERSION; memcpy(data.board,g_board,sizeof(g_board));
    data.turn=g_turn; data.turnSeconds=g_turnSeconds; data.secondsLeft=g_secondsLeft;
    data.charX=g_charX; data.charO=g_charO; data.sx=g_statsX; data.so=g_statsO;
    fp=fopen(path,"wb"); if(!fp) { MessageBoxA(g_hwnd,"Khong the tao file luu.","CARO",MB_ICONERROR); return; }
    fwrite(&data,sizeof(data),1,fp); fclose(fp);
    MessageBoxA(g_hwnd,"Da luu van dau!","CARO",MB_OK|MB_ICONINFORMATION);
}
static void LoadGame(void) {
    OPENFILENAMEA ofn; char path[MAX_PATH]=""; FILE *fp; SAVE_DATA data;
    memset(&ofn,0,sizeof(ofn)); ofn.lStructSize=sizeof(ofn); ofn.hwndOwner=g_hwnd;
    ofn.lpstrFilter="Caro save (*.sav)\0*.sav\0All files\0*.*\0"; ofn.lpstrFile=path; ofn.nMaxFile=MAX_PATH;
    ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
    if(!GetOpenFileNameA(&ofn)) return;
    fp=fopen(path,"rb"); if(!fp) return;
    if(fread(&data,sizeof(data),1,fp)!=1 || data.version!=SAVE_VERSION || (data.turn!=1 && data.turn!=-1)) {
        fclose(fp); MessageBoxA(g_hwnd,"File luu khong hop le.","CARO",MB_ICONERROR); return;
    }
    fclose(fp); memcpy(g_board,data.board,sizeof(g_board)); g_turn=data.turn;
    g_turnSeconds=(data.turnSeconds<1||data.turnSeconds>3600)?TURN_DEFAULT:data.turnSeconds;
    g_secondsLeft=(data.secondsLeft<0||data.secondsLeft>g_turnSeconds)?g_turnSeconds:data.secondsLeft;
    g_charX=data.charX; g_charO=data.charO; g_statsX=data.sx; g_statsO=data.so;
    g_gameOver=0; g_turnStart=GetTickCount()-(g_turnSeconds-g_secondsLeft)*1000;
    g_screen=SCREEN_GAME; SetTimer(g_hwnd,1,250,NULL); InvalidateRect(g_hwnd,NULL,FALSE);
}
static void DrawMenu(HDC dc) {
    int i;
    DrawBitmapCover(dc, g_menuBmp);
    /* Align controls exactly over the three red panels in the supplied menu bitmap. */
    for (i = 0; i < 3; ++i) {
        RECT r = MenuButtonRect(i);
        const char *labels[3] = {"PLAY", "LOAD GAME", "EXIT"};
        DrawButton(dc, r, labels[i], g_hoverMenu == i);
    }
    DrawTextAt(dc, "CARO // PROTOCOL", 48, 32, 26, C_WHITE, 1, 0);
}
static void DrawSelect(HDC dc) {
    int i;
    DrawBitmapCover(dc, g_selectBmp);
    /* Match the actual card frames embedded in select_bg.bmp. */
    for (i = 0; i < 3; ++i) {
        RECT r = AgentCardRect(i);
        int selected = (g_selectingPlayer == 2 && g_charX == i);
        int hot = (g_hoverAgent == i);
        COLORREF edge = selected ? C_RED : (hot ? C_CYAN : RGB(170, 180, 195));
        HPEN pen = CreatePen(PS_SOLID, (selected || hot) ? 4 : 1, edge);
        HBRUSH oldBrush = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
        HPEN oldPen = (HPEN)SelectObject(dc, pen);
        Rectangle(dc, r.left, r.top, r.right, r.bottom);
        SelectObject(dc, oldPen);
        SelectObject(dc, oldBrush);
        DeleteObject(pen);

        {
            RECT label = {r.left, 548, r.right, 580};
            DrawCentered(dc, g_agents[i], label, C_WHITE, 20, 1);
        }
    }
    {
        RECT prompt = MakeRect(300, 632, 980, 674);
        DrawCentered(dc,
            g_selectingPlayer == 1 ? "CHON NHAN VAT CHO PLAYER X" : "CHON NHAN VAT CHO PLAYER O",
            prompt, g_selectingPlayer == 1 ? C_RED : C_CYAN, 22, 1);
    }
    DrawTextAt(dc, "Nhan ESC de quay lai menu", 48, 670, 16, C_WHITE, 0, 0);
}
static void DrawGame(HDC dc) {
    int r,c,x,y; RECT panel={650,75,1225,650};
    FillRectColor(dc,MakeRect(0,0,APP_W,APP_H),C_BG);
    /* angular accent strips */
    FillRectColor(dc,MakeRect(0,0,APP_W,7),C_RED);
    DrawTextAt(dc,"CARO // TACTICAL ARENA",55,25,24,C_WHITE,1,0);
    FillRectColor(dc,panel,C_PANEL);
    { HPEN p=CreatePen(PS_SOLID,2,C_RED); HPEN old=(HPEN)SelectObject(dc,p); HBRUSH ob=(HBRUSH)SelectObject(dc,GetStockObject(NULL_BRUSH)); Rectangle(dc,650,75,1225,650); SelectObject(dc,ob); SelectObject(dc,old); DeleteObject(p); }
    /* board background and grid */
    FillRectColor(dc,MakeRect(BOARD_X-18,BOARD_Y-18,BOARD_X+N*CELL+18,BOARD_Y+N*CELL+18),RGB(15,22,36));
    for(r=0;r<N;r++) for(c=0;c<N;c++) {
        x=BOARD_X+c*CELL; y=BOARD_Y+r*CELL;
        if((r+c)%2==0) FillRectColor(dc,MakeRect(x,y,x+CELL,y+CELL),RGB(20,30,47));
    }
    { HPEN p=CreatePen(PS_SOLID,1,RGB(72,91,115)); HPEN old=(HPEN)SelectObject(dc,p);
      for(r=0;r<=N;r++){ MoveToEx(dc,BOARD_X,BOARD_Y+r*CELL,NULL); LineTo(dc,BOARD_X+N*CELL,BOARD_Y+r*CELL); }
      for(c=0;c<=N;c++){ MoveToEx(dc,BOARD_X+c*CELL,BOARD_Y,NULL); LineTo(dc,BOARD_X+c*CELL,BOARD_Y+N*CELL); }
      SelectObject(dc,old); DeleteObject(p); }
    for(r=0;r<N;r++) for(c=0;c<N;c++) if(g_board[r][c]) {
        x=BOARD_X+c*CELL; y=BOARD_Y+r*CELL;
        if(g_board[r][c]==1) {
            HPEN p=CreatePen(PS_SOLID,4,C_RED), old=(HPEN)SelectObject(dc,p);
            MoveToEx(dc,x+10,y+10,NULL); LineTo(dc,x+CELL-10,y+CELL-10);
            MoveToEx(dc,x+CELL-10,y+10,NULL); LineTo(dc,x+10,y+CELL-10);
            SelectObject(dc,old); DeleteObject(p);
        } else {
            HPEN p=CreatePen(PS_SOLID,4,C_CYAN), old=(HPEN)SelectObject(dc,p); HBRUSH ob=(HBRUSH)SelectObject(dc,GetStockObject(NULL_BRUSH));
            Ellipse(dc,x+9,y+9,x+CELL-9,y+CELL-9); SelectObject(dc,ob); SelectObject(dc,old); DeleteObject(p);
        }
    }
    DrawTextAt(dc,"MATCH STATUS",685,100,20,C_RED,1,0);
    { char s[160]; sprintf(s,"PLAYER X  |  %s",g_charX>=0?g_agents[g_charX]:"PHOENIX"); DrawTextAt(dc,s,690,155,18,C_WHITE,0,0);
      sprintf(s,"PLAYER O  |  %s",g_charO>=0?g_agents[g_charO]:"JETT"); DrawTextAt(dc,s,690,195,18,C_WHITE,0,0);
      sprintf(s,"LUOT HIEN TAI: %c",g_turn==1?'X':'O'); DrawTextAt(dc,s,690,260,20,g_turn==1?C_RED:C_CYAN,1,0);
      sprintf(s,"THOI GIAN: %02d s",g_secondsLeft); DrawTextAt(dc,s,690,305,20,C_WHITE,1,0);
      sprintf(s,"X  %d nuoc  |  %d thang",g_statsX.moves,g_statsX.wins); DrawTextAt(dc,s,690,375,16,C_MUTED,0,0);
      sprintf(s,"O  %d nuoc  |  %d thang",g_statsO.moves,g_statsO.wins); DrawTextAt(dc,s,690,410,16,C_MUTED,0,0); }
    DrawTextAt(dc,"CLICK: DANH CO",690,485,16,C_WHITE,0,0);
    DrawTextAt(dc,"F2: VAN MOI     F5: LUU",690,520,16,C_WHITE,0,0);
    DrawTextAt(dc,"F9: TAI          ESC: MENU",690,550,16,C_WHITE,0,0);
    if(g_gameOver) {
        RECT veil={BOARD_X-15,BOARD_Y+190,BOARD_X+N*CELL+15,BOARD_Y+310};
        FillRectColor(dc,veil,RGB(7,10,20));
        { char msg[100]; if(g_winner==1) strcpy(msg,"PLAYER X WINS!"); else if(g_winner==-1) strcpy(msg,"PLAYER O WINS!"); else strcpy(msg,"DRAW MATCH!");
          DrawCentered(dc,msg,veil,C_RED,30,1); }
        DrawTextAt(dc,"Nhan F2 de choi lai hoac ESC de ve menu",BOARD_X+20,veil.bottom+12,15,C_WHITE,0,0);
    }
}
static void DrawStats(HDC dc) {
    char s[200]; FillRectColor(dc,MakeRect(0,0,APP_W,APP_H),C_BG);
    DrawTextAt(dc,"MATCH STATISTICS",80,60,30,C_RED,1,0);
    sprintf(s,"PLAYER X: %d moves | %d wins | %d losses | %d draws",g_statsX.moves,g_statsX.wins,g_statsX.losses,g_statsX.draws); DrawTextAt(dc,s,100,190,20,C_WHITE,0,0);
    sprintf(s,"PLAYER O: %d moves | %d wins | %d losses | %d draws",g_statsO.moves,g_statsO.wins,g_statsO.losses,g_statsO.draws); DrawTextAt(dc,s,100,250,20,C_CYAN,0,0);
    DrawTextAt(dc,"ESC: BACK TO MENU",100,620,18,C_MUTED,0,0);
}
static void DrawSettings(HDC dc) {
    char s[100]; FillRectColor(dc,MakeRect(0,0,APP_W,APP_H),C_BG);
    DrawTextAt(dc,"SETTINGS",80,60,32,C_RED,1,0);
    sprintf(s,"THOI GIAN MOI LUOT: %d GIAY",g_turnSeconds); DrawTextAt(dc,s,100,210,24,C_WHITE,1,0);
    DrawTextAt(dc,"[-] giam 5 giay       [+] tang 5 giay",100,280,20,C_CYAN,0,0);
    DrawTextAt(dc,"ESC: BACK TO MENU",100,620,18,C_MUTED,0,0);
}
static void PaintScene(HWND hwnd) {
    PAINTSTRUCT ps; HDC dc=BeginPaint(hwnd,&ps);
    switch(g_screen) {
        case SCREEN_MENU: DrawMenu(dc); break;
        case SCREEN_SELECT: DrawSelect(dc); break;
        case SCREEN_GAME: DrawGame(dc); break;
        case SCREEN_STATS: DrawStats(dc); break;
        case SCREEN_SETTINGS: DrawSettings(dc); break;
    }
    EndPaint(hwnd,&ps);
}
static void HandleClick(int x,int y) {
    if(g_screen==SCREEN_MENU) {
        int i;
        for (i = 0; i < 3; ++i) {
            if (PointInRect(x, y, MenuButtonRect(i))) {
                PlaySFX("click");
                if (i == 0) {
                    g_selectingPlayer=1; g_charX=g_charO=-1;
                    g_hoverAgent=-1; g_screen=SCREEN_SELECT;
                    PlayBGM("select");
                } else if (i == 1) {
                    LoadGame();
                    if (g_screen == SCREEN_GAME) PlayBGM("game");
                } else {
                    PostMessage(g_hwnd,WM_CLOSE,0,0);
                }
                break;
            }
        }
    } else if(g_screen==SCREEN_SELECT) {
        int idx=-1, i;
        for (i = 0; i < 3; ++i) {
            if (PointInRect(x, y, AgentCardRect(i))) { idx = i; break; }
        }
        if(idx>=0) {
            PlaySFX("click");
            if(g_selectingPlayer==1) {
                g_charX=idx; g_selectingPlayer=2;
                PlayBGM("select");
            } else {
                g_charO=idx; StartNewGame();
            }
        }
    } else if(g_screen==SCREEN_GAME && !g_gameOver) {
        int c=(x-BOARD_X)/CELL, r=(y-BOARD_Y)/CELL;
        if(x>=BOARD_X&&x<BOARD_X+N*CELL&&y>=BOARD_Y&&y<BOARD_Y+N*CELL&&r>=0&&r<N&&c>=0&&c<N&&g_board[r][c]==0) {
            g_board[r][c]=g_turn; PlaySFX("place"); if(g_turn==1) g_statsX.moves++; else g_statsO.moves++;
            { int result=CheckWinner(); if(result==1||result==-1||result==2) FinishGame(result); else NextTurn(); }
        }
    } else if(g_screen==SCREEN_SETTINGS) {
        if(x>=80&&x<=600&&y>=250&&y<=330) { if(x<330) g_turnSeconds-=5; else g_turnSeconds+=5; if(g_turnSeconds<5)g_turnSeconds=5; if(g_turnSeconds>300)g_turnSeconds=300; }
    }
    InvalidateRect(g_hwnd,NULL,FALSE);
}
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg) {
        case WM_PAINT: PaintScene(hwnd); return 0;
        case WM_MOUSEMOVE: {
            int x = (int)(short)LOWORD(lp), y = (int)(short)HIWORD(lp);
            TRACKMOUSEEVENT tme;
            int oldMenu = g_hoverMenu, oldAgent = g_hoverAgent;
            g_hoverMenu = -1; g_hoverAgent = -1;
            if (g_screen == SCREEN_MENU) {
                int i;
                for (i = 0; i < 3; ++i) if (PointInRect(x, y, MenuButtonRect(i))) { g_hoverMenu = i; break; }
            } else if (g_screen == SCREEN_SELECT) {
                int i;
                for (i = 0; i < 3; ++i) if (PointInRect(x, y, AgentCardRect(i))) { g_hoverAgent = i; break; }
            }
            if (oldMenu != g_hoverMenu || oldAgent != g_hoverAgent) InvalidateRect(hwnd, NULL, FALSE);
            ZeroMemory(&tme, sizeof(tme));
            tme.cbSize = sizeof(tme); tme.dwFlags = TME_LEAVE; tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            return 0;
        }
        case WM_MOUSELEAVE:
            g_hoverMenu = -1; g_hoverAgent = -1;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        case WM_LBUTTONDOWN: HandleClick(LOWORD(lp),HIWORD(lp)); return 0;
        case WM_KEYDOWN:
            if(wp==VK_ESCAPE) {
                if(g_screen==SCREEN_GAME) { KillTimer(hwnd,1); g_screen=SCREEN_MENU; PlayBGM("menu"); }
                else if(g_screen==SCREEN_SELECT) { g_screen=SCREEN_MENU; PlayBGM("menu"); }
                else if(g_screen==SCREEN_STATS||g_screen==SCREEN_SETTINGS) { g_screen=SCREEN_MENU; PlayBGM("menu"); }
                InvalidateRect(hwnd,NULL,FALSE); return 0;
            }
            if(g_screen==SCREEN_MENU) {
                if(wp==VK_F1) { g_screen=SCREEN_STATS; InvalidateRect(hwnd,NULL,FALSE); }
                if(wp==VK_F2) { g_screen=SCREEN_SETTINGS; InvalidateRect(hwnd,NULL,FALSE); }
            } else if(g_screen==SCREEN_GAME) {
                if(wp==VK_F2) StartNewGame();
                else if(wp==VK_F5) SaveGame();
                else if(wp==VK_F9) LoadGame();
            }
            return 0;
        case WM_TIMER:
            if(g_screen==SCREEN_GAME&&!g_gameOver) {
                DWORD elapsed=GetTickCount()-g_turnStart;
                int left=g_turnSeconds-(int)(elapsed/1000);
                if(left!=g_secondsLeft) { g_secondsLeft=left; if(g_secondsLeft<=0) { g_secondsLeft=0; NextTurn(); } InvalidateRect(hwnd,NULL,FALSE); }
            }
            return 0;
        case WM_DESTROY: StopAudio(); PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd,msg,wp,lp);
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE prev,LPSTR cmd,int show) {
    WNDCLASSA wc; MSG msg; (void)prev; (void)cmd;
    g_instance=instance;
    g_menuBmp=(HBITMAP)LoadImageA(NULL,"menu_bg.bmp",IMAGE_BITMAP,0,0,LR_LOADFROMFILE|LR_CREATEDIBSECTION);
    g_selectBmp=(HBITMAP)LoadImageA(NULL,"select_bg.bmp",IMAGE_BITMAP,0,0,LR_LOADFROMFILE|LR_CREATEDIBSECTION);
    g_fontTitle=CreateFontA(48,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,"Arial");
    g_fontBig=CreateFontA(25,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,"Arial");
    g_font=CreateFontA(19,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,"Arial");
    g_fontSmall=CreateFontA(15,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,"Arial");
    memset(&wc,0,sizeof(wc)); wc.lpfnWndProc=WndProc; wc.hInstance=instance; wc.lpszClassName="CaroValorantWindow";
    wc.hCursor=LoadCursor(NULL,IDC_ARROW); wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
    if(!RegisterClassA(&wc)) return 1;
    g_hwnd=CreateWindowA("CaroValorantWindow","CARO // VALORANT THEME",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
        CW_USEDEFAULT,CW_USEDEFAULT,APP_W,APP_H,NULL,NULL,instance,NULL);
    if(!g_hwnd) return 1;
    ShowWindow(g_hwnd,show); UpdateWindow(g_hwnd); PlayBGM("menu");
    if(!g_menuBmp||!g_selectBmp) MessageBoxA(g_hwnd,"Khong tim thay menu_bg.bmp hoac select_bg.bmp. Hay de 2 file BMP cung thu muc voi file EXE.","Thieu hinh nen",MB_ICONWARNING);
    while(GetMessage(&msg,NULL,0,0)>0) { TranslateMessage(&msg); DispatchMessage(&msg); }
    
if (g_menuBmp) {
    DeleteObject(g_menuBmp);
}

if (g_selectBmp) {
    DeleteObject(g_selectBmp);
}

if (g_fontTitle) {
    DeleteObject(g_fontTitle);
}

if (g_fontBig) {
    DeleteObject(g_fontBig);
}

if (g_font) {
    DeleteObject(g_font);
}

if (g_fontSmall) {
    DeleteObject(g_fontSmall);
}

    return (int)msg.wParam;
}
