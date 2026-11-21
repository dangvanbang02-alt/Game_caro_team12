#include "AgentSelect.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static float Clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static float LerpFloat(float a, float b, float t) {
    return a + (b - a) * t;
}

static void PlayHoverSfx(const AgentSelectUI *ui) {
    if (ui && ui->hasHoverSound && ui->hoverSound.frameCount > 0) {
        PlaySound(ui->hoverSound);
    }
}

static void PlayClickSfx(const AgentSelectUI *ui) {
    if (ui && ui->hasClickSound && ui->clickSound.frameCount > 0) {
        PlaySound(ui->clickSound);
    }
}

static void LockAgentForSide(AgentSelectUI *ui, int idx) {
    if (!ui || idx < 0 || idx >= AGENT_COUNT) return;

    PlayClickSfx(ui);
    if (ui->pickingSide == 0) {
        ui->alphaIndex = idx;
        ui->pickingSide = 1;
        if (ui->focusedIndex == idx) {
            ui->focusedIndex = (idx + 1) % AGENT_COUNT;
        }
    } else {
        ui->omegaIndex = idx;
    }
}

void AgentSelect_Init(AgentSelectUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;

    memset(ui, 0, sizeof(*ui));

    // Nạp trực tiếp tệp .png kèm theo
    ui->guideTex = LoadTexture(AGENT_IMG_GUIDE);

    const char *names[AGENT_COUNT]         = { "PHOENIX", "JETT", "SAGE" };
    const char *normalPaths[AGENT_COUNT]   = { AGENT_IMG_P1, AGENT_IMG_J1, AGENT_IMG_S1 };
    const char *selectedPaths[AGENT_COUNT] = { AGENT_IMG_P2, AGENT_IMG_J2, AGENT_IMG_S2 };

    for (int i = 0; i < AGENT_COUNT; i++) {
        ui->cards[i].id = (AgentId)i;
        ui->cards[i].label = names[i];
        ui->cards[i].normalTex = LoadTexture(normalPaths[i]);
        ui->cards[i].selectedTex = LoadTexture(selectedPaths[i]);
        ui->cards[i].cardRect = (Rectangle){ 0.0f, 0.0f, 0.0f, 0.0f };
        ui->cards[i].hoverAnim = 0.0f;
    }

    ui->focusedIndex = 0;
    ui->pickingSide = 0;
    ui->alphaIndex = -1;
    ui->omegaIndex = -1;

    AgentSelect_Resize(ui, screenWidth, screenHeight);
}

void AgentSelect_Resize(AgentSelectUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;

    float sw = (float)((screenWidth > 0) ? screenWidth : 1280);
    float sh = (float)((screenHeight > 0) ? screenHeight : 720);

    float bannerW = fminf(340.0f, sw * 0.30f);
    float bannerH = fminf(78.0f, sh * 0.12f);
    ui->guideBannerRect = (Rectangle){ sw * 0.04f, sh * 0.03f, bannerW, bannerH };

    float topArea = sh * 0.19f;
    float bottomArea = sh * 0.16f;
    float availH = sh - topArea - bottomArea;
    if (availH < 180.0f) availH = sh * 0.60f;

    float gap = sw * 0.03f;
    float totalW = sw * 0.78f;
    float cardW = (totalW - gap * (AGENT_COUNT - 1)) / (float)AGENT_COUNT;
    float cardH = availH;
    float startX = (sw - totalW) * 0.5f;

    for (int i = 0; i < AGENT_COUNT; i++) {
        ui->cards[i].cardRect = (Rectangle){
            startX + i * (cardW + gap),
            topArea,
            cardW,
            cardH
        };
    }

    float btnW = 240.0f;
    float btnH = 44.0f;
    ui->confirmBtnRect = (Rectangle){
        (sw - btnW) * 0.5f,
        sh - bottomArea + 14.0f,
        btnW,
        btnH
    };
}

void AgentSelect_SetAudio(AgentSelectUI *ui, Sound hoverSound, Sound clickSound) {
    if (!ui) return;
    ui->hoverSound = hoverSound;
    ui->clickSound = clickSound;
    ui->hasHoverSound = (hoverSound.frameCount > 0);
    ui->hasClickSound = (clickSound.frameCount > 0);
}

void AgentSelect_Reset(AgentSelectUI *ui) {
    if (!ui) return;
    ui->pickingSide = 0;
    ui->alphaIndex = -1;
    ui->omegaIndex = -1;
    ui->focusedIndex = 0;
    ui->confirmHover = 0.0f;
}

