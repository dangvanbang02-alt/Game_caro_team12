#ifndef PVE_H
#define PVE_H

#include "raylib.h"
#include "Menu.h"
#include "Play.h"
#include "HUD.h"
#include <stdbool.h>
// Difficult
#define PVE_IMG_DIFF_FULL           "assets/Play/PVE/DIFFICULTY.png"
#define PVE_IMG_EASY_1              "assets/Play/PVE/EASY1.png"
#define PVE_IMG_EASY_2              "assets/Play/PVE/EASY2.png"
#define PVE_IMG_MED_1               "assets/Play/PVE/MEDIUM1.png"
#define PVE_IMG_MED_2               "assets/Play/PVE/MEDIUM2.png"
#define PVE_IMG_HARD_1              "assets/Play/PVE/HARD1.png"
#define PVE_IMG_HARD_2              "assets/Play/PVE/HARD2.png"
// Avt
#define PVE_IMG_AGENT_ALPHA_BG      "assets/Play/PVE/7.png"
#define PVE_IMG_AGENT_OMEGA_BG      "assets/Play/PVE/6.png"
#define PVE_IMG_AGENT_FULL          "assets/Play/PVE/GUIDE.png"
#define PVE_IMG_AGENT_0_NORMAL      "assets/Play/PVE/21.png"
#define PVE_IMG_AGENT_0_ALPHA       "assets/Play/PVE/22.png"
#define PVE_IMG_AGENT_0_OMEGA       "assets/Play/PVE/23.png"
#define PVE_IMG_AGENT_1_NORMAL      "assets/Play/PVE/24.png"
#define PVE_IMG_AGENT_1_ALPHA       "assets/Play/PVE/25.png"
#define PVE_IMG_AGENT_1_OMEGA       "assets/Play/PVE/26.png"
#define PVE_IMG_AGENT_2_NORMAL      "assets/Play/PVE/27.png"
#define PVE_IMG_AGENT_2_ALPHA       "assets/Play/PVE/28.png"
#define PVE_IMG_AGENT_2_OMEGA       "assets/Play/PVE/29.png"
// Name
#define PVE_IMG_ALPHA_BG            "assets/Play/PVE/8.png"
#define PVE_IMG_ALPHA_FULL          "assets/Play/PVE/34.png"
#define PVE_IMG_OMEGA_BG            "assets/Play/PVE/9.png"
#define PVE_IMG_OMEGA_FULL          "assets/Play/PVE/36.png"
#define PVE_IMG_SAVE_NORMAL         "assets/Play/PVE/33.png"
#define PVE_IMG_SAVE_ALPHA          "assets/Play/PVE/35.png"
#define PVE_IMG_SAVE_OMEGA          "assets/Play/PVE/37.png"

#define PVE_ACCENT_MINT             (Color){ 44, 229, 167, 255 }
#define PVE_ACCENT_RED              (Color){ 255, 70, 85, 255 }

typedef enum {
    PVE_ACTION_NONE = 0,
    PVE_ACTION_BACK,
    PVE_ACTION_READY
} PveAction;

typedef enum {
    PVE_STEP_DIFFICULTY = 0,
    PVE_STEP_AGENT,
    PVE_STEP_NAME
} PveStep;

typedef enum {
    PVE_DIFF_EASY = 0,
    PVE_DIFF_MEDIUM,
    PVE_DIFF_HARD
} PveDifficulty;

typedef enum {
    PVE_SIDE_ALPHA = 0,
    PVE_SIDE_OMEGA = 1
} PvePlayerSide;

typedef struct {
    PveDifficulty difficulty;
    PvePlayerSide playerSide;
    int firstTurnPiece;
    HudAgentId alphaAgent;
    HudAgentId omegaAgent;
    char alphaName[HUD_NAME_MAX_LEN];
    char omegaName[HUD_NAME_MAX_LEN];
} PveSetupResult;

typedef struct {
    Texture2D diffFull;
    Texture2D diffNormal[3];
    Texture2D diffHover[3];

    Texture2D agentAlphaBg;
    Texture2D agentOmegaBg;
    Texture2D agentFull;
    Texture2D agentNormal[3];
    Texture2D agentHoverAlpha[3];
    Texture2D agentHoverOmega[3];

    Texture2D alphaBg;
    Texture2D alphaFull;
    Texture2D omegaBg;
    Texture2D omegaFull;
    Texture2D saveNormal;
    Texture2D saveHoverAlpha;
    Texture2D saveHoverOmega;

    PveStep step;
    float stepAnim;

    int hoveredDiff;
    int selectedDiff;
    float diffHoverAnim[3];

    int hoveredAgent;
    int selectedAgent;
    float agentHoverAnim[3];

    PvePlayerSide randomSide;
    char inputName[HUD_NAME_MAX_LEN];
    int nameLen;
    float cursorBlink;
    bool hoverSaveBtn;
    bool pressSaveBtn;
    float saveHoverAnim;

    PveSetupResult result;

    Rectangle panelRect;
    Rectangle diffRects[3];
    Rectangle agentRects[3];
    Rectangle nameBoxRect;
    Rectangle saveBtnRect;
} PveUI;

void Pve_Init(PveUI *ui, int screenWidth, int screenHeight);
void Pve_Resize(PveUI *ui, int screenWidth, int screenHeight);
void Pve_Open(PveUI *ui);
PvePlayerSide Pve_RollRandomSide(void);
PveAction Pve_Update(PveUI *ui, MenuUI *menu, PlayUI *play, float dt);
void Pve_Draw(const PveUI *ui);
void Pve_Unload(PveUI *ui);
const PveSetupResult *Pve_GetResult(const PveUI *ui);

#endif