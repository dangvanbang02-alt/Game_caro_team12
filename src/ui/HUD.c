#include "HUD.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

#define HUD_COLOR_MINT     (Color){ 44, 229, 167, 255 }
#define HUD_COLOR_PANEL_BG (Color){ 18, 22, 28, 220 }
#define HUD_COLOR_ALPHA    (Color){ 255, 70, 85, 255 }
#define HUD_COLOR_OMEGA    (Color){ 56, 189, 248, 255 }

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

static bool IsValidSaveChar(int key) {
    return isalnum(key) || key == '_' || key == '-' || key == ' ';
}

static void HUD_UpdatePauseLayout(GameHUD *hud, int screenWidth, int screenHeight) {
    float sw = (float)screenWidth;
    float sh = (float)screenHeight;
    float iconSize = fmaxf(38.0f, sw * 0.03f);
    hud->pauseIconRect = (Rectangle){
        sw - iconSize - 24.0f,
        20.0f,
        iconSize,
        iconSize
    };

    Texture2D refTex = (hud->png.pauseFull.id > 0) ? hud->png.pauseFull : hud->png.pauseBg;
    float panelW = fminf(680.0f, sw * 0.56f);
    float aspect = (refTex.id > 0 && refTex.width > 0)
                   ? ((float)refTex.height / (float)refTex.width)
                   : 0.62f;
    float panelH = panelW * aspect;

    hud->pausePanelRect = (Rectangle){
        (sw - panelW) * 0.5f,
        (sh - panelH) * 0.5f,
        panelW,
        panelH
    };

    Rectangle p = hud->pausePanelRect;
    float closeSize = p.width * 0.068f;
    hud->closeBtnRect = (Rectangle){
        p.x + p.width * 0.88f,
        p.y + p.height * 0.09f,
        closeSize,
        closeSize
    };

    float btnW = p.width * 0.44f;
    float btnH = p.height * 0.16f;
    float btnX = p.x + (p.width - btnW) * 0.5f;
    hud->saveBtnRect = (Rectangle){ btnX, p.y + p.height * 0.37f, btnW, btnH };
    hud->exitBtnRect = (Rectangle){ btnX, p.y + p.height * 0.65f, btnW, btnH };
    Texture2D nameRefTex = (hud->png.saveNameFull.id > 0) ? hud->png.saveNameFull : hud->png.saveNameBg;
    float nameW = fminf(560.0f, sw * 0.46f);
    float nameAspect = (nameRefTex.id > 0 && nameRefTex.width > 0)
                       ? ((float)nameRefTex.height / (float)nameRefTex.width)
                       : 0.48f;
    float nameH = nameW * nameAspect;
    hud->namePanelRect = (Rectangle){
        (sw - nameW) * 0.5f,
        (sh - nameH) * 0.5f,
        nameW,
        nameH
    };

    Rectangle np = hud->namePanelRect;
    float nCloseSize = np.width * 0.072f;
    hud->nameCloseBtnRect = (Rectangle){
        np.x + np.width * 0.885f,
        np.y + np.height * 0.085f,
        nCloseSize,
        nCloseSize
    };
    float boxW = np.width * 0.72f;
    float boxH = np.height * 0.22f;
    hud->nameInputBoxRect = (Rectangle){
        np.x + (np.width - boxW) * 0.5f,
        np.y + np.height * 0.39f,
        boxW,
        boxH
    };
    float confirmW = np.width * 0.28f;
    float confirmH = (hud->png.saveConfirmNormal.id > 0 && hud->png.saveConfirmNormal.width > 0)
                     ? confirmW * ((float)hud->png.saveConfirmNormal.height / (float)hud->png.saveConfirmNormal.width)
                     : (np.height * 0.14f);

    hud->nameConfirmBtnRect = (Rectangle){
        np.x + (np.width - confirmW) * 0.5f,
        np.y + np.height * 0.73f,
        confirmW,
        confirmH
    };
}

