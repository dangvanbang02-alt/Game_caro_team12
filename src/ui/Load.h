#ifndef LOAD_H
#define LOAD_H
#include "raylib.h"
#include "Menu.h"
#include "HUD.h"
#include <stdbool.h>

#define LOAD_SAVE_DIR             "saves"
#define LOAD_VISIBLE_SLOTS        2
#define LOAD_BOARD_MAX_DIM        30
// Save
#define LOAD_IMG_BG               "assets/Load/12.png"
#define LOAD_IMG_FULL             "assets/Load/74.png"
#define LOAD_IMG_CLOSE_NORMAL     "assets/Load/75.png"
#define LOAD_IMG_CLOSE_HOVER      "assets/Load/76.png"
#define LOAD_IMG_DELETE_NORMAL    "assets/Load/77.png"
#define LOAD_IMG_DELETE_HOVER     "assets/Load/78.png"
#define LOAD_IMG_LOAD_NORMAL      "assets/Load/load1.png"
#define LOAD_IMG_LOAD_HOVER       "assets/Load/load2.png"

#define LOAD_ACCENT_MINT          (Color){ 44, 229, 167, 255 }
#define LOAD_ACCENT_RED           (Color){ 255, 70, 85, 255 }

typedef enum {
    LOAD_ACTION_NONE = 0,
    LOAD_ACTION_BACK,
    LOAD_ACTION_LOADED
} LoadAction;

typedef struct {
    char saveName[HUD_SAVE_NAME_MAX_LEN];
    char p1Name[HUD_NAME_MAX_LEN];
    char p2Name[HUD_NAME_MAX_LEN];
    int round;
    int currentTurnPiece;
    int p1Wins;
    int p2Wins;
    HudAgentId alphaAgent;
    HudAgentId omegaAgent;
    int gameMode;
    int difficulty;
    int humanPiece;      /* 1 = ALPHA/X, 2 = OMEGA/O (ben nguoi choi o che do PVE) */
    int boardRows;
    int boardCols;
    int board[LOAD_BOARD_MAX_DIM][LOAD_BOARD_MAX_DIM];
} SavedMatchData;

typedef struct {
    char filePath[260];
    long modTime;
    SavedMatchData data;
} LoadSlotEntry;

typedef struct {
    Texture2D bg;
    Texture2D full;
    Texture2D closeNormal;
    Texture2D closeHover;
    Texture2D deleteNormal;
    Texture2D deleteHover;
    Texture2D loadNormal;
    Texture2D loadHover;

    float openAnim;
    LoadSlotEntry *slots;
    int slotCount;
    int slotCapacity;
    int selectedIndex;
    int scrollOffset;

    bool hoverCloseBtn;
    bool hoverLoadBtn;
    bool hoverDeleteBtn;
    bool pressLoadBtn;
    bool pressDeleteBtn;
    bool draggingScrollbar;
    int hoveredRow;
    float statusTimer;
    char statusMsg[64];

    Rectangle panelRect;
    Rectangle slotRects[LOAD_VISIBLE_SLOTS];
    Rectangle scrollTrackRect;
    Rectangle closeBtnRect;
    Rectangle loadBtnRect;
    Rectangle deleteBtnRect;
} LoadUI;

void Load_Init(LoadUI *ui, int screenWidth, int screenHeight);
void Load_Resize(LoadUI *ui, int screenWidth, int screenHeight);
void Load_Open(LoadUI *ui);
void Load_RefreshList(LoadUI *ui);
LoadAction Load_Update(LoadUI *ui, MenuUI *menu, float dt);
void Load_Draw(const LoadUI *ui);
void Load_Unload(LoadUI *ui);

const SavedMatchData *Load_GetSelectedData(const LoadUI *ui);
bool Load_SaveMatch(const SavedMatchData *matchData);

#endif