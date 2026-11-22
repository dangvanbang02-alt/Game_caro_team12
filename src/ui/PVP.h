#ifndef PVP_H
#define PVP_H

#include "raylib.h"
#include "Menu.h"
#include "Play.h"
#include "HUD.h"
#include <stdbool.h>
// Avt
#define PVP_IMG_ALPHA_AGENT_BG      "assets/Play/PVP/7.png"
#define PVP_IMG_ALPHA_AGENT_FULL    "assets/Play/PVP/alpha.png"
#define PVP_IMG_OMEGA_AGENT_BG      "assets/Play/PVP/6.png"
#define PVP_IMG_OMEGA_AGENT_FULL    "assets/Play/PVP/omega.png"
#define PVP_IMG_AGENT_0_NORMAL      "assets/Play/PVP/21.png"
#define PVP_IMG_AGENT_0_ALPHA       "assets/Play/PVP/22.png"
#define PVP_IMG_AGENT_0_OMEGA       "assets/Play/PVP/23.png"
#define PVP_IMG_AGENT_1_NORMAL      "assets/Play/PVP/24.png"
#define PVP_IMG_AGENT_1_ALPHA       "assets/Play/PVP/25.png"
#define PVP_IMG_AGENT_1_OMEGA       "assets/Play/PVP/26.png"
#define PVP_IMG_AGENT_2_NORMAL      "assets/Play/PVP/27.png"
#define PVP_IMG_AGENT_2_ALPHA       "assets/Play/PVP/28.png"
#define PVP_IMG_AGENT_2_OMEGA       "assets/Play/PVP/29.png"
// Name
#define PVP_IMG_ALPHA_NAME_BG       "assets/Play/PVP/8.png"
#define PVP_IMG_ALPHA_NAME_FULL     "assets/Play/PVP/34.png"
#define PVP_IMG_OMEGA_NAME_BG       "assets/Play/PVP/9.png"
#define PVP_IMG_OMEGA_NAME_FULL     "assets/Play/PVP/36.png"
// Save
#define PVP_IMG_SAVE_NORMAL         "assets/Play/PVP/33.png"
#define PVP_IMG_SAVE_ALPHA          "assets/Play/PVP/35.png"
#define PVP_IMG_SAVE_OMEGA          "assets/Play/PVP/37.png"

#define PVP_ACCENT_MINT             (Color){ 44, 229, 167, 255 }
#define PVP_ACCENT_RED              (Color){ 255, 70, 85, 255 }

typedef enum {
    PVP_ACTION_NONE = 0,
    PVP_ACTION_BACK,
    PVP_ACTION_READY
} PvpAction;

typedef enum {
    PVP_STEP_ALPHA_AGENT = 0,
    PVP_STEP_ALPHA_NAME,
    PVP_STEP_OMEGA_AGENT,
    PVP_STEP_OMEGA_NAME
} PvpStep;

typedef struct {
    HudAgentId alphaAgent;
    HudAgentId omegaAgent;
    char alphaName[HUD_NAME_MAX_LEN];
    char omegaName[HUD_NAME_MAX_LEN];
} PvpSetupResult;

typedef struct {
    Texture2D alphaAgentBg;
    Texture2D alphaAgentFull;
    Texture2D omegaAgentBg;
    Texture2D omegaAgentFull;

    Texture2D agentNormal[3];
    Texture2D agentHoverAlpha[3];
    Texture2D agentHoverOmega[3];

    Texture2D alphaNameBg;
    Texture2D alphaNameFull;
    Texture2D omegaNameBg;
    Texture2D omegaNameFull;

    Texture2D saveNormal;
    Texture2D saveHoverAlpha;
    Texture2D saveHoverOmega;

    PvpStep step;
    float stepAnim;

    int hoveredAgent;
    int selectedAgent;
    float agentHoverAnim[3];

    char inputName[HUD_NAME_MAX_LEN];
    int nameLen;
    float cursorBlink;
    bool hoverSaveBtn;
    bool pressSaveBtn;
    float saveHoverAnim;

    PvpSetupResult result;

    Rectangle panelRect;
    Rectangle agentRects[3];
    Rectangle nameBoxRect;
    Rectangle saveBtnRect;
} PvpUI;

void Pvp_Init(PvpUI *ui, int screenWidth, int screenHeight);
void Pvp_Resize(PvpUI *ui, int screenWidth, int screenHeight);
void Pvp_Open(PvpUI *ui);
PvpAction Pvp_Update(PvpUI *ui, MenuUI *menu, PlayUI *play, float dt);
void Pvp_Draw(const PvpUI *ui);
void Pvp_Unload(PvpUI *ui);
const PvpSetupResult *Pvp_GetResult(const PvpUI *ui);

#endif