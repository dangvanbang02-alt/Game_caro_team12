#ifndef AGENT_SELECT_H
#define AGENT_SELECT_H

#include "raylib.h"
#include <stdbool.h>

#define AGENT_COUNT 3

// Đường dẫn tệp .png cố định cho AgentSelect (đổi lại theo thư mục thực tế của bạn)
#define AGENT_IMG_GUIDE        "assets/AgentSelect/GUIDE.png"
#define AGENT_IMG_P1           "assets/AgentSelect/P1.png"
#define AGENT_IMG_P2           "assets/AgentSelect/P2.png"
#define AGENT_IMG_J1           "assets/AgentSelect/J1.png"
#define AGENT_IMG_J2           "assets/AgentSelect/J2.png"
#define AGENT_IMG_S1           "assets/AgentSelect/S1.png"
#define AGENT_IMG_S2           "assets/AgentSelect/S2.png"

// Bảng màu phong cách chiến thuật (#0F1923 & #FF4655)
#define AGENT_COLOR_BG         (Color){ 15, 25, 35, 255 }
#define AGENT_COLOR_CARD       (Color){ 24, 34, 46, 255 }
#define AGENT_COLOR_CARD_HOVER (Color){ 32, 46, 62, 255 }
#define AGENT_COLOR_BORDER     (Color){ 64, 82, 102, 255 }
#define AGENT_COLOR_RED        (Color){ 255, 70, 85, 255 }
#define AGENT_COLOR_ALPHA      (Color){ 56, 178, 138, 255 }
#define AGENT_COLOR_CREAM      (Color){ 236, 232, 225, 255 }
#define AGENT_COLOR_MUTED      (Color){ 148, 163, 184, 255 }

typedef enum {
    AGENT_PHOENIX = 0,
    AGENT_JETT    = 1,
    AGENT_SAGE    = 2
} AgentId;

typedef enum {
    AGENT_SELECT_RESULT_NONE = 0,
    AGENT_SELECT_RESULT_DONE,
    AGENT_SELECT_RESULT_CANCEL
} AgentSelectResult;

typedef struct {
    AgentId id;
    const char *label;
    Texture2D normalTex;   // P1.png / J1.png / S1.png
    Texture2D selectedTex; // P2.png / J2.png / S2.png
    Rectangle cardRect;
    float hoverAnim;
} AgentCard;

typedef struct {
    AgentCard cards[AGENT_COUNT];
    Texture2D guideTex;    // GUIDE.png
    int focusedIndex;
    int pickingSide;   // 0 = chọn cho ALPHA, 1 = chọn cho OMEGA
    int alphaIndex;    // 0..2 (-1 nếu chưa chọn)
    int omegaIndex;    // 0..2 (-1 nếu chưa chọn)
    Rectangle guideBannerRect;
    Rectangle confirmBtnRect;
    float confirmHover;
    Sound hoverSound;
    Sound clickSound;
    bool hasHoverSound;
    bool hasClickSound;
} AgentSelectUI;

void AgentSelect_Init(AgentSelectUI *ui, int screenWidth, int screenHeight);
void AgentSelect_Resize(AgentSelectUI *ui, int screenWidth, int screenHeight);
void AgentSelect_SetAudio(AgentSelectUI *ui, Sound hoverSound, Sound clickSound);
void AgentSelect_Reset(AgentSelectUI *ui);
AgentSelectResult AgentSelect_Update(AgentSelectUI *ui, float dt);
void AgentSelect_Draw(const AgentSelectUI *ui);
Texture2D AgentSelect_GetAlphaPortrait(const AgentSelectUI *ui);
Texture2D AgentSelect_GetOmegaPortrait(const AgentSelectUI *ui);
void AgentSelect_Unload(AgentSelectUI *ui);

#endif