void HUD_Init(GameHUD *hud, int screenWidth, int screenHeight) {
    if (!hud) return;
    memset(hud, 0, sizeof(GameHUD));

    hud->round = 1;
    strncpy(hud->p1.name, "ALPHA", HUD_NAME_MAX_LEN - 1);
    strncpy(hud->p2.name, "OMEGA", HUD_NAME_MAX_LEN - 1);
    hud->p1.piece = 1;
    hud->p2.piece = 2;
    hud->p1.isTurn = true;
    hud->p2.isTurn = false;
    hud->showTurnPng = true;
    hud->prevAlphaTurn = true;
    hud->bgmBaseVolume = 1.0f;
    hud->png.pipEmpty = LoadTextureSafe(HUD_PNG_PIP_EMPTY);
    hud->png.pipAlpha = LoadTextureSafe(HUD_PNG_PIP_ALPHA);
    hud->png.pipOmega = LoadTextureSafe(HUD_PNG_PIP_OMEGA);
    hud->png.agentAvatar[HUD_AGENT_PHOENIX] = LoadTextureSafe(HUD_PNG_AVATAR_PHOENIX);
    hud->png.agentAvatar[HUD_AGENT_JETT]    = LoadTextureSafe(HUD_PNG_AVATAR_JETT);
    hud->png.agentAvatar[HUD_AGENT_SAGE]    = LoadTextureSafe(HUD_PNG_AVATAR_SAGE);
    hud->png.turnAlphaPng = LoadTextureSafe(HUD_PNG_TURN_ALPHA);
    hud->png.turnOmegaPng = LoadTextureSafe(HUD_PNG_TURN_OMEGA);
    hud->png.pauseBg         = LoadTextureSafe(HUD_PNG_PAUSE_BG);
    hud->png.pauseFull       = LoadTextureSafe(HUD_PNG_PAUSE_FULL);
    hud->png.pauseIconNormal = LoadTextureSafe(HUD_PNG_PAUSE_ICON_NORMAL);
    hud->png.pauseIconHover  = LoadTextureSafe(HUD_PNG_PAUSE_ICON_HOVER);
    hud->png.closeNormal     = LoadTextureSafe(HUD_PNG_CLOSE_NORMAL);
    hud->png.closeHover      = LoadTextureSafe(HUD_PNG_CLOSE_HOVER);
    hud->png.saveNameBg        = LoadTextureSafe(HUD_PNG_SAVE_NAME_BG);
    hud->png.saveNameFull      = LoadTextureSafe(HUD_PNG_SAVE_NAME_FULL);
    hud->png.saveConfirmNormal = LoadTextureSafe(HUD_PNG_SAVE_CONFIRM_NORM);
    hud->png.saveConfirmHover  = LoadTextureSafe(HUD_PNG_SAVE_CONFIRM_HOVER);
    hud->png.nameCloseNormal   = LoadTextureSafe(HUD_PNG_NAME_CLOSE_NORM);
    hud->png.nameCloseHover    = LoadTextureSafe(HUD_PNG_NAME_CLOSE_HOVER);

    if (IsAudioDeviceReady() && FileExists(HUD_BGM_GAMEPLAY)) {
        hud->bgm = LoadMusicStream(HUD_BGM_GAMEPLAY);
        if (hud->bgm.ctxData != NULL) {
            hud->bgm.looping = true;
            hud->bgmLoaded = true;
        }
    }

    HUD_Resize(hud, screenWidth, screenHeight);
}

