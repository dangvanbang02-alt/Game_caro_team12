#ifndef HUD_H
#define HUD_H
#include "raylib.h"
#include "Renderer.h"
#include <stdbool.h>
#define HUD_NAME_MAX_LEN      32
#define HUD_SAVE_NAME_MAX_LEN 24
#define HUD_MAX_PIPS          5
// Điểm
#define HUD_PNG_PIP_EMPTY          "assets/Button/none.png"
#define HUD_PNG_PIP_ALPHA          "assets/Button/alpha.png"
#define HUD_PNG_PIP_OMEGA          "assets/Button/omega.png"
// Avatar
#define HUD_PNG_AVATAR_PHOENIX     "assets/Avt/PHOENIX_PROFILE.png"
#define HUD_PNG_AVATAR_JETT        "assets/Avt/JETT_PROFILE.png"
#define HUD_PNG_AVATAR_SAGE        "assets/Avt/SAGE_PROFILE.png"
// Lượt
#define HUD_PNG_TURN_ALPHA         "assets/Avt/ALPHA_TURN_GUIDE.png"
#define HUD_PNG_TURN_OMEGA         "assets/Avt/OMEGA_TURN_GUIDE.png"
// Pause 
#define HUD_PNG_PAUSE_BG           "assets/Pause/BackGround.png"
#define HUD_PNG_PAUSE_FULL         "assets/Pause/Tong.png"
#define HUD_PNG_PAUSE_ICON_NORMAL  "assets/Pause/Pause1.png"
#define HUD_PNG_PAUSE_ICON_HOVER   "assets/Pause/Pause2.png"
#define HUD_PNG_CLOSE_NORMAL       "assets/Pause/X1.png"
#define HUD_PNG_CLOSE_HOVER        "assets/Pause/X2.png"
// Save
#define HUD_PNG_SAVE_NAME_BG       "assets/Pause/14.png"
#define HUD_PNG_SAVE_NAME_FULL     "assets/Pause/87.png"
#define HUD_PNG_SAVE_CONFIRM_NORM  "assets/Pause/33.png"
#define HUD_PNG_SAVE_CONFIRM_HOVER "assets/Pause/35.png"
#define HUD_PNG_NAME_CLOSE_NORM    "assets/Pause/75.png"
#define HUD_PNG_NAME_CLOSE_HOVER   "assets/Pause/76.png"
// Nhạc nền 
#define HUD_BGM_GAMEPLAY           "assets/Sound/trong tran.mp3"

typedef enum {
    HUD_AGENT_NONE = 0,
    HUD_AGENT_PHOENIX,
    HUD_AGENT_JETT,
    HUD_AGENT_SAGE,
    HUD_AGENT_COUNT
} HudAgentId;

typedef enum {
    HUD_PAUSE_ACTION_NONE = 0,
    HUD_PAUSE_ACTION_RESUME,
    HUD_PAUSE_ACTION_SAVE_CONFIRMED,
    HUD_PAUSE_ACTION_EXIT
} HudPauseAction;

typedef struct {
    char name[HUD_NAME_MAX_LEN];
    int piece;
    int wins;
    bool isTurn;
    HudAgentId selectedAgent;
} HudPlayerInfo;

typedef struct {
    Texture2D pipEmpty;
    Texture2D pipAlpha;
    Texture2D pipOmega;
    Texture2D agentAvatar[HUD_AGENT_COUNT];
    Texture2D turnAlphaPng;
    Texture2D turnOmegaPng;
    Texture2D pauseBg;
    Texture2D pauseFull;
    Texture2D pauseIconNormal;
    Texture2D pauseIconHover;
    Texture2D closeNormal;
    Texture2D closeHover;
    Texture2D saveNameBg;        
    Texture2D saveNameFull;      
    Texture2D saveConfirmNormal; 
    Texture2D saveConfirmHover;  
    Texture2D nameCloseNormal;   
    Texture2D nameCloseHover;    
} HudPngSlots;

typedef struct {
    int round;
    HudPlayerInfo p1; 
    HudPlayerInfo p2; 
    Rectangle alphaPanel;
    Rectangle omegaPanel;
    Rectangle bottomBar;
    bool showTurnPng;
    bool prevAlphaTurn;
    float turnBlend;
    float switchAnim;
    float pipAnim[2][HUD_MAX_PIPS];
    bool isPaused;
    float pauseOpenAnim;
    bool hoverPauseIcon;
    bool hoverCloseBtn;
    bool hoverSaveBtn;
    bool hoverExitBtn;
    bool pressSaveBtn;
    bool pressExitBtn;
    float saveNoticeTimer;
    Rectangle pauseIconRect;
    Rectangle pausePanelRect;
    Rectangle closeBtnRect;
    Rectangle saveBtnRect;
    Rectangle exitBtnRect;
    bool isSaveNaming;
    float nameOpenAnim; 
    bool hoverNameCloseBtn;
    bool hoverNameConfirmBtn;
    bool pressNameConfirmBtn;
    bool emptyNameWarning;
    float cursorBlinkTimer;
    char saveNameInput[HUD_SAVE_NAME_MAX_LEN];
    Rectangle namePanelRect;
    Rectangle nameInputBoxRect;
    Rectangle nameCloseBtnRect;
    Rectangle nameConfirmBtnRect;
    Music bgm;
    bool bgmLoaded;
    bool bgmPlaying;
    float bgmBaseVolume;

    HudPngSlots png;
} GameHUD;

void HUD_Init(GameHUD *hud, int screenWidth, int screenHeight);
void HUD_UnloadPngAssets(GameHUD *hud);
void HUD_Resize(GameHUD *hud, int screenWidth, int screenHeight);
void HUD_SetMatchState(GameHUD *hud,
                       int round,
                       int currentTurnPiece,
                       int p1Wins,
                       int p2Wins,
                       const char *p1Name,
                       const char *p2Name);
void HUD_SetSelectedAgents(GameHUD *hud, HudAgentId alphaAgent, HudAgentId omegaAgent);
void HUD_SetTurnPngVisible(GameHUD *hud, bool visible);
void HUD_Update(GameHUD *hud, float dt);
void HUD_Draw(const GameHUD *hud);
void HUD_SetPaused(GameHUD *hud, bool paused);
HudPauseAction HUD_HandlePauseInput(GameHUD *hud);
const char *HUD_GetSaveName(const GameHUD *hud);
void HUD_StartMusic(GameHUD *hud);
void HUD_StopMusic(GameHUD *hud);
void HUD_SetMusicVolume(GameHUD *hud, float volume);

#endif