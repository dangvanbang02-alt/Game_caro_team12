#ifndef GAMEOVER_H
#define GAMEOVER_H

#include "raylib.h"
#include "Menu.h"
#include "HUD.h"

// End
#define GAMEOVER_IMG_ALPHA_BG     "assets/GameOver/100.png"
#define GAMEOVER_IMG_ALPHA_FULL   "assets/GameOver/86.png"
#define GAMEOVER_IMG_OMEGA_BG     "assets/GameOver/103.png"
#define GAMEOVER_IMG_OMEGA_FULL   "assets/GameOver/87.png"
#define GAMEOVER_IMG_DRAW         "assets/GameOver/104.png"
// Sound
#define GAMEOVER_BGM_WIN          "assets/Sound/win.mp3"
#define GAMEOVER_BGM_LOSE         "assets/Sound/loss.mp3"
#define GAMEOVER_BGM_DRAW         "assets/Sound/draw.mp3"

#define GAMEOVER_ACCENT_MINT      (Color){ 44, 229, 167, 255 }
#define GAMEOVER_ACCENT_RED       (Color){ 255, 70, 85, 255 }
#define GAMEOVER_ACCENT_GOLD      (Color){ 255, 210, 90, 255 }

typedef enum {
    GAMEOVER_ACTION_NONE = 0,
    GAMEOVER_ACTION_BACK_TO_MENU
} GameOverAction;

typedef enum {
    GAMEOVER_MUSIC_WIN = 0,
    GAMEOVER_MUSIC_LOSE,
    GAMEOVER_MUSIC_DRAW
} GameOverMusicType;

typedef struct {
    Texture2D alphaBg;
    Texture2D alphaFull;
    Texture2D omegaBg;
    Texture2D omegaFull;
    Texture2D drawImg;
    Music winBgm;
    bool winBgmLoaded;
    Music loseBgm;
    bool loseBgmLoaded;
    Music drawBgm;
    bool drawBgmLoaded;
    GameOverMusicType activeMusic;
    float musicVolume;
    int winnerPiece;
    char winnerName[HUD_NAME_MAX_LEN];
    int alphaScore;
    int omegaScore;
    float openAnim;
    float pulseTimer;
    Rectangle panelRect;
} GameOverUI;

void GameOver_Init(GameOverUI *ui, int screenWidth, int screenHeight);
void GameOver_Resize(GameOverUI *ui, int screenWidth, int screenHeight);
void GameOver_SetMusicVolume(GameOverUI *ui, float volume);
void GameOver_StopMusic(GameOverUI *ui);
void GameOver_Open(GameOverUI *ui, const GameHUD *hud, bool isPveMode, int pvePlayerPiece);
GameOverAction GameOver_Update(GameOverUI *ui, MenuUI *menu, float dt);
void GameOver_Draw(const GameOverUI *ui);
void GameOver_Unload(GameOverUI *ui);

#endif