void HUD_UnloadPngAssets(GameHUD *hud) {
    if (!hud) return;

    UnloadTextureSafe(&hud->png.pipEmpty);
    UnloadTextureSafe(&hud->png.pipAlpha);
    UnloadTextureSafe(&hud->png.pipOmega);

    for (int i = 0; i < HUD_AGENT_COUNT; i++) {
        UnloadTextureSafe(&hud->png.agentAvatar[i]);
    }

    UnloadTextureSafe(&hud->png.turnAlphaPng);
    UnloadTextureSafe(&hud->png.turnOmegaPng);
    UnloadTextureSafe(&hud->png.pauseBg);
    UnloadTextureSafe(&hud->png.pauseFull);
    UnloadTextureSafe(&hud->png.pauseIconNormal);
    UnloadTextureSafe(&hud->png.pauseIconHover);
    UnloadTextureSafe(&hud->png.closeNormal);
    UnloadTextureSafe(&hud->png.closeHover);
    UnloadTextureSafe(&hud->png.saveNameBg);
    UnloadTextureSafe(&hud->png.saveNameFull);
    UnloadTextureSafe(&hud->png.saveConfirmNormal);
    UnloadTextureSafe(&hud->png.saveConfirmHover);
    UnloadTextureSafe(&hud->png.nameCloseNormal);
    UnloadTextureSafe(&hud->png.nameCloseHover);

    if (hud->bgmLoaded) {
        StopMusicStream(hud->bgm);
        UnloadMusicStream(hud->bgm);
        hud->bgmLoaded = false;
        hud->bgmPlaying = false;
    }
}

void HUD_StartMusic(GameHUD *hud) {
    if (!hud || !hud->bgmLoaded) return;
    SeekMusicStream(hud->bgm, 0.0f);
    SetMusicVolume(hud->bgm, hud->bgmBaseVolume);
    PlayMusicStream(hud->bgm);
    hud->bgmPlaying = true;
}

void HUD_StopMusic(GameHUD *hud) {
    if (!hud || !hud->bgmLoaded) return;
    StopMusicStream(hud->bgm);
    hud->bgmPlaying = false;
}

void HUD_SetMusicVolume(GameHUD *hud, float volume) {
    if (!hud) return;
    hud->bgmBaseVolume = Clamp01(volume);
    if (hud->bgmLoaded) {
        float effectiveVol = hud->isPaused ? (hud->bgmBaseVolume * 0.35f) : hud->bgmBaseVolume;
        SetMusicVolume(hud->bgm, effectiveVol);
    }
}

void HUD_Resize(GameHUD *hud, int screenWidth, int screenHeight) {
    if (!hud) return;
    float sw = (float)screenWidth;
    float sh = (float)screenHeight;

    float panelW = fminf(280.0f, sw * 0.21f);
    float panelH = fminf(120.0f, sh * 0.16f);
    float marginX = 24.0f;
    float topY = 74.0f;

    hud->alphaPanel = (Rectangle){ marginX, topY, panelW, panelH };
    hud->omegaPanel = (Rectangle){ sw - marginX - panelW, topY, panelW, panelH };

    float barW = fminf(360.0f, sw * 0.30f);
    float barH = 54.0f;
    hud->bottomBar = (Rectangle){ (sw - barW) * 0.5f, sh - barH - 18.0f, barW, barH };

    HUD_UpdatePauseLayout(hud, screenWidth, screenHeight);
}

void HUD_SetMatchState(GameHUD *hud,
                       int round,
                       int currentTurnPiece,
                       int p1Wins,
                       int p2Wins,
                       const char *p1Name,
                       const char *p2Name) {
    if (!hud) return;
    hud->round = round;
    hud->p1.wins = (p1Wins < 0) ? 0 : (p1Wins > HUD_MAX_PIPS ? HUD_MAX_PIPS : p1Wins);
    hud->p2.wins = (p2Wins < 0) ? 0 : (p2Wins > HUD_MAX_PIPS ? HUD_MAX_PIPS : p2Wins);

    bool alphaTurn = (currentTurnPiece == hud->p1.piece);
    if (alphaTurn != hud->prevAlphaTurn) {
        hud->switchAnim = 1.0f;
        hud->prevAlphaTurn = alphaTurn;
    }
    hud->p1.isTurn = alphaTurn;
    hud->p2.isTurn = !alphaTurn;

    if (p1Name && p1Name[0] != '\0') {
        strncpy(hud->p1.name, p1Name, HUD_NAME_MAX_LEN - 1);
        hud->p1.name[HUD_NAME_MAX_LEN - 1] = '\0';
    }
    if (p2Name && p2Name[0] != '\0') {
        strncpy(hud->p2.name, p2Name, HUD_NAME_MAX_LEN - 1);
        hud->p2.name[HUD_NAME_MAX_LEN - 1] = '\0';
    }
}