AgentSelectResult AgentSelect_Update(AgentSelectUI *ui, float dt) {
    if (!ui) return AGENT_SELECT_RESULT_NONE;
    if (dt <= 0.0f) dt = 0.016f;

    if (IsKeyPressed(KEY_ESCAPE)) {
        PlayClickSfx(ui);
        return AGENT_SELECT_RESULT_CANCEL;
    }

    if (IsKeyPressed(KEY_TAB)) {
        ui->pickingSide = (ui->pickingSide == 0) ? 1 : 0;
        PlayHoverSfx(ui);
    } else if (IsKeyPressed(KEY_ONE)) {
        ui->pickingSide = 0;
        PlayHoverSfx(ui);
    } else if (IsKeyPressed(KEY_TWO)) {
        ui->pickingSide = 1;
        PlayHoverSfx(ui);
    }

    Vector2 mouse = GetMousePosition();
    int hovered = -1;
    for (int i = 0; i < AGENT_COUNT; i++) {
        if (CheckCollisionPointRec(mouse, ui->cards[i].cardRect)) {
            hovered = i;
            break;
        }
    }

    if (hovered != -1 && hovered != ui->focusedIndex) {
        ui->focusedIndex = hovered;
        PlayHoverSfx(ui);
    }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        ui->focusedIndex = (ui->focusedIndex - 1 + AGENT_COUNT) % AGENT_COUNT;
        PlayHoverSfx(ui);
    } else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        ui->focusedIndex = (ui->focusedIndex + 1) % AGENT_COUNT;
        PlayHoverSfx(ui);
    }

    for (int i = 0; i < AGENT_COUNT; i++) {
        bool active = (i == ui->focusedIndex) || (i == ui->alphaIndex) || (i == ui->omegaIndex);
        float target = active ? 1.0f : 0.0f;
        ui->cards[i].hoverAnim = Clamp01(LerpFloat(ui->cards[i].hoverAnim, target, Clamp01(12.0f * dt)));
    }

    if (hovered != -1 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        LockAgentForSide(ui, hovered);
    } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        LockAgentForSide(ui, ui->focusedIndex);
    }

    bool ready = (ui->alphaIndex >= 0 && ui->omegaIndex >= 0);
    bool hoverConfirm = ready && CheckCollisionPointRec(mouse, ui->confirmBtnRect);
    ui->confirmHover = Clamp01(LerpFloat(ui->confirmHover, hoverConfirm ? 1.0f : 0.0f, Clamp01(14.0f * dt)));

    if (ready && hoverConfirm && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlayClickSfx(ui);
        return AGENT_SELECT_RESULT_DONE;
    }

    return AGENT_SELECT_RESULT_NONE;
}

