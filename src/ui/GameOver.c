#include "GameOver.h"
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

static void CopySubtitleSafe(char *dst, const char *src, const char *fallback) {
    if (!dst) return;
    const char *text = (src && src[0] != '\0') ? src : fallback;
    strncpy(dst, text, GAMEOVER_REASON_MAX_LEN - 1);
    dst[GAMEOVER_REASON_MAX_LEN - 1] = '\0';
}

static void PlayHoverSfx(const GameOverUI *ui) {
    if (ui && ui->hasHoverSound && ui->hoverSound.frameCount > 0) {
        PlaySound(ui->hoverSound);
    }
}

static void PlayClickSfx(const GameOverUI *ui) {
    if (ui && ui->hasClickSound && ui->clickSound.frameCount > 0) {
        PlaySound(ui->clickSound);
    }
}

static Color GetOutcomeAccent(GameOverOutcome outcome) {
    switch (outcome) {
        case GAMEOVER_OUTCOME_ALPHA_WIN: return GO_COLOR_ALPHA;
        case GAMEOVER_OUTCOME_OMEGA_WIN: return GO_COLOR_RED;
        case GAMEOVER_OUTCOME_DRAW:
        default:                         return GO_COLOR_CREAM;
    }
}

static const char *GetOutcomeTitle(GameOverOutcome outcome) {
    switch (outcome) {
        case GAMEOVER_OUTCOME_ALPHA_WIN: return "ALPHA VICTORY";
        case GAMEOVER_OUTCOME_OMEGA_WIN: return "OMEGA VICTORY";
        case GAMEOVER_OUTCOME_DRAW:      return "MATCH DRAW";
        default:                         return "GAME OVER";
    }
}

static void DrawSlantedButton(const GameOverButton *btn, Color accent) {
    Rectangle r = btn->rect;
    float slant = 12.0f;
    float lift = -3.0f * btn->hoverAnim;
    float x = r.x;
    float y = r.y + lift;
    float w = r.width;
    float h = r.height;

    Color fill = btn->primary
                     ? (btn->hoverAnim > 0.2f ? GO_COLOR_RED : Fade(GO_COLOR_RED, 0.85f))
                     : (btn->hoverAnim > 0.2f ? GO_COLOR_PANEL_LITE : GO_COLOR_PANEL);
    Color border = btn->primary
                       ? GO_COLOR_CREAM
                       : (btn->hoverAnim > 0.2f ? accent : GO_COLOR_BORDER);

    Vector2 p0 = { x + slant, y };
    Vector2 p1 = { x + w, y };
    Vector2 p2 = { x + w - slant, y + h };
    Vector2 p3 = { x, y + h };

    DrawTriangle(p0, p3, p1, fill);
    DrawTriangle(p1, p3, p2, fill);

    DrawLineEx(p0, p1, 2.0f, border);
    DrawLineEx(p1, p2, 2.0f, border);
    DrawLineEx(p2, p3, 2.0f, border);
    DrawLineEx(p3, p0, 2.0f, border);

    int fontSize = 18;
    int textW = MeasureText(btn->label, fontSize);
    DrawText(
        btn->label,
        (int)(x + (w - textW) * 0.5f),
        (int)(y + (h - fontSize) * 0.5f),
        fontSize,
        GO_COLOR_CREAM
    );
}

void GameOver_Init(GameOverUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;

    memset(ui, 0, sizeof(*ui));
    ui->active = false;
    ui->outcome = GAMEOVER_OUTCOME_NONE;
    CopySubtitleSafe(ui->subtitle, "FIVE IN A ROW", "FIVE IN A ROW");
    ui->roundNumber = 1;
    ui->hoveredIndex = 0;
    ui->appearAnim = 0.0f;

    ui->btnPlayAgain.label = "PLAY AGAIN";
    ui->btnPlayAgain.primary = true;

    ui->btnAgentSelect.label = "AGENT SELECT";
    ui->btnAgentSelect.primary = false;

    ui->btnMainMenu.label = "MAIN MENU";
    ui->btnMainMenu.primary = false;

    GameOver_Resize(ui, screenWidth, screenHeight);
}

void GameOver_Resize(GameOverUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;

    float sw = (float)((screenWidth > 0) ? screenWidth : 1280);
    float sh = (float)((screenHeight > 0) ? screenHeight : 720);

    float panelW = fminf(620.0f, sw * 0.72f);
    float panelH = fminf(430.0f, sh * 0.68f);
    if (panelW < 320.0f) panelW = sw * 0.85f;
    if (panelH < 280.0f) panelH = sh * 0.75f;

    ui->panelRect = (Rectangle){
        (sw - panelW) * 0.5f,
        (sh - panelH) * 0.5f,
        panelW,
        panelH
    };

    float btnGap = 14.0f;
    float padX = 36.0f;
    float btnW = (panelW - padX * 2.0f - btnGap * 2.0f) / 3.0f;
    float btnH = 46.0f;
    float btnY = ui->panelRect.y + panelH - btnH - 28.0f;

    ui->btnPlayAgain.rect   = (Rectangle){ ui->panelRect.x + padX, btnY, btnW, btnH };
    ui->btnAgentSelect.rect = (Rectangle){ ui->panelRect.x + padX + (btnW + btnGap), btnY, btnW, btnH };
    ui->btnMainMenu.rect    = (Rectangle){ ui->panelRect.x + padX + (btnW + btnGap) * 2.0f, btnY, btnW, btnH };
}

void GameOver_SetAudio(GameOverUI *ui, Sound hoverSound, Sound clickSound) {
    if (!ui) return;
    ui->hoverSound = hoverSound;
    ui->clickSound = clickSound;
    ui->hasHoverSound = (hoverSound.frameCount > 0);
    ui->hasClickSound = (clickSound.frameCount > 0);
}

void GameOver_Open(GameOverUI *ui,
                   GameOverOutcome outcome,
                   const char *subtitle,
                   int roundNumber,
                   int alphaWins,
                   int omegaWins,
                   Texture2D winnerPortrait) {
    if (!ui) return;

    ui->active = true;
    ui->outcome = (outcome != GAMEOVER_OUTCOME_NONE) ? outcome : GAMEOVER_OUTCOME_DRAW;
    CopySubtitleSafe(ui->subtitle, subtitle, "MATCH COMPLETED");
    ui->roundNumber = (roundNumber > 0) ? roundNumber : 1;
    ui->alphaWins = (alphaWins >= 0) ? alphaWins : 0;
    ui->omegaWins = (omegaWins >= 0) ? omegaWins : 0;
    ui->winnerPortrait = winnerPortrait;
    ui->hoveredIndex = 0;
    ui->appearAnim = 0.0f;
}

void GameOver_Close(GameOverUI *ui) {
    if (!ui) return;
    ui->active = false;
    ui->appearAnim = 0.0f;
}

bool GameOver_IsActive(const GameOverUI *ui) {
    return ui ? ui->active : false;
}

static GameOverAction TriggerButtonAt(GameOverUI *ui, int idx) {
    PlayClickSfx(ui);
    ui->active = false;
    switch (idx) {
        case 0:  return GAMEOVER_ACTION_PLAY_AGAIN;
        case 1:  return GAMEOVER_ACTION_AGENT_SELECT;
        case 2:
        default: return GAMEOVER_ACTION_MAIN_MENU;
    }
}

GameOverAction GameOver_Update(GameOverUI *ui, float dt) {
    if (!ui || !ui->active) return GAMEOVER_ACTION_NONE;
    if (dt <= 0.0f) dt = 0.016f;

    ui->appearAnim = Clamp01(LerpFloat(ui->appearAnim, 1.0f, Clamp01(10.0f * dt)));

    GameOverButton *btns[3] = {
        &ui->btnPlayAgain,
        &ui->btnAgentSelect,
        &ui->btnMainMenu
    };

    Vector2 mouse = GetMousePosition();
    int newHover = -1;
    for (int i = 0; i < 3; i++) {
        if (CheckCollisionPointRec(mouse, btns[i]->rect)) {
            newHover = i;
            break;
        }
    }

    if (newHover != -1 && newHover != ui->hoveredIndex) {
        ui->hoveredIndex = newHover;
        PlayHoverSfx(ui);
    }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        ui->hoveredIndex = (ui->hoveredIndex + 2) % 3;
        PlayHoverSfx(ui);
    } else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        ui->hoveredIndex = (ui->hoveredIndex + 1) % 3;
        PlayHoverSfx(ui);
    }

    for (int i = 0; i < 3; i++) {
        float target = (i == ui->hoveredIndex) ? 1.0f : 0.0f;
        btns[i]->hoverAnim = Clamp01(LerpFloat(btns[i]->hoverAnim, target, Clamp01(14.0f * dt)));
    }

    if (newHover != -1 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return TriggerButtonAt(ui, newHover);
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_R)) {
        int idx = (ui->hoveredIndex >= 0 && ui->hoveredIndex < 3) ? ui->hoveredIndex : 0;
        return TriggerButtonAt(ui, idx);
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        return TriggerButtonAt(ui, 2);
    }

    return GAMEOVER_ACTION_NONE;
}

void GameOver_Draw(const GameOverUI *ui) {
    if (!ui || !ui->active) return;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // Lớp phủ làm mờ bàn cờ phía sau
    DrawRectangle(0, 0, sw, sh, GO_COLOR_BG_DIM);

    Rectangle p = ui->panelRect;
    float slideY = (1.0f - ui->appearAnim) * 18.0f;
    p.y += slideY;

    Color accent = GetOutcomeAccent(ui->outcome);
    const char *title = GetOutcomeTitle(ui->outcome);

    // Khung chính + thanh nhấn trên cùng
    DrawRectangleRec(p, GO_COLOR_PANEL);
    DrawRectangleLinesEx(p, 1.8f, GO_COLOR_BORDER);
    DrawRectangle((int)p.x, (int)p.y, (int)p.width, 4, accent);

    // Nhãn ván đấu nhỏ góc trên
    char roundTag[32];
    snprintf(roundTag, sizeof(roundTag), "ROUND %02d", ui->roundNumber);
    DrawText(roundTag, (int)(p.x + 24.0f), (int)(p.y + 16.0f), 15, GO_COLOR_MUTED);

    // Tiêu đề thắng / hòa
    int titleSize = 34;
    int titleW = MeasureText(title, titleSize);
    DrawText(
        title,
        (int)(p.x + (p.width - titleW) * 0.5f),
        (int)(p.y + 42.0f),
        titleSize,
        accent
    );

    // Mô tả ngắn (ví dụ: FIVE IN A ROW)
    int subSize = 16;
    int subW = MeasureText(ui->subtitle, subSize);
    DrawText(
        ui->subtitle,
        (int)(p.x + (p.width - subW) * 0.5f),
        (int)(p.y + 84.0f),
        subSize,
        GO_COLOR_MUTED
    );

    // Khung chân dung bên trái (nếu có) + bảng tỉ số ALPHA / OMEGA bên phải
    float midY = p.y + 118.0f;
    float portSize = 96.0f;
    Rectangle portRect = { p.x + 54.0f, midY, portSize, portSize };

    if (ui->winnerPortrait.id > 0) {
        Rectangle src = {
            0.0f,
            0.0f,
            (float)ui->winnerPortrait.width,
            (float)ui->winnerPortrait.height
        };
        DrawTexturePro(ui->winnerPortrait, src, portRect, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
        DrawRectangleLinesEx(portRect, 1.5f, accent);
    } else {
        DrawRectangleRec(portRect, GO_COLOR_PANEL_LITE);
        DrawRectangleLinesEx(portRect, 1.5f, accent);
        const char *tag = (ui->outcome == GAMEOVER_OUTCOME_ALPHA_WIN) ? "ALPHA" :
                          (ui->outcome == GAMEOVER_OUTCOME_OMEGA_WIN) ? "OMEGA" : "DRAW";
        int tagW = MeasureText(tag, 18);
        DrawText(
            tag,
            (int)(portRect.x + (portSize - tagW) * 0.5f),
            (int)(portRect.y + (portSize - 18) * 0.5f),
            18,
            accent
        );
    }

    float scoreX = portRect.x + portSize + 36.0f;
    char alphaScore[48];
    char omegaScore[48];
    snprintf(alphaScore, sizeof(alphaScore), "ALPHA WINS : %d", ui->alphaWins);
    snprintf(omegaScore, sizeof(omegaScore), "OMEGA WINS : %d", ui->omegaWins);

    DrawText(alphaScore, (int)scoreX, (int)(midY + 12.0f), 20, GO_COLOR_ALPHA);
    DrawText(omegaScore, (int)scoreX, (int)(midY + 46.0f), 20, GO_COLOR_RED);

    // Dịch nút theo slideY nếu đang mở
    GameOverButton b0 = ui->btnPlayAgain;
    GameOverButton b1 = ui->btnAgentSelect;
    GameOverButton b2 = ui->btnMainMenu;
    b0.rect.y += slideY;
    b1.rect.y += slideY;
    b2.rect.y += slideY;

    DrawSlantedButton(&b0, accent);
    DrawSlantedButton(&b1, accent);
    DrawSlantedButton(&b2, accent);
}