#ifndef MENU_H
#define MENU_H

#include "raylib.h"
#include <stdbool.h>
// Menu
#define MENU_IMG_BG          "assets/Menu/BACKGROUND.png"
#define MENU_IMG_FULL        "assets/Menu/FULL.png"
#define MENU_IMG_PLAY_1      "assets/Menu/PLAY1.png"
#define MENU_IMG_PLAY_2      "assets/Menu/PLAY2.png"
#define MENU_IMG_LOAD_1      "assets/Menu/LOAD1.png"
#define MENU_IMG_LOAD_2      "assets/Menu/LOAD2.png"
#define MENU_IMG_SETTINGS_1  "assets/Menu/SETTINGS1.png"
#define MENU_IMG_SETTINGS_2  "assets/Menu/SETTINGS2.png"
#define MENU_IMG_EXIT_1      "assets/Menu/EXIT1.png"
#define MENU_IMG_EXIT_2      "assets/Menu/EXIT2.png"
#define MENU_IMG_CLOSE_1     "assets/Menu/75.png"
#define MENU_IMG_CLOSE_2     "assets/Menu/76.png"
// Music
#define MENU_SFX_HOVER       "assets/Sound/Selected.mp3"
#define MENU_SFX_CLICK       "assets/Sound/Confirm.mp3"
#define MENU_BGM_PATH        "assets/Sound/menu.mp3"

#define MENU_BUTTON_COUNT    4
#define MENU_ACCENT_RED      (Color){ 255, 70, 85, 255 }
#define MENU_ACCENT_MINT     (Color){ 44, 229, 167, 255 }

typedef enum {
    MENU_ACTION_NONE = 0,
    MENU_ACTION_PLAY,
    MENU_ACTION_LOAD,
    MENU_ACTION_SETTINGS,
    MENU_ACTION_EXIT
} MenuAction;

typedef struct {
    Texture2D normal;
    Texture2D active;
    Rectangle baseBounds;
    float hoverAnim;
    bool isHovered;
    bool isPressed;
    MenuAction action;
} MenuButton;

typedef struct {
    Texture2D background;
    Texture2D full;
    float openAnim;
    MenuButton buttons[MENU_BUTTON_COUNT];
    MenuButton exitCornerBtn;
    int selectedIndex;
    Sound hoverSfx;
    Sound clickSfx;
    Music bgm;
    bool audioLoaded;
    bool bgmLoaded;
    bool exitRequested;
} MenuUI;

void Menu_Init(MenuUI *menu, int screenWidth, int screenHeight);
void Menu_Resize(MenuUI *menu, int screenWidth, int screenHeight);
void Menu_ResetIntroAnim(MenuUI *menu);
void Menu_StartMusic(MenuUI *menu);
void Menu_StopMusic(MenuUI *menu);
void Menu_Update(MenuUI *menu, MenuAction *outAction, float dt);
void Menu_Draw(const MenuUI *menu);
void Menu_Unload(MenuUI *menu);
bool Menu_IsExitRequested(const MenuUI *menu);

#endif