void AgentSelect_Draw(const AgentSelectUI *ui) {
    if (!ui) return;

    ClearBackground(AGENT_COLOR_BG);

    float sw = (float)GetScreenWidth();

    // Vẽ GUIDE.png nếu có, kèm tiêu đề phe đang chọn
    if (ui->guideTex.id > 0) {
        Rectangle src = { 0.0f, 0.0f, (float)ui->guideTex.width, (float)ui->guideTex.height };
        DrawTexturePro(ui->guideTex, src, ui->guideBannerRect, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    }

    const char *sideLabel = (ui->pickingSide == 0) ? "ALPHA" : "OMEGA";
    Color sideCol = (ui->pickingSide == 0) ? AGENT_COLOR_ALPHA : AGENT_COLOR_RED;

    char title[96];
    snprintf(title, sizeof(title), "AGENT SELECTION - %s", sideLabel);
    int titleSize = 26;
    int titleW = MeasureText(title, titleSize);
    DrawText(title, (int)((sw - titleW) * 0.5f), (int)(GetScreenHeight() * 0.055f), titleSize, sideCol);

    const char *aName = (ui->alphaIndex >= 0) ? ui->cards[ui->alphaIndex].label : "---";
    const char *oName = (ui->omegaIndex >= 0) ? ui->cards[ui->omegaIndex].label : "---";
    char statusLine[128];
    snprintf(statusLine, sizeof(statusLine), "ALPHA: %s    |    OMEGA: %s    (Tab / 1-2: Doi phe)", aName, oName);
    int subSize = 15;
    int subW = MeasureText(statusLine, subSize);
    DrawText(statusLine, (int)((sw - subW) * 0.5f), (int)(GetScreenHeight() * 0.11f), subSize, AGENT_COLOR_CREAM);

    for (int i = 0; i < AGENT_COUNT; i++) {
        const AgentCard *card = &ui->cards[i];
        Rectangle r = card->cardRect;
        r.y += -5.0f * card->hoverAnim;

        bool isAlpha = (i == ui->alphaIndex);
        bool isOmega = (i == ui->omegaIndex);
        bool isFocused = (i == ui->focusedIndex);

        // Dùng *2.png khi đang focus hoặc đã chốt chọn, ngược lại dùng *1.png
        Texture2D tex = (isFocused || isAlpha || isOmega) ? card->selectedTex : card->normalTex;
        Color border = isFocused ? AGENT_COLOR_RED :
                       (isAlpha ? AGENT_COLOR_ALPHA :
                       (isOmega ? AGENT_COLOR_RED : AGENT_COLOR_BORDER));

        DrawRectangleRec(r, isFocused ? AGENT_COLOR_CARD_HOVER : AGENT_COLOR_CARD);
        DrawRectangle((int)r.x, (int)r.y, (int)r.width, 3, border);

        if (tex.id > 0) {
            Rectangle src = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
            Rectangle dst = { r.x + 8.0f, r.y + 8.0f, r.width - 16.0f, r.height - 44.0f };
            DrawTexturePro(tex, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
        }

        DrawRectangleLinesEx(r, isFocused ? 2.2f : 1.2f, border);

        int lblSize = 18;
        int lblW = MeasureText(card->label, lblSize);
        DrawText(
            card->label,
            (int)(r.x + (r.width - lblW) * 0.5f),
            (int)(r.y + r.height - 28.0f),
            lblSize,
            isFocused ? AGENT_COLOR_RED : AGENT_COLOR_CREAM
        );

        if (isAlpha || isOmega) {
            const char *tag = (isAlpha && isOmega) ? "A+O" : (isAlpha ? "ALPHA" : "OMEGA");
            Color tagCol = isAlpha ? AGENT_COLOR_ALPHA : AGENT_COLOR_RED;
            DrawRectangle((int)(r.x + 8.0f), (int)(r.y + 8.0f), 62, 22, tagCol);
            DrawText(tag, (int)(r.x + 14.0f), (int)(r.y + 12.0f), 13, AGENT_COLOR_CREAM);
        }
    }

    if (ui->alphaIndex >= 0 && ui->omegaIndex >= 0) {
        Rectangle b = ui->confirmBtnRect;
        Color bFill = (ui->confirmHover > 0.2f) ? AGENT_COLOR_RED : AGENT_COLOR_CARD_HOVER;
        DrawRectangleRec(b, bFill);
        DrawRectangleLinesEx(b, 2.0f, AGENT_COLOR_RED);

        const char *btnTxt = "CONFIRM & PLAY";
        int bSize = 18;
        int bW = MeasureText(btnTxt, bSize);
        DrawText(
            btnTxt,
            (int)(b.x + (b.width - bW) * 0.5f),
            (int)(b.y + (b.height - bSize) * 0.5f),
            bSize,
            AGENT_COLOR_CREAM
        );
    } else {
        const char *tip = "Click hoac Phim Trai/Phai + Enter de chon  |  ESC: Quay lai";
        int tipSize = 15;
        int tipW = MeasureText(tip, tipSize);
        DrawText(tip, (int)((sw - tipW) * 0.5f), (int)(ui->confirmBtnRect.y + 14.0f), tipSize, AGENT_COLOR_MUTED);
    }
}

Texture2D AgentSelect_GetAlphaPortrait(const AgentSelectUI *ui) {
    if (!ui || ui->alphaIndex < 0 || ui->alphaIndex >= AGENT_COUNT) {
        return (Texture2D){ 0 };
    }
    return ui->cards[ui->alphaIndex].selectedTex.id > 0
               ? ui->cards[ui->alphaIndex].selectedTex
               : ui->cards[ui->alphaIndex].normalTex;
}

Texture2D AgentSelect_GetOmegaPortrait(const AgentSelectUI *ui) {
    if (!ui || ui->omegaIndex < 0 || ui->omegaIndex >= AGENT_COUNT) {
        return (Texture2D){ 0 };
    }
    return ui->cards[ui->omegaIndex].selectedTex.id > 0
               ? ui->cards[ui->omegaIndex].selectedTex
               : ui->cards[ui->omegaIndex].normalTex;
}

void AgentSelect_Unload(AgentSelectUI *ui) {
    if (!ui) return;

    if (ui->guideTex.id > 0) {
        UnloadTexture(ui->guideTex);
        ui->guideTex.id = 0;
    }

    for (int i = 0; i < AGENT_COUNT; i++) {
        if (ui->cards[i].normalTex.id > 0) {
            UnloadTexture(ui->cards[i].normalTex);
            ui->cards[i].normalTex.id = 0;
        }
        if (ui->cards[i].selectedTex.id > 0) {
            UnloadTexture(ui->cards[i].selectedTex);
            ui->cards[i].selectedTex.id = 0;
        }
    }
}