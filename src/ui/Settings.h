#ifndef SETTINGS_H
#define SETTINGS_H
#include "raylib.h"
#include "Menu.h"
#define SETTINGS_NAME "Settings"
#define SETTINGS_VERSION "1.0"
#define Texture2D Texture
#define SETTINGS_SLIDER_STEPS      9
// Setting
#define SETTINGS_IMG_BG            "assets/Setting/13.png"
#define SETTINGS_IMG_FULL          "assets/Setting/SETTING_GUIDE.png"
#define SETTINGS_IMG_CLOSE_NORMAL  "assets/Setting/75.png"
#define SETTINGS_IMG_CLOSE_HOVER   "assets/Setting/76.png"
// Volume
#define SETTINGS_IMG_BAR_0         "assets/Setting/85.png"
#define SETTINGS_IMG_BAR_1         "assets/Setting/84.png"
#define SETTINGS_IMG_BAR_2         "assets/Setting/83.png"
#define SETTINGS_IMG_BAR_3         "assets/Setting/82.png"
#define SETTINGS_IMG_BAR_4         "assets/Setting/81.png"
#define SETTINGS_IMG_BAR_5         "assets/Setting/80.png"
#define SETTINGS_IMG_BAR_6         "assets/Setting/79.png"
#define SETTINGS_IMG_BAR_7         "assets/Setting/78.png"
#define SETTINGS_IMG_BAR_8         "assets/Setting/77.png"
#define SETTINGS_ACCENT_MINT       (Color){ 44, 229, 167, 255 }

typedef enum {
    SETTINGS_ACTION_NONE = 0,
    SETTINGS_ACTION_BACK
} SettingsAction;

typedef struct {
    Texture2D bg;
    Texture2D full;
    Texture2D closeNormal;
    Texture2D closeHover;
    Texture2D barSteps[SETTINGS_SLIDER_STEPS];
    float openAnim;
    int musicLevel;
    int sfxLevel;
    float musicVisualPos;
    float sfxVisualPos;
    int activeRow;
    bool hoverCloseBtn;
    bool hoverMusicBar;
    bool hoverSfxBar;
    bool draggingMusic;
    bool draggingSfx;
    Rectangle panelRect;
    Rectangle closeBtnRect;
    Rectangle musicBarRect;
    Rectangle sfxBarRect;
} SettingsUI;

void Settings_Init(SettingsUI *ui, int screenWidth, int screenHeight);
void Settings_Resize(SettingsUI *ui, int screenWidth, int screenHeight);
void Settings_Open(SettingsUI *ui);
SettingsAction Settings_Update(SettingsUI *ui, MenuUI *menu, float dt);
void Settings_Draw(const SettingsUI *ui);
void Settings_Unload(SettingsUI *ui);
float Settings_GetMusicVolume(const SettingsUI *ui);
float Settings_GetSfxVolume(const SettingsUI *ui);

#endif