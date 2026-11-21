#ifndef GAME_MODE_H
#define GAME_MODE_H

#include "raylib.h"
#include <stdbool.h>

#define GAMEMODE_COUNT 2

// Đường dẫn tệp .png cố định cho phần chọn Game Mode (sửa lại theo thư mục thực tế của bạn)
#define GAMEMODE_IMG_LOGO "assets/GameMode/GAMEMODE_LOGO.png"
#define GAMEMODE_IMG_PVP  "assets/GameMode/PVP.png"
#define GAMEMODE_IMG_PVE  "assets/GameMode/PVE.png"

// Bảng màu đồng bộ phong cách chiến thuật & khung ảnh mẫu
#define GM_COLOR_BG         (Color){ 10, 12, 16, 255 }
#define GM_COLOR_FRAME      (Color){ 20, 24, 30, 255 }
#define GM_COLOR_INNER_BOX  (Color){ 56, 58, 56, 255 }
#define GM_COLOR_BORDER     (Color){ 92, 102, 116, 255 }
#define GM_COLOR_RED        (Color){ 255, 70, 85, 255 }
#define GM_COLOR_GOLD       (Color){ 180, 152, 92, 255 }
#define GM_COLOR_CREAM      (Color){ 236, 232, 225, 255 }
#define GM_COLOR_MUTED      (Color){ 168, 176, 186, 255 }

typedef enum {
    GAME_MODE_NONE = -1,
    GAME_MODE_PVP  = 0,
    GAME_MODE_PVE  = 1
} GameModeType;

typedef enum {
    GAMEMODE_RESULT_NONE = 0,
    GAMEMODE_RESULT_SELECTED, // Đã chọn xong PVP hoặc PVE
    GAMEMODE_RESULT_BACK      // Bấm ESC quay lại
} GameModeResult;

typedef struct {
    GameModeType mode;
    const char *title;
    const char *desc;
    Texture2D tex;
    Rectangle cardRect;
    float hoverAnim;
} GameModeCard;

typedef struct {
    Texture2D logoTex;
    GameModeCard cards[GAMEMODE_COUNT];

    Rectangle outerFrame;
    Rectangle innerBox;
    Rectangle logoRect;

    int focusedIndex;
    GameModeType selectedMode;

    Sound hoverSound;
    Sound clickSound;
    bool hasHoverSound;
    bool hasClickSound;
} GameModeUI;

void GameMode_Init(GameModeUI *ui, int screenWidth, int screenHeight);
void GameMode_Resize(GameModeUI *ui, int screenWidth, int screenHeight);
void GameMode_SetAudio(GameModeUI *ui, Sound hoverSound, Sound clickSound);
void GameMode_Reset(GameModeUI *ui);
GameModeResult GameMode_Update(GameModeUI *ui, float dt);
void GameMode_Draw(const GameModeUI *ui);
GameModeType GameMode_GetSelected(const GameModeUI *ui);
void GameMode_Unload(GameModeUI *ui);

#endif