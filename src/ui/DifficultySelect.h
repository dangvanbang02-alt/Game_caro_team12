#ifndef DIFFICULTY_SELECT_H
#define DIFFICULTY_SELECT_H

#include "raylib.h"
#include <stdbool.h>

#define DIFFICULTY_COUNT 3

// Đường dẫn tệp .png cố định cho 3 mức độ (sửa lại theo thư mục thực tế của bạn)
#define DIFFICULTY_IMG_EASY   "assets/Difficulty/EASY.png"
#define DIFFICULTY_IMG_MEDIUM "assets/Difficulty/MEDIUM.png"
#define DIFFICULTY_IMG_HARD   "assets/Difficulty/HARD.png"

// Bảng màu đồng bộ phong cách chiến thuật (#0F1923 & #FF4655)
#define DIFF_COLOR_BG         (Color){ 15, 25, 35, 255 }
#define DIFF_COLOR_PANEL      (Color){ 22, 33, 46, 255 }
#define DIFF_COLOR_PANEL_LITE (Color){ 32, 46, 62, 255 }
#define DIFF_COLOR_BORDER     (Color){ 64, 82, 102, 255 }
#define DIFF_COLOR_RED        (Color){ 255, 70, 85, 255 }
#define DIFF_COLOR_EASY       (Color){ 130, 155, 175, 255 }
#define DIFF_COLOR_MEDIUM     (Color){ 56, 178, 138, 255 }
#define DIFF_COLOR_HARD       (Color){ 214, 168, 92, 255 }
#define DIFF_COLOR_CREAM      (Color){ 236, 232, 225, 255 }
#define DIFF_COLOR_MUTED      (Color){ 148, 163, 184, 255 }

typedef enum {
    DIFFICULTY_NONE   = -1,
    DIFFICULTY_EASY   = 0,
    DIFFICULTY_MEDIUM = 1,
    DIFFICULTY_HARD   = 2
} DifficultyLevel;

typedef enum {
    DIFFICULTY_RESULT_NONE = 0,
    DIFFICULTY_RESULT_SELECTED, // Đã chọn xong EASY / MEDIUM / HARD
    DIFFICULTY_RESULT_BACK      // Bấm ESC quay lại
} DifficultySelectResult;

typedef struct {
    DifficultyLevel level;
    const char *label;
    const char *subtitle;
    Color accentColor;
    Texture2D tex;
    Rectangle cardRect;
    float hoverAnim;
} DifficultyCard;

typedef struct {
    DifficultyCard cards[DIFFICULTY_COUNT];
    Rectangle groupRect;

    int focusedIndex;
    DifficultyLevel selectedLevel;

    Sound hoverSound;
    Sound clickSound;
    bool hasHoverSound;
    bool hasClickSound;
} DifficultySelectUI;

void DifficultySelect_Init(DifficultySelectUI *ui, int screenWidth, int screenHeight);
void DifficultySelect_Resize(DifficultySelectUI *ui, int screenWidth, int screenHeight);
void DifficultySelect_SetAudio(DifficultySelectUI *ui, Sound hoverSound, Sound clickSound);
void DifficultySelect_Reset(DifficultySelectUI *ui);
DifficultySelectResult DifficultySelect_Update(DifficultySelectUI *ui, float dt);
void DifficultySelect_Draw(const DifficultySelectUI *ui);
DifficultyLevel DifficultySelect_GetSelected(const DifficultySelectUI *ui);
void DifficultySelect_Unload(DifficultySelectUI *ui);

#endif