void HUD_SetSelectedAgents(GameHUD *hud, HudAgentId alphaAgent, HudAgentId omegaAgent) {
    if (!hud) return;
    hud->p1.selectedAgent = (alphaAgent >= 0 && alphaAgent < HUD_AGENT_COUNT) ? alphaAgent : HUD_AGENT_NONE;
    hud->p2.selectedAgent = (omegaAgent >= 0 && omegaAgent < HUD_AGENT_COUNT) ? omegaAgent : HUD_AGENT_NONE;
}

void HUD_SetTurnPngVisible(GameHUD *hud, bool visible) {
    if (!hud) return;
    hud->showTurnPng = visible;
}

void HUD_SetPaused(GameHUD *hud, bool paused) {
    if (!hud) return;
    hud->isPaused = paused;
    if (paused) {
        hud->pauseOpenAnim = 0.0f;
    } else {
        hud->isSaveNaming        = false;
        hud->nameOpenAnim        = 0.0f;
        hud->hoverCloseBtn       = false;
        hud->hoverSaveBtn        = false;
        hud->hoverExitBtn        = false;
        hud->pressSaveBtn        = false;
        hud->pressExitBtn        = false;
        hud->pressNameConfirmBtn = false;
    }
}

const char *HUD_GetSaveName(const GameHUD *hud) {
    return (hud != NULL) ? hud->saveNameInput : "";
}

static HudPauseAction HandleSaveNamingInput(GameHUD *hud, Vector2 mouse) {
    hud->hoverNameCloseBtn   = CheckCollisionPointRec(mouse, hud->nameCloseBtnRect);
    hud->hoverNameConfirmBtn = CheckCollisionPointRec(mouse, hud->nameConfirmBtnRect);
    hud->pressNameConfirmBtn = hud->hoverNameConfirmBtn && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    bool anyHover = (hud->hoverNameCloseBtn || hud->hoverNameConfirmBtn);
    SetMouseCursor(anyHover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_IBEAM);

    if (IsKeyPressed(KEY_ESCAPE) ||
        (hud->hoverNameCloseBtn && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))) {
        hud->isSaveNaming        = false;
        hud->emptyNameWarning    = false;
        hud->pressNameConfirmBtn = false;
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        return HUD_PAUSE_ACTION_NONE;
    }

    int key = GetCharPressed();
    while (key > 0) {
        size_t len = strlen(hud->saveNameInput);
        if (IsValidSaveChar(key) && len < HUD_SAVE_NAME_MAX_LEN - 1) {
            hud->saveNameInput[len] = (char)key;
            hud->saveNameInput[len + 1] = '\0';
            hud->emptyNameWarning = false;
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        size_t len = strlen(hud->saveNameInput);
        if (len > 0) {
            hud->saveNameInput[len - 1] = '\0';
        }
    }

    bool submit = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
                  (hud->hoverNameConfirmBtn && IsMouseButtonReleased(MOUSE_BUTTON_LEFT));

    if (submit) {
        size_t len = strlen(hud->saveNameInput);
        while (len > 0 && hud->saveNameInput[len - 1] == ' ') {
            hud->saveNameInput[--len] = '\0';
        }

        if (len == 0) {
            hud->emptyNameWarning = true;
            return HUD_PAUSE_ACTION_NONE;
        }

        hud->isSaveNaming        = false;
        hud->emptyNameWarning    = false;
        hud->pressNameConfirmBtn = false;
        hud->saveNoticeTimer     = 2.5f;
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        return HUD_PAUSE_ACTION_SAVE_CONFIRMED;
    }

    return HUD_PAUSE_ACTION_NONE;
}

HudPauseAction HUD_HandlePauseInput(GameHUD *hud) {
    if (!hud) return HUD_PAUSE_ACTION_NONE;

    Vector2 mouse = GetMousePosition();
    hud->hoverPauseIcon      = false;
    hud->hoverCloseBtn       = false;
    hud->hoverSaveBtn        = false;
    hud->hoverExitBtn        = false;
    hud->pressSaveBtn        = false;
    hud->pressExitBtn        = false;
    hud->hoverNameCloseBtn   = false;
    hud->hoverNameConfirmBtn = false;
    hud->pressNameConfirmBtn = false;

    if (hud->isPaused && hud->isSaveNaming) {
        return HandleSaveNamingInput(hud, mouse);
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        HUD_SetPaused(hud, !hud->isPaused);
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        return hud->isPaused ? HUD_PAUSE_ACTION_NONE : HUD_PAUSE_ACTION_RESUME;
    }

    if (!hud->isPaused) {
        hud->hoverPauseIcon = CheckCollisionPointRec(mouse, hud->pauseIconRect);
        SetMouseCursor(hud->hoverPauseIcon ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

        if (hud->hoverPauseIcon && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            HUD_SetPaused(hud, true);
            hud->hoverPauseIcon = false;
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }
        return HUD_PAUSE_ACTION_NONE;
    }

    hud->hoverCloseBtn = CheckCollisionPointRec(mouse, hud->closeBtnRect);
    hud->hoverSaveBtn  = CheckCollisionPointRec(mouse, hud->saveBtnRect);
    hud->hoverExitBtn  = CheckCollisionPointRec(mouse, hud->exitBtnRect);
    hud->pressSaveBtn  = hud->hoverSaveBtn && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    hud->pressExitBtn  = hud->hoverExitBtn && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool anyHover = (hud->hoverCloseBtn || hud->hoverSaveBtn || hud->hoverExitBtn);
    SetMouseCursor(anyHover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (hud->hoverCloseBtn) {
            HUD_SetPaused(hud, false);
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            return HUD_PAUSE_ACTION_RESUME;
        }
        if (hud->hoverSaveBtn) {
            hud->isSaveNaming     = true;
            hud->nameOpenAnim     = 0.0f;
            hud->emptyNameWarning = false;
            hud->cursorBlinkTimer = 0.0f;
            hud->saveNameInput[0] = '\0';
            while (GetCharPressed() > 0) {}
            return HUD_PAUSE_ACTION_NONE;
        }
        if (hud->hoverExitBtn) {
            HUD_SetPaused(hud, false);
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            return HUD_PAUSE_ACTION_EXIT;
        }
    }

    return HUD_PAUSE_ACTION_NONE;
}

void HUD_Update(GameHUD *hud, float dt) {
    if (!hud) return;

    if (hud->bgmLoaded && hud->bgmPlaying) {
        float effectiveVol = hud->isPaused ? (hud->bgmBaseVolume * 0.35f) : hud->bgmBaseVolume;
        SetMusicVolume(hud->bgm, effectiveVol);
        UpdateMusicStream(hud->bgm);

        float totalLen = GetMusicTimeLength(hud->bgm);
        float played   = GetMusicTimePlayed(hud->bgm);

        if (!IsMusicStreamPlaying(hud->bgm) || (totalLen > 0.0f && played >= totalLen - 0.05f)) {
            StopMusicStream(hud->bgm);
            SeekMusicStream(hud->bgm, 0.0f);
            PlayMusicStream(hud->bgm);
        }
    }
    if (hud->saveNoticeTimer > 0.0f) {
        hud->saveNoticeTimer -= dt;
        if (hud->saveNoticeTimer < 0.0f) hud->saveNoticeTimer = 0.0f;
    }

    if (hud->isSaveNaming) {
        hud->cursorBlinkTimer += dt;
        if (hud->nameOpenAnim < 1.0f) {
            hud->nameOpenAnim += dt * 3.8f;
            if (hud->nameOpenAnim > 1.0f) hud->nameOpenAnim = 1.0f;
        }
    }
    if (hud->isPaused) {
        if (hud->pauseOpenAnim < 1.0f) {
            hud->pauseOpenAnim += dt * 3.5f;
            if (hud->pauseOpenAnim > 1.0f) hud->pauseOpenAnim = 1.0f;
        }
        return;
    }
    float targetBlend = hud->p1.isTurn ? 0.0f : 1.0f;
    hud->turnBlend += (targetBlend - hud->turnBlend) * Clamp01(dt * 10.0f);

    if (hud->switchAnim > 0.0f) {
        hud->switchAnim -= dt * 3.0f;
        if (hud->switchAnim < 0.0f) hud->switchAnim = 0.0f;
    }

    for (int i = 0; i < HUD_MAX_PIPS; i++) {
        float t1 = (i < hud->p1.wins) ? 1.0f : 0.0f;
        float t2 = (i < hud->p2.wins) ? 1.0f : 0.0f;
        hud->pipAnim[0][i] += (t1 - hud->pipAnim[0][i]) * Clamp01(dt * 12.0f);
        hud->pipAnim[1][i] += (t2 - hud->pipAnim[1][i]) * Clamp01(dt * 12.0f);
    }
}

static void DrawPlayerPanel(const GameHUD *hud, const HudPlayerInfo *player, Rectangle rect, bool isAlpha) {
    Color accent = isAlpha ? HUD_COLOR_ALPHA : HUD_COLOR_OMEGA;
    Color border = player->isTurn ? accent : Fade(WHITE, 0.25f);

    DrawRectangleRec(rect, HUD_COLOR_PANEL_BG);
    DrawRectangleLinesEx(rect, player->isTurn ? 2.5f : 1.0f, border);

    float avtSize = rect.height - 20.0f;
    Rectangle avtRect = {
        isAlpha ? (rect.x + 10.0f) : (rect.x + rect.width - avtSize - 10.0f),
        rect.y + 10.0f,
        avtSize,
        avtSize
    };

    Texture2D avtTex = hud->png.agentAvatar[player->selectedAgent];
    if (avtTex.id > 0) {
        DrawTextureFit(avtTex, avtRect, WHITE);
    } else {
        DrawRectangleRec(avtRect, Fade(accent, 0.25f));
    }
    DrawRectangleLinesEx(avtRect, 1.5f, accent);

    float textX = isAlpha ? (avtRect.x + avtSize + 12.0f) : (rect.x + 14.0f);
    DrawText(player->name, (int)textX, (int)(rect.y + 14.0f), 20, WHITE);

    float pipSize = 20.0f;
    float pipGap = 6.0f;
    for (int i = 0; i < HUD_MAX_PIPS; i++) {
        Rectangle pipRect = {
            textX + i * (pipSize + pipGap),
            rect.y + rect.height - pipSize - 16.0f,
            pipSize,
            pipSize
        };
        bool filled = (i < player->wins);
        Texture2D pipTex = filled ? (isAlpha ? hud->png.pipAlpha : hud->png.pipOmega) : hud->png.pipEmpty;
        if (pipTex.id > 0) {
            DrawTextureFit(pipTex, pipRect, WHITE);
        } else {
            DrawRectangleRec(pipRect, filled ? accent : DARKGRAY);
        }
    }
}

static void DrawPauseButtonHighlight(Rectangle rect, bool isHover, bool isPressed) {
    if (!isHover && !isPressed) return;

    Rectangle outer = { rect.x - 4.0f, rect.y - 4.0f, rect.width + 8.0f, rect.height + 8.0f };
    Color fill = isPressed ? Fade(HUD_COLOR_MINT, 0.35f) : Fade(HUD_COLOR_MINT, 0.18f);

    DrawRectangleRec(rect, fill);
    DrawRectangleLinesEx(outer, 2.0f, HUD_COLOR_MINT);
}

static void DrawSaveNamingPopup(const GameHUD *hud) {
    float t = hud->nameOpenAnim;
    float bgAlpha = Clamp01(t / 0.45f);
    float scale   = 0.92f + 0.08f * (1.0f - powf(1.0f - bgAlpha, 3.0f));
    float fullAlpha = Clamp01((t - 0.35f) / 0.65f);

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.55f * bgAlpha));

    Rectangle animRect = {
        hud->namePanelRect.x + hud->namePanelRect.width * (1.0f - scale) * 0.5f,
        hud->namePanelRect.y + hud->namePanelRect.height * (1.0f - scale) * 0.5f,
        hud->namePanelRect.width * scale,
        hud->namePanelRect.height * scale
    };

    if (hud->png.saveNameBg.id > 0) {
        DrawTextureFit(hud->png.saveNameBg, animRect, Fade(WHITE, bgAlpha));
    } else {
        DrawRectangleRec(animRect, Fade((Color){ 18, 24, 32, 250 }, bgAlpha));
    }
    if (hud->png.saveNameFull.id > 0 && fullAlpha > 0.0f) {
        DrawTextureFit(hud->png.saveNameFull, animRect, Fade(WHITE, fullAlpha));
    }
    if (t >= 0.95f) {
        Texture2D closeTex = (hud->hoverNameCloseBtn && hud->png.nameCloseHover.id > 0)
                             ? hud->png.nameCloseHover
                             : hud->png.nameCloseNormal;
        if (closeTex.id > 0) {
            DrawTextureFit(closeTex, hud->nameCloseBtnRect, WHITE);
        }
        Color boxBorder = hud->emptyNameWarning ? HUD_COLOR_ALPHA : HUD_COLOR_MINT;
        DrawRectangleLinesEx(hud->nameInputBoxRect, 1.5f, Fade(boxBorder, 0.85f));

        int fontSize = (int)(hud->nameInputBoxRect.height * 0.52f);
        int textX = (int)(hud->nameInputBoxRect.x + 14.0f);
        int textY = (int)(hud->nameInputBoxRect.y + (hud->nameInputBoxRect.height - fontSize) * 0.5f);

        if (hud->saveNameInput[0] == '\0') {
            const char *placeholder = hud->emptyNameWarning ? "PLEASE ENTER A NAME..." : "ENTER SAVE NAME...";
            Color phColor = hud->emptyNameWarning ? HUD_COLOR_ALPHA : Fade(WHITE, 0.35f);
            DrawText(placeholder, textX, textY, fontSize - 2, phColor);
        } else {
            DrawText(hud->saveNameInput, textX, textY, fontSize, WHITE);
        }
        if (((int)(hud->cursorBlinkTimer * 2.0f)) % 2 == 0) {
            int curX = textX + MeasureText(hud->saveNameInput, fontSize) + 3;
            DrawRectangle(curX, textY, 2, fontSize, HUD_COLOR_MINT);
        }
        Texture2D confirmTex = (hud->hoverNameConfirmBtn && hud->png.saveConfirmHover.id > 0)
                               ? hud->png.saveConfirmHover
                               : hud->png.saveConfirmNormal;
        Rectangle btnRect = hud->nameConfirmBtnRect;
        if (hud->pressNameConfirmBtn) {
            btnRect.x += 1.5f;
            btnRect.y += 1.5f;
            btnRect.width -= 3.0f;
            btnRect.height -= 3.0f;
        }
        if (confirmTex.id > 0) {
            DrawTextureFit(confirmTex, btnRect, WHITE);
        }
    }
}

void HUD_Draw(const GameHUD *hud) {
    if (!hud) return;

    DrawPlayerPanel(hud, &hud->p1, hud->alphaPanel, true);
    DrawPlayerPanel(hud, &hud->p2, hud->omegaPanel, false);
    DrawRectangleRec(hud->bottomBar, HUD_COLOR_PANEL_BG);
    DrawRectangleLinesEx(hud->bottomBar, 1.5f, Fade(WHITE, 0.3f));

    char roundText[32];
    snprintf(roundText, sizeof(roundText), "ROUND %d", hud->round);
    int rW = MeasureText(roundText, 20);
    DrawText(roundText,
             (int)(hud->bottomBar.x + (hud->bottomBar.width - rW) * 0.5f),
             (int)(hud->bottomBar.y + 16.0f),
             20,
             WHITE);

    if (hud->showTurnPng) {
        Texture2D turnTex = hud->p1.isTurn ? hud->png.turnAlphaPng : hud->png.turnOmegaPng;
        if (turnTex.id > 0) {
            float tW = 240.0f;
            float tH = tW * ((float)turnTex.height / (float)turnTex.width);
            Rectangle turnRect = {
                (GetScreenWidth() - tW) * 0.5f,
                hud->bottomBar.y - tH - 8.0f,
                tW,
                tH
            };
            DrawTextureFit(turnTex, turnRect, WHITE);
        }
    }
    Texture2D iconTex = (hud->hoverPauseIcon && hud->png.pauseIconHover.id > 0)
                        ? hud->png.pauseIconHover
                        : hud->png.pauseIconNormal;
    if (iconTex.id > 0) {
        DrawTextureFit(iconTex, hud->pauseIconRect, WHITE);
    } else {
        DrawRectangleLinesEx(hud->pauseIconRect, 2.0f, hud->hoverPauseIcon ? HUD_COLOR_MINT : WHITE);
        DrawText("||", (int)(hud->pauseIconRect.x + 12), (int)(hud->pauseIconRect.y + 8), 22,
                 hud->hoverPauseIcon ? HUD_COLOR_MINT : WHITE);
    }
    if (hud->isPaused) {
        float t = hud->pauseOpenAnim;

        float bgAlpha   = Clamp01(t / 0.45f);
        float scale     = 0.93f + 0.07f * (1.0f - powf(1.0f - bgAlpha, 3.0f));
        float fullAlpha = Clamp01((t - 0.35f) / 0.65f);

        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.68f * bgAlpha));

        Rectangle animRect = {
            hud->pausePanelRect.x + hud->pausePanelRect.width * (1.0f - scale) * 0.5f,
            hud->pausePanelRect.y + hud->pausePanelRect.height * (1.0f - scale) * 0.5f,
            hud->pausePanelRect.width * scale,
            hud->pausePanelRect.height * scale
        };
        if (hud->png.pauseBg.id > 0) {
            DrawTextureFit(hud->png.pauseBg, animRect, Fade(WHITE, bgAlpha));
        } else {
            DrawRectangleRec(animRect, Fade((Color){ 24, 28, 34, 245 }, bgAlpha));
        }
        if (hud->png.pauseFull.id > 0 && fullAlpha > 0.0f) {
            DrawTextureFit(hud->png.pauseFull, animRect, Fade(WHITE, fullAlpha));
        }
        if (t >= 0.95f) {
            if (hud->hoverCloseBtn && hud->png.closeHover.id > 0) {
                DrawTextureFit(hud->png.closeHover, hud->closeBtnRect, WHITE);
            } else if (hud->png.pauseFull.id <= 0 && hud->png.closeNormal.id > 0) {
                DrawTextureFit(hud->png.closeNormal, hud->closeBtnRect, WHITE);
            }

            DrawPauseButtonHighlight(hud->saveBtnRect, hud->hoverSaveBtn, hud->pressSaveBtn);
            DrawPauseButtonHighlight(hud->exitBtnRect, hud->hoverExitBtn, hud->pressExitBtn);
        }
        if (hud->saveNoticeTimer > 0.0f) {
            char msg[64];
            snprintf(msg, sizeof(msg), "SAVED AS \"%s\"!", hud->saveNameInput);
            int mW = MeasureText(msg, 18);
            DrawText(msg,
                     (int)(hud->pausePanelRect.x + (hud->pausePanelRect.width - mW) * 0.5f),
                     (int)(hud->exitBtnRect.y + hud->exitBtnRect.height + 16.0f),
                     18,
                     HUD_COLOR_MINT);
        }
        if (hud->isSaveNaming) {
            DrawSaveNamingPopup(hud);
        }
    }
}