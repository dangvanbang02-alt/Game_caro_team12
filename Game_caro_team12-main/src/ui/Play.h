#ifndef PLAY_H
#define PLAY_H

#include "raylib.h"
#include "Menu.h"
#include <stdbool.h>
// Mode
#define PLAY_IMG_BG            "assets/Play/Mode/4.png"
#define PLAY_IMG_FULL          "assets/Play/Mode/GAMEMODE.png"
#define PLAY_IMG_PVE_NORMAL    "assets/Play/Mode/11.png"
#define PLAY_IMG_PVE_HOVER     "assets/Play/Mode/12.png"
#define PLAY_IMG_PVP_NORMAL    "assets/Play/Mode/13.png"
#define PLAY_IMG_PVP_HOVER     "assets/Play/Mode/14.png"
// Music
#define PLAY_BGM_PATH          "assets/Sound/play.mp3"
#define PLAY_SFX_MODE_CLICK    "assets/Sound/mode_click.mp3"

typedef enum {
    PLAY_ACTION_NONE = 0,
    PLAY_ACTION_BACK,
    PLAY_ACTION_SELECT_PVP,
    PLAY_ACTION_SELECT_PVE
} PlayAction;

typedef enum {
    PLAY_MODE_NONE = 0,
    PLAY_MODE_PVP,
    PLAY_MODE_PVE
} PlayGameMode;

typedef struct {
    Texture2D bg;
    Texture2D full;
    Texture2D pvpNormal;
    Texture2D pvpHover;
    Texture2D pveNormal;
    Texture2D pveHover;

    Music bgm;
    bool bgmLoaded;
    float masterMusicVol;
    float bgmFade;

    Sound modeClickSfx;
    bool modeClickLoaded;
    float masterSfxVol;

    float openAnim;
    float pvpHoverAnim;
    float pveHoverAnim;
    int selectedModeIndex;

    bool hoverPvpCard;
    bool hoverPveCard;
    bool pressPvpCard;
    bool pressPveCard;

    PlayGameMode chosenMode;

    Rectangle panelRect;
    Rectangle pvpCardRect;
    Rectangle pveCardRect;
} PlayUI;

void Play_Init(PlayUI *ui, int screenWidth, int screenHeight);
void Play_Resize(PlayUI *ui, int screenWidth, int screenHeight);
void Play_Open(PlayUI *ui);
void Play_SetMusicVolume(PlayUI *ui, float volume);
void Play_SetSfxVolume(PlayUI *ui, float volume);
void Play_StopMusic(PlayUI *ui);
PlayAction Play_Update(PlayUI *ui, MenuUI *menu, float dt);
void Play_Draw(const PlayUI *ui);
void Play_Unload(PlayUI *ui);
PlayGameMode Play_GetChosenMode(const PlayUI *ui);

#endif