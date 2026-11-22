#include "Pve.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

static Texture2D LoadTextureSafe(const char *path) {
    Texture2D tex = { 0 };
    if (path != NULL && path[0] != '\0' && FileExists(path)) {
        tex = LoadTexture(path);
        if (tex.id > 0) {
            SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
        }
    }
    return tex;
}

static void UnloadTextureSafe(Texture2D *tex) {
    if (tex != NULL && tex->id > 0) {
        UnloadTexture(*tex);
        tex->id = 0;
    }
}

static void DrawTextureFit(Texture2D tex, Rectangle dest, Color tint) {
    if (tex.id <= 0) return;
    Rectangle src = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
    DrawTexturePro(tex, src, dest, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
}

static float Clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static void PlayPveHoverOnce(MenuUI *menu) {
    if (!menu || !menu->audioLoaded) return;
    StopSound(menu->hoverSfx);
    PlaySound(menu->hoverSfx);
}

static void PlayPveClickOnce(MenuUI *menu) {
    if (!menu || !menu->audioLoaded) return;
    StopSound(menu->hoverSfx);
    StopSound(menu->clickSfx);
    PlaySound(menu->clickSfx);
}

static void KeepPlayBgmRunning(PlayUI *play) {
    if (!play || !play->bgmLoaded) return;
    UpdateMusicStream(play->bgm);
    float totalLen = GetMusicTimeLength(play->bgm);
    float played   = GetMusicTimePlayed(play->bgm);
    if (!IsMusicStreamPlaying(play->bgm) ||
        (totalLen > 0.0f && played >= totalLen - 0.05f)) {
        StopMusicStream(play->bgm);
        SeekMusicStream(play->bgm, 0.0f);
        PlayMusicStream(play->bgm);
    }
}

PvePlayerSide Pve_RollRandomSide(void) {
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = true;
    }
    int roll = GetRandomValue(0, 1000) ^ rand();
    return (roll & 1) ? PVE_SIDE_OMEGA : PVE_SIDE_ALPHA;
}

static HudAgentId PickAiAgentDifferentFrom(HudAgentId playerAgent) {
    HudAgentId pool[3] = { HUD_AGENT_PHOENIX, HUD_AGENT_JETT, HUD_AGENT_SAGE };
    HudAgentId candidates[2];
    int count = 0;
    for (int i = 0; i < 3; i++) {
        if (pool[i] != playerAgent) {
            candidates[count++] = pool[i];
        }
    }
    return candidates[GetRandomValue(0, count - 1)];
}

static void EnterStep(PveUI *ui, PveStep nextStep) {
    ui->step          = nextStep;
    ui->stepAnim      = 0.0f;
    ui->hoveredDiff   = -1;
    ui->hoveredAgent  = -1;
    ui->hoverSaveBtn  = false;
    ui->pressSaveBtn  = false;
    ui->saveHoverAnim = 0.0f;
    for (int i = 0; i < 3; i++) {
        ui->diffHoverAnim[i]  = 0.0f;
        ui->agentHoverAnim[i] = 0.0f;
    }
}

void Pve_Resize(PveUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;
    float sw = (float)screenWidth;
    float sh = (float)screenHeight;

    Texture2D refTex = (ui->diffFull.id > 0) ? ui->diffFull : ui->agentFull;
    float panelW = fminf(780.0f, sw * 0.64f);
    float aspect = (refTex.id > 0 && refTex.width > 0)
                   ? ((float)refTex.height / (float)refTex.width)
                   : 0.56f;
    float panelH = panelW * aspect;

    ui->panelRect = (Rectangle){
        (sw - panelW) * 0.5f,
        (sh - panelH) * 0.5f,
        panelW,
        panelH
    };

    Rectangle p = ui->panelRect;

    float thirdW = p.width / 3.0f;
    for (int i = 0; i < 3; i++) {
        ui->diffRects[i] = (Rectangle){
            p.x + thirdW * (float)i,
            p.y,
            thirdW,
            p.height
        };
    }

    float cardW   = p.width * 0.255f;
    float cardH   = p.height * 0.650f;
    float cardY   = p.y + p.height * 0.225f;
    float startX  = p.x + p.width * 0.088f;
    float gapX    = p.width * 0.030f;

    for (int i = 0; i < 3; i++) {
        ui->agentRects[i] = (Rectangle){
            startX + (cardW + gapX) * (float)i,
            cardY,
            cardW,
            cardH
        };
    }

    ui->nameBoxRect = (Rectangle){
        p.x + p.width * 0.275f,
        p.y + p.height * 0.415f,
        p.width * 0.450f,
        p.height * 0.165f
    };

    ui->saveBtnRect = (Rectangle){
        p.x + p.width * 0.405f,
        p.y + p.height * 0.655f,
        p.width * 0.190f,
        p.height * 0.082f
    };
}

void Pve_Open(PveUI *ui) {
    if (!ui) return;
    memset(&ui->result, 0, sizeof(PveSetupResult));
    ui->result.firstTurnPiece = 1;
    ui->selectedDiff          = 0;
    ui->selectedAgent         = 0;
    ui->inputName[0]          = '\0';
    ui->nameLen               = 0;
    ui->cursorBlink           = 0.0f;
    ui->randomSide            = PVE_SIDE_ALPHA;
    EnterStep(ui, PVE_STEP_DIFFICULTY);
}

void Pve_Init(PveUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;
    memset(ui, 0, sizeof(PveUI));

    ui->diffFull      = LoadTextureSafe(PVE_IMG_DIFF_FULL);
    ui->diffNormal[0] = LoadTextureSafe(PVE_IMG_EASY_1);
    ui->diffHover[0]  = LoadTextureSafe(PVE_IMG_EASY_2);
    ui->diffNormal[1] = LoadTextureSafe(PVE_IMG_MED_1);
    ui->diffHover[1]  = LoadTextureSafe(PVE_IMG_MED_2);
    ui->diffNormal[2] = LoadTextureSafe(PVE_IMG_HARD_1);
    ui->diffHover[2]  = LoadTextureSafe(PVE_IMG_HARD_2);

    ui->agentAlphaBg       = LoadTextureSafe(PVE_IMG_AGENT_ALPHA_BG);
    ui->agentOmegaBg       = LoadTextureSafe(PVE_IMG_AGENT_OMEGA_BG);
    ui->agentFull          = LoadTextureSafe(PVE_IMG_AGENT_FULL);

    ui->agentNormal[0]     = LoadTextureSafe(PVE_IMG_AGENT_0_NORMAL);
    ui->agentHoverAlpha[0] = LoadTextureSafe(PVE_IMG_AGENT_0_ALPHA);
    ui->agentHoverOmega[0] = LoadTextureSafe(PVE_IMG_AGENT_0_OMEGA);

    ui->agentNormal[1]     = LoadTextureSafe(PVE_IMG_AGENT_1_NORMAL);
    ui->agentHoverAlpha[1] = LoadTextureSafe(PVE_IMG_AGENT_1_ALPHA);
    ui->agentHoverOmega[1] = LoadTextureSafe(PVE_IMG_AGENT_1_OMEGA);

    ui->agentNormal[2]     = LoadTextureSafe(PVE_IMG_AGENT_2_NORMAL);
    ui->agentHoverAlpha[2] = LoadTextureSafe(PVE_IMG_AGENT_2_ALPHA);
    ui->agentHoverOmega[2] = LoadTextureSafe(PVE_IMG_AGENT_2_OMEGA);

    ui->alphaBg        = LoadTextureSafe(PVE_IMG_ALPHA_BG);
    ui->alphaFull      = LoadTextureSafe(PVE_IMG_ALPHA_FULL);
    ui->omegaBg        = LoadTextureSafe(PVE_IMG_OMEGA_BG);
    ui->omegaFull      = LoadTextureSafe(PVE_IMG_OMEGA_FULL);
    ui->saveNormal     = LoadTextureSafe(PVE_IMG_SAVE_NORMAL);
    ui->saveHoverAlpha = LoadTextureSafe(PVE_IMG_SAVE_ALPHA);
    ui->saveHoverOmega = LoadTextureSafe(PVE_IMG_SAVE_OMEGA);

    Pve_Open(ui);
    Pve_Resize(ui, screenWidth, screenHeight);
}

static void FinalizePveResult(PveUI *ui) {
    const char *finalPlayerName = (ui->nameLen > 0) ? ui->inputName : "PLAYER";
    const char *botNames[3]     = { "BOT EASY", "BOT MEDIUM", "BOT HARD" };
    const char *botName         = botNames[ui->result.difficulty];

    HudAgentId playerAgent = (HudAgentId)(ui->selectedAgent + 1);
    HudAgentId aiAgent     = PickAiAgentDifferentFrom(playerAgent);

    ui->result.playerSide     = ui->randomSide;
    ui->result.firstTurnPiece = 1;

    if (ui->randomSide == PVE_SIDE_ALPHA) {
        ui->result.alphaAgent = playerAgent;
        ui->result.omegaAgent = aiAgent;
        snprintf(ui->result.alphaName, sizeof(ui->result.alphaName), "%s", finalPlayerName);
        snprintf(ui->result.omegaName, sizeof(ui->result.omegaName), "%s", botName);
    } else {
        ui->result.alphaAgent = aiAgent;
        ui->result.omegaAgent = playerAgent;
        snprintf(ui->result.alphaName, sizeof(ui->result.alphaName), "%s", botName);
        snprintf(ui->result.omegaName, sizeof(ui->result.omegaName), "%s", finalPlayerName);
    }
}

PveAction Pve_Update(PveUI *ui, MenuUI *menu, PlayUI *play, float dt) {
    if (!ui) return PVE_ACTION_NONE;

    KeepPlayBgmRunning(play);

    if (ui->stepAnim < 1.0f) {
        ui->stepAnim += dt * 3.0f;
        if (ui->stepAnim > 1.0f) ui->stepAnim = 1.0f;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE)) {
            ui->stepAnim = 1.0f;
        } else {
            return PVE_ACTION_NONE;
        }
    }

    Vector2 mouse = GetMousePosition();

    if (ui->step == PVE_STEP_DIFFICULTY) {
        int prevHover = ui->hoveredDiff;
        int prevSel   = ui->selectedDiff;
        ui->hoveredDiff = -1;

        for (int i = 0; i < 3; i++) {
            if (CheckCollisionPointRec(mouse, ui->diffRects[i])) {
                ui->hoveredDiff  = i;
                ui->selectedDiff = i;
            }
        }

        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
            ui->selectedDiff = (ui->selectedDiff <= 0) ? 2 : (ui->selectedDiff - 1);
        } else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
            ui->selectedDiff = (ui->selectedDiff + 1) % 3;
        }

        if ((ui->hoveredDiff != -1 && ui->hoveredDiff != prevHover) ||
            (ui->selectedDiff != prevSel)) {
            PlayPveHoverOnce(menu);
        }

        for (int i = 0; i < 3; i++) {
            float target = (ui->hoveredDiff == i || (ui->hoveredDiff == -1 && ui->selectedDiff == i)) ? 1.0f : 0.0f;
            ui->diffHoverAnim[i] += (target - ui->diffHoverAnim[i]) * Clamp01(dt * 16.0f);
        }

        SetMouseCursor((ui->hoveredDiff != -1) ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

        if (IsKeyPressed(KEY_ESCAPE)) {
            PlayPveClickOnce(menu);
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            return PVE_ACTION_BACK;
        }

        bool confirm = (ui->hoveredDiff != -1 && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) ||
                       IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
        if (confirm) {
            PlayPveClickOnce(menu);
            ui->result.difficulty = (PveDifficulty)ui->selectedDiff;
            ui->randomSide        = Pve_RollRandomSide();
            ui->selectedAgent     = 0;
            EnterStep(ui, PVE_STEP_AGENT);
        }

        return PVE_ACTION_NONE;
    }

    if (ui->step == PVE_STEP_AGENT) {
        int prevHover = ui->hoveredAgent;
        int prevSel   = ui->selectedAgent;
        ui->hoveredAgent = -1;

        for (int i = 0; i < 3; i++) {
            if (CheckCollisionPointRec(mouse, ui->agentRects[i])) {
                ui->hoveredAgent  = i;
                ui->selectedAgent = i;
            }
        }

        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
            ui->selectedAgent = (ui->selectedAgent <= 0) ? 2 : (ui->selectedAgent - 1);
        } else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
            ui->selectedAgent = (ui->selectedAgent + 1) % 3;
        }

        if ((ui->hoveredAgent != -1 && ui->hoveredAgent != prevHover) ||
            (ui->selectedAgent != prevSel)) {
            PlayPveHoverOnce(menu);
        }

        for (int i = 0; i < 3; i++) {
            float target = (ui->hoveredAgent == i || (ui->hoveredAgent == -1 && ui->selectedAgent == i)) ? 1.0f : 0.0f;
            ui->agentHoverAnim[i] += (target - ui->agentHoverAnim[i]) * Clamp01(dt * 16.0f);
        }

        SetMouseCursor((ui->hoveredAgent != -1) ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

        if (IsKeyPressed(KEY_ESCAPE)) {
            PlayPveClickOnce(menu);
            EnterStep(ui, PVE_STEP_DIFFICULTY);
            return PVE_ACTION_NONE;
        }

        bool confirm = (ui->hoveredAgent != -1 && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) ||
                       IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
        if (confirm) {
            PlayPveClickOnce(menu);
            ui->inputName[0] = '\0';
            ui->nameLen      = 0;
            ui->cursorBlink  = 0.0f;
            EnterStep(ui, PVE_STEP_NAME);
        }

        return PVE_ACTION_NONE;
    }

    if (ui->step == PVE_STEP_NAME) {
        ui->cursorBlink += dt;
        if (ui->cursorBlink >= 1.0f) ui->cursorBlink -= 1.0f;

        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 125 && ui->nameLen < HUD_NAME_MAX_LEN - 1) {
                if (key >= 'a' && key <= 'z') key -= 32;
                ui->inputName[ui->nameLen++] = (char)key;
                ui->inputName[ui->nameLen]   = '\0';
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) && ui->nameLen > 0) {
            ui->nameLen--;
            ui->inputName[ui->nameLen] = '\0';
        }

        bool prevHoverSave = ui->hoverSaveBtn;
        ui->hoverSaveBtn = CheckCollisionPointRec(mouse, ui->saveBtnRect);
        ui->pressSaveBtn = ui->hoverSaveBtn && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

        if (ui->hoverSaveBtn && !prevHoverSave) {
            PlayPveHoverOnce(menu);
        }

        float saveTarget = ui->hoverSaveBtn ? 1.0f : 0.0f;
        ui->saveHoverAnim += (saveTarget - ui->saveHoverAnim) * Clamp01(dt * 16.0f);

        SetMouseCursor(ui->hoverSaveBtn ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

        if (IsKeyPressed(KEY_ESCAPE)) {
            PlayPveClickOnce(menu);
            EnterStep(ui, PVE_STEP_AGENT);
            return PVE_ACTION_NONE;
        }

        bool confirmSave = (ui->hoverSaveBtn && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) ||
                           IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
        if (confirmSave) {
            PlayPveClickOnce(menu);
            FinalizePveResult(ui);
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            return PVE_ACTION_READY;
        }
    }

    return PVE_ACTION_NONE;
}

void Pve_Draw(const PveUI *ui) {
    if (!ui) return;

    float t       = ui->stepAnim;
    float bgAlpha = Clamp01(t / 0.45f);
    float scale   = 0.94f + 0.06f * (1.0f - powf(1.0f - bgAlpha, 3.0f));
    float fullA   = Clamp01((t - 0.35f) / 0.65f);

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.76f * bgAlpha));

    Rectangle animRect = {
        ui->panelRect.x + ui->panelRect.width * (1.0f - scale) * 0.5f,
        ui->panelRect.y + ui->panelRect.height * (1.0f - scale) * 0.5f,
        ui->panelRect.width * scale,
        ui->panelRect.height * scale
    };

    if (ui->step == PVE_STEP_DIFFICULTY) {
        if (ui->diffFull.id > 0) {
            DrawTextureFit(ui->diffFull, animRect, Fade(WHITE, bgAlpha));
        } else {
            DrawRectangleRec(animRect, Fade((Color){ 18, 22, 28, 245 }, bgAlpha));
        }

        if (t >= 0.90f) {
            for (int i = 0; i < 3; i++) {
                if (ui->diffNormal[i].id > 0) {
                    DrawTextureFit(ui->diffNormal[i], ui->panelRect, WHITE);
                }
                if (ui->diffHover[i].id > 0 && ui->diffHoverAnim[i] > 0.01f) {
                    DrawTextureFit(ui->diffHover[i], ui->panelRect, Fade(WHITE, ui->diffHoverAnim[i]));
                }
            }
        }
        return;
    }

    if (ui->step == PVE_STEP_AGENT) {
        bool isAlpha    = (ui->randomSide == PVE_SIDE_ALPHA);
        Texture2D bgTex = isAlpha ? ui->agentAlphaBg : ui->agentOmegaBg;

        if (bgTex.id > 0) {
            DrawTextureFit(bgTex, animRect, Fade(WHITE, bgAlpha));
        } else {
            DrawRectangleRec(animRect, Fade((Color){ 18, 22, 28, 245 }, bgAlpha));
        }

        if (ui->agentFull.id > 0 && fullA > 0.0f) {
            DrawTextureFit(ui->agentFull, animRect, Fade(WHITE, fullA));
        }

        if (t >= 0.95f) {
            for (int i = 0; i < 3; i++) {
                if (ui->agentNormal[i].id > 0) {
                    DrawTextureFit(ui->agentNormal[i], ui->panelRect, WHITE);
                }
                Texture2D hoverTex = isAlpha ? ui->agentHoverAlpha[i] : ui->agentHoverOmega[i];
                if (hoverTex.id > 0 && ui->agentHoverAnim[i] > 0.01f) {
                    DrawTextureFit(hoverTex, ui->panelRect, Fade(WHITE, ui->agentHoverAnim[i]));
                }
            }
        }
        return;
    }

    if (ui->step == PVE_STEP_NAME) {
        bool isAlpha       = (ui->randomSide == PVE_SIDE_ALPHA);
        Texture2D bgTex    = isAlpha ? ui->alphaBg        : ui->omegaBg;
        Texture2D fullTex  = isAlpha ? ui->alphaFull      : ui->omegaFull;
        Texture2D saveHov  = isAlpha ? ui->saveHoverAlpha : ui->saveHoverOmega;
        Color sideColor    = isAlpha ? PVE_ACCENT_MINT    : PVE_ACCENT_RED;

        if (bgTex.id > 0) {
            DrawTextureFit(bgTex, animRect, Fade(WHITE, bgAlpha));
        } else {
            DrawRectangleRec(animRect, Fade((Color){ 18, 22, 28, 245 }, bgAlpha));
        }

        if (fullTex.id > 0 && fullA > 0.0f) {
            DrawTextureFit(fullTex, animRect, Fade(WHITE, fullA));
        }

        if (t >= 0.95f) {
            const char *sideBadge = isAlpha ? "ASSIGNED TEAM: ALPHA" : "ASSIGNED TEAM: OMEGA";
            int badgeSize = (int)(ui->panelRect.height * 0.046f);
            if (badgeSize < 14) badgeSize = 14;
            int bw = MeasureText(sideBadge, badgeSize);
            DrawText(sideBadge,
                     (int)(ui->panelRect.x + (ui->panelRect.width - bw) * 0.5f),
                     (int)(ui->panelRect.y + ui->panelRect.height * 0.14f),
                     badgeSize,
                     sideColor);

            int fontSize = (int)(ui->nameBoxRect.height * 0.46f);
            if (fontSize < 18) fontSize = 18;

            char displayBuf[HUD_NAME_MAX_LEN + 4];
            if (ui->nameLen == 0) {
                snprintf(displayBuf, sizeof(displayBuf), "ENTER NAME...");
                int tw = MeasureText(displayBuf, fontSize);
                DrawText(displayBuf,
                         (int)(ui->nameBoxRect.x + (ui->nameBoxRect.width - tw) * 0.5f),
                         (int)(ui->nameBoxRect.y + (ui->nameBoxRect.height - fontSize) * 0.5f),
                         fontSize,
                         Fade(LIGHTGRAY, 0.45f));
            } else {
                snprintf(displayBuf, sizeof(displayBuf), "%s%s",
                         ui->inputName, (ui->cursorBlink < 0.5f) ? "_" : "");
                int tw = MeasureText(ui->inputName, fontSize);
                DrawText(displayBuf,
                         (int)(ui->nameBoxRect.x + (ui->nameBoxRect.width - tw) * 0.5f),
                         (int)(ui->nameBoxRect.y + (ui->nameBoxRect.height - fontSize) * 0.5f),
                         fontSize,
                         WHITE);
            }

            Rectangle sRect = ui->saveBtnRect;
            if (ui->pressSaveBtn) {
                sRect.x += 1.5f;
                sRect.y += 1.5f;
            }
            if (ui->saveNormal.id > 0) {
                DrawTextureFit(ui->saveNormal, sRect, WHITE);
            }
            if (saveHov.id > 0 && ui->saveHoverAnim > 0.01f) {
                DrawTextureFit(saveHov, sRect, Fade(WHITE, ui->saveHoverAnim));
            }
        }
    }
}

void Pve_Unload(PveUI *ui) {
    if (!ui) return;

    UnloadTextureSafe(&ui->diffFull);
    for (int i = 0; i < 3; i++) {
        UnloadTextureSafe(&ui->diffNormal[i]);
        UnloadTextureSafe(&ui->diffHover[i]);
    }

    UnloadTextureSafe(&ui->agentAlphaBg);
    UnloadTextureSafe(&ui->agentOmegaBg);
    UnloadTextureSafe(&ui->agentFull);
    for (int i = 0; i < 3; i++) {
        UnloadTextureSafe(&ui->agentNormal[i]);
        UnloadTextureSafe(&ui->agentHoverAlpha[i]);
        UnloadTextureSafe(&ui->agentHoverOmega[i]);
    }

    UnloadTextureSafe(&ui->alphaBg);
    UnloadTextureSafe(&ui->alphaFull);
    UnloadTextureSafe(&ui->omegaBg);
    UnloadTextureSafe(&ui->omegaFull);
    UnloadTextureSafe(&ui->saveNormal);
    UnloadTextureSafe(&ui->saveHoverAlpha);
    UnloadTextureSafe(&ui->saveHoverOmega);
}

const PveSetupResult *Pve_GetResult(const PveUI *ui) {
    if (!ui) return NULL;
    return &ui->result;
}