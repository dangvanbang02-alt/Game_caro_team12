#include "PVP.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

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

static void PlayPvpHoverOnce(MenuUI *menu) {
    if (!menu || !menu->audioLoaded) return;
    StopSound(menu->hoverSfx);
    PlaySound(menu->hoverSfx);
}

static void PlayPvpClickOnce(MenuUI *menu) {
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

static void EnterPvpStep(PvpUI *ui, PvpStep nextStep) {
    ui->step          = nextStep;
    ui->stepAnim      = 0.0f;
    ui->hoveredAgent  = -1;
    ui->selectedAgent = 0;
    ui->hoverSaveBtn  = false;
    ui->pressSaveBtn  = false;
    ui->saveHoverAnim = 0.0f;
    ui->inputName[0]  = '\0';
    ui->nameLen       = 0;
    ui->cursorBlink   = 0.0f;
    for (int i = 0; i < 3; i++) {
        ui->agentHoverAnim[i] = 0.0f;
    }
}

void Pvp_Resize(PvpUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;
    float sw = (float)screenWidth;
    float sh = (float)screenHeight;

    Texture2D refTex = (ui->alphaAgentFull.id > 0) ? ui->alphaAgentFull : ui->omegaAgentFull;
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

    float cardW  = p.width * 0.255f;
    float cardH  = p.height * 0.650f;
    float cardY  = p.y + p.height * 0.225f;
    float startX = p.x + p.width * 0.088f;
    float gapX   = p.width * 0.030f;

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

void Pvp_Open(PvpUI *ui) {
    if (!ui) return;
    memset(&ui->result, 0, sizeof(PvpSetupResult));
    EnterPvpStep(ui, PVP_STEP_ALPHA_AGENT);
}

void Pvp_Init(PvpUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;
    memset(ui, 0, sizeof(PvpUI));

    ui->alphaAgentBg   = LoadTextureSafe(PVP_IMG_ALPHA_AGENT_BG);
    ui->alphaAgentFull = LoadTextureSafe(PVP_IMG_ALPHA_AGENT_FULL);
    ui->omegaAgentBg   = LoadTextureSafe(PVP_IMG_OMEGA_AGENT_BG);
    ui->omegaAgentFull = LoadTextureSafe(PVP_IMG_OMEGA_AGENT_FULL);

    ui->agentNormal[0]     = LoadTextureSafe(PVP_IMG_AGENT_0_NORMAL);
    ui->agentHoverAlpha[0] = LoadTextureSafe(PVP_IMG_AGENT_0_ALPHA);
    ui->agentHoverOmega[0] = LoadTextureSafe(PVP_IMG_AGENT_0_OMEGA);

    ui->agentNormal[1]     = LoadTextureSafe(PVP_IMG_AGENT_1_NORMAL);
    ui->agentHoverAlpha[1] = LoadTextureSafe(PVP_IMG_AGENT_1_ALPHA);
    ui->agentHoverOmega[1] = LoadTextureSafe(PVP_IMG_AGENT_1_OMEGA);

    ui->agentNormal[2]     = LoadTextureSafe(PVP_IMG_AGENT_2_NORMAL);
    ui->agentHoverAlpha[2] = LoadTextureSafe(PVP_IMG_AGENT_2_ALPHA);
    ui->agentHoverOmega[2] = LoadTextureSafe(PVP_IMG_AGENT_2_OMEGA);

    ui->alphaNameBg    = LoadTextureSafe(PVP_IMG_ALPHA_NAME_BG);
    ui->alphaNameFull  = LoadTextureSafe(PVP_IMG_ALPHA_NAME_FULL);
    ui->omegaNameBg    = LoadTextureSafe(PVP_IMG_OMEGA_NAME_BG);
    ui->omegaNameFull  = LoadTextureSafe(PVP_IMG_OMEGA_NAME_FULL);

    ui->saveNormal     = LoadTextureSafe(PVP_IMG_SAVE_NORMAL);
    ui->saveHoverAlpha = LoadTextureSafe(PVP_IMG_SAVE_ALPHA);
    ui->saveHoverOmega = LoadTextureSafe(PVP_IMG_SAVE_OMEGA);

    Pvp_Open(ui);
    Pvp_Resize(ui, screenWidth, screenHeight);
}

static bool UpdateAgentSelectionStep(PvpUI *ui, MenuUI *menu, float dt, bool *outConfirmed, bool *outBack) {
    *outConfirmed = false;
    *outBack      = false;

    Vector2 mouse = GetMousePosition();
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
        PlayPvpHoverOnce(menu);
    }

    for (int i = 0; i < 3; i++) {
        float target = (ui->hoveredAgent == i || (ui->hoveredAgent == -1 && ui->selectedAgent == i)) ? 1.0f : 0.0f;
        ui->agentHoverAnim[i] += (target - ui->agentHoverAnim[i]) * Clamp01(dt * 16.0f);
    }

    SetMouseCursor((ui->hoveredAgent != -1) ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

    if (IsKeyPressed(KEY_ESCAPE)) {
        PlayPvpClickOnce(menu);
        *outBack = true;
        return true;
    }

    bool confirm = (ui->hoveredAgent != -1 && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) ||
                   IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    if (confirm) {
        PlayPvpClickOnce(menu);
        *outConfirmed = true;
        return true;
    }

    return false;
}

static bool UpdateNameInputStep(PvpUI *ui, MenuUI *menu, float dt, bool *outConfirmed, bool *outBack) {
    *outConfirmed = false;
    *outBack      = false;

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

    Vector2 mouse = GetMousePosition();
    bool prevHoverSave = ui->hoverSaveBtn;
    ui->hoverSaveBtn = CheckCollisionPointRec(mouse, ui->saveBtnRect);
    ui->pressSaveBtn = ui->hoverSaveBtn && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    if (ui->hoverSaveBtn && !prevHoverSave) {
        PlayPvpHoverOnce(menu);
    }

    float saveTarget = ui->hoverSaveBtn ? 1.0f : 0.0f;
    ui->saveHoverAnim += (saveTarget - ui->saveHoverAnim) * Clamp01(dt * 16.0f);

    SetMouseCursor(ui->hoverSaveBtn ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

    if (IsKeyPressed(KEY_ESCAPE)) {
        PlayPvpClickOnce(menu);
        *outBack = true;
        return true;
    }

    bool confirmSave = (ui->hoverSaveBtn && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) ||
                       IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    if (confirmSave) {
        PlayPvpClickOnce(menu);
        *outConfirmed = true;
        return true;
    }

    return false;
}

PvpAction Pvp_Update(PvpUI *ui, MenuUI *menu, PlayUI *play, float dt) {
    if (!ui) return PVP_ACTION_NONE;

    KeepPlayBgmRunning(play);

    if (ui->stepAnim < 1.0f) {
        ui->stepAnim += dt * 3.0f;
        if (ui->stepAnim > 1.0f) ui->stepAnim = 1.0f;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE)) {
            ui->stepAnim = 1.0f;
        } else {
            return PVP_ACTION_NONE;
        }
    }

    bool confirmed = false;
    bool back      = false;

    if (ui->step == PVP_STEP_ALPHA_AGENT) {
        if (UpdateAgentSelectionStep(ui, menu, dt, &confirmed, &back)) {
            if (back) {
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
                return PVP_ACTION_BACK;
            }
            if (confirmed) {
                ui->result.alphaAgent = (HudAgentId)ui->selectedAgent;
                EnterPvpStep(ui, PVP_STEP_ALPHA_NAME);
            }
        }
        return PVP_ACTION_NONE;
    }

    if (ui->step == PVP_STEP_ALPHA_NAME) {
        if (UpdateNameInputStep(ui, menu, dt, &confirmed, &back)) {
            if (back) {
                EnterPvpStep(ui, PVP_STEP_ALPHA_AGENT);
                return PVP_ACTION_NONE;
            }
            if (confirmed) {
                const char *name = (ui->nameLen > 0) ? ui->inputName : "ALPHA";
                snprintf(ui->result.alphaName, sizeof(ui->result.alphaName), "%s", name);
                EnterPvpStep(ui, PVP_STEP_OMEGA_AGENT);
            }
        }
        return PVP_ACTION_NONE;
    }

    if (ui->step == PVP_STEP_OMEGA_AGENT) {
        if (UpdateAgentSelectionStep(ui, menu, dt, &confirmed, &back)) {
            if (back) {
                EnterPvpStep(ui, PVP_STEP_ALPHA_NAME);
                return PVP_ACTION_NONE;
            }
            if (confirmed) {
                ui->result.omegaAgent = (HudAgentId)ui->selectedAgent;
                EnterPvpStep(ui, PVP_STEP_OMEGA_NAME);
            }
        }
        return PVP_ACTION_NONE;
    }

    if (ui->step == PVP_STEP_OMEGA_NAME) {
        if (UpdateNameInputStep(ui, menu, dt, &confirmed, &back)) {
            if (back) {
                EnterPvpStep(ui, PVP_STEP_OMEGA_AGENT);
                return PVP_ACTION_NONE;
            }
            if (confirmed) {
                const char *name = (ui->nameLen > 0) ? ui->inputName : "OMEGA";
                snprintf(ui->result.omegaName, sizeof(ui->result.omegaName), "%s", name);
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
                return PVP_ACTION_READY;
            }
        }
    }

    return PVP_ACTION_NONE;
}

void Pvp_Draw(const PvpUI *ui) {
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

    if (ui->step == PVP_STEP_ALPHA_AGENT || ui->step == PVP_STEP_OMEGA_AGENT) {
        bool isAlpha = (ui->step == PVP_STEP_ALPHA_AGENT);
        Texture2D bgTex   = isAlpha ? ui->alphaAgentBg   : ui->omegaAgentBg;
        Texture2D fullTex = isAlpha ? ui->alphaAgentFull : ui->omegaAgentFull;

        if (bgTex.id > 0) {
            DrawTextureFit(bgTex, animRect, Fade(WHITE, bgAlpha));
        } else {
            DrawRectangleRec(animRect, Fade((Color){ 18, 22, 28, 245 }, bgAlpha));
        }

        if (fullTex.id > 0 && fullA > 0.0f) {
            DrawTextureFit(fullTex, animRect, Fade(WHITE, fullA));
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

    if (ui->step == PVP_STEP_ALPHA_NAME || ui->step == PVP_STEP_OMEGA_NAME) {
        bool isAlpha      = (ui->step == PVP_STEP_ALPHA_NAME);
        Texture2D bgTex   = isAlpha ? ui->alphaNameBg    : ui->omegaNameBg;
        Texture2D fullTex = isAlpha ? ui->alphaNameFull  : ui->omegaNameFull;
        Texture2D saveHov = isAlpha ? ui->saveHoverAlpha : ui->saveHoverOmega;

        if (bgTex.id > 0) {
            DrawTextureFit(bgTex, animRect, Fade(WHITE, bgAlpha));
        } else {
            DrawRectangleRec(animRect, Fade((Color){ 18, 22, 28, 245 }, bgAlpha));
        }

        if (fullTex.id > 0 && fullA > 0.0f) {
            DrawTextureFit(fullTex, animRect, Fade(WHITE, fullA));
        }

        if (t >= 0.95f) {
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

void Pvp_Unload(PvpUI *ui) {
    if (!ui) return;

    UnloadTextureSafe(&ui->alphaAgentBg);
    UnloadTextureSafe(&ui->alphaAgentFull);
    UnloadTextureSafe(&ui->omegaAgentBg);
    UnloadTextureSafe(&ui->omegaAgentFull);

    for (int i = 0; i < 3; i++) {
        UnloadTextureSafe(&ui->agentNormal[i]);
        UnloadTextureSafe(&ui->agentHoverAlpha[i]);
        UnloadTextureSafe(&ui->agentHoverOmega[i]);
    }

    UnloadTextureSafe(&ui->alphaNameBg);
    UnloadTextureSafe(&ui->alphaNameFull);
    UnloadTextureSafe(&ui->omegaNameBg);
    UnloadTextureSafe(&ui->omegaNameFull);

    UnloadTextureSafe(&ui->saveNormal);
    UnloadTextureSafe(&ui->saveHoverAlpha);
    UnloadTextureSafe(&ui->saveHoverOmega);
}

const PvpSetupResult *Pvp_GetResult(const PvpUI *ui) {
    if (!ui) return NULL;
    return &ui->result;
}
