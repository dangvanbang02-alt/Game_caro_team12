#ifndef GAME_OVER_H
#define GAME_OVER_H

#include "raylib.h"
#include <stdbool.h>

#define GAMEOVER_REASON_MAX_LEN 64

// Bảng màu đồng bộ phong cách Valorant (#0F1923 & #FF4655)
#define GO_COLOR_BG_DIM       (Color){ 10, 16, 23, 210 }
#define GO_COLOR_PANEL        (Color){ 20, 31, 43, 255 }
#define GO_COLOR_PANEL_LITE   (Color){ 28, 42, 58, 255 }
#define GO_COLOR_BORDER       (Color){ 64, 82, 102, 255 }
#define GO_COLOR_RED          (Color){ 255, 70, 85, 255 }    // #FF4655
#define GO_COLOR_ALPHA        (Color){ 56, 178, 138, 255 }
#define GO_COLOR_CREAM        (Color){ 236, 232, 225, 255 }
#define GO_COLOR_MUTED        (Color){ 148, 163, 184, 255 }

typedef enum {
    GAMEOVER_OUTCOME_NONE = 0,
    GAMEOVER_OUTCOME_ALPHA_WIN,
    GAMEOVER_OUTCOME_OMEGA_WIN,
    GAMEOVER_OUTCOME_DRAW
} GameOverOutcome;

typedef enum {
    GAMEOVER_ACTION_NONE = 0,
    GAMEOVER_ACTION_PLAY_AGAIN,
    GAMEOVER_ACTION_AGENT_SELECT,
    GAMEOVER_ACTION_MAIN_MENU
} GameOverAction;

typedef struct {
    Rectangle rect;
    const char *label;
    bool primary;
    float hoverAnim;
} GameOverButton;

typedef struct {
    bool active;
    GameOverOutcome outcome;
    char subtitle[GAMEOVER_REASON_MAX_LEN];
    int roundNumber;
    int alphaWins;
    int omegaWins;
    Texture2D winnerPortrait;

    Rectangle panelRect;
    GameOverButton btnPlayAgain;
    GameOverButton btnAgentSelect;
    GameOverButton btnMainMenu;

    int hoveredIndex; // -1: none, 0: play again, 1: agent select, 2: main menu
    float appearAnim;

    Sound hoverSound;
    Sound clickSound;
    bool hasHoverSound;
    bool hasClickSound;
} GameOverUI;

void GameOver_Init(GameOverUI *ui, int screenWidth, int screenHeight);
void GameOver_Resize(GameOverUI *ui, int screenWidth, int screenHeight);
void GameOver_SetAudio(GameOverUI *ui, Sound hoverSound, Sound clickSound);
void GameOver_Open(GameOverUI *ui,
                   GameOverOutcome outcome,
                   const char *subtitle,
                   int roundNumber,
                   int alphaWins,
                   int omegaWins,
                   Texture2D winnerPortrait);
void GameOver_Close(GameOverUI *ui);
bool GameOver_IsActive(const GameOverUI *ui);
GameOverAction GameOver_Update(GameOverUI *ui, float dt);
void GameOver_Draw(const GameOverUI *ui);

#endif