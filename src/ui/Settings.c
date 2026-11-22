#include "Settings.h"
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

static int ClampLevel(int level) {
    if (level < 0) return 0;
    if (level >= SETTINGS_SLIDER_STEPS) return SETTINGS_SLIDER_STEPS - 1;
    return level;
}

static float CalculateSliderExactPosFromMouse(Rectangle barRect, float mouseX) {
    float trackStart = barRect.x + barRect.width * 0.12f;
    float trackWidth = barRect.width * 0.76f;
    float ratio = Clamp01((mouseX - trackStart) / trackWidth);
    return ratio * (float)(SETTINGS_SLIDER_STEPS - 1);
}

static void UpdateAndSyncMenuAudio(const SettingsUI *ui, MenuUI *menu) {
    if (!ui || !menu) return;

    float musicVol = Settings_GetMusicVolume(ui);
    float sfxVol   = Settings_GetSfxVolume(ui);

    if (menu->bgmLoaded) {
        SetMusicVolume(menu->bgm, musicVol);
        UpdateMusicStream(menu->bgm);
        float totalLen = GetMusicTimeLength(menu->bgm);
        float played   = GetMusicTimePlayed(menu->bgm);
        if (!IsMusicStreamPlaying(menu->bgm) ||
            (totalLen > 0.0f && played >= totalLen - 0.05f)) {
            StopMusicStream(menu->bgm);
            SeekMusicStream(menu->bgm, 0.0f);
            PlayMusicStream(menu->bgm);
        }
    }

    if (menu->audioLoaded) {
        SetSoundVolume(menu->hoverSfx, sfxVol);
        SetSoundVolume(menu->clickSfx, sfxVol);
    }
}

static void PlayMenuPreviewSfx(MenuUI *menu, bool isClick) {
    if (!menu || !menu->audioLoaded) return;
    Sound sfx = isClick ? menu->clickSfx : menu->hoverSfx;
    StopSound(sfx);
    PlaySound(sfx);
}

void Settings_Resize(SettingsUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;
    float sw = (float)screenWidth;
    float sh = (float)screenHeight;

    Texture2D refTex = (ui->full.id > 0) ? ui->full : ui->bg;
    float panelW = fminf(680.0f, sw * 0.56f);
    float aspect = (refTex.id > 0 && refTex.width > 0)
                   ? ((float)refTex.height / (float)refTex.width)
                   : 0.62f;
    float panelH = panelW * aspect;

    ui->panelRect = (Rectangle){
        (sw - panelW) * 0.5f,
        (sh - panelH) * 0.5f,
        panelW,
        panelH
    };

    Rectangle p = ui->panelRect;

    float closeSize = p.width * 0.068f;
    ui->closeBtnRect = (Rectangle){
        p.x + p.width * 0.885f,
        p.y + p.height * 0.088f,
        closeSize,
        closeSize
    };

    float barW = p.width * 0.52f;
    float barH = (ui->barSteps[8].id > 0 && ui->barSteps[8].width > 0)
                 ? barW * ((float)ui->barSteps[8].height / (float)ui->barSteps[8].width)
                 : (p.height * 0.14f);
    float barX = p.x + p.width * 0.36f;

    ui->musicBarRect = (Rectangle){
        barX,
        p.y + p.height * 0.43f - barH * 0.5f,
        barW,
        barH
    };

    ui->sfxBarRect = (Rectangle){
        barX,
        p.y + p.height * 0.70f - barH * 0.5f,
        barW,
        barH
    };
}

void Settings_Open(SettingsUI *ui) {
    if (!ui) return;
    ui->openAnim       = 0.0f;
    ui->activeRow      = 0;
    ui->musicVisualPos = (float)ui->musicLevel;
    ui->sfxVisualPos   = (float)ui->sfxLevel;
    ui->hoverCloseBtn  = false;
    ui->hoverMusicBar  = false;
    ui->hoverSfxBar    = false;
    ui->draggingMusic  = false;
    ui->draggingSfx    = false;
}

void Settings_Init(SettingsUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;
    memset(ui, 0, sizeof(SettingsUI));

    ui->musicLevel     = SETTINGS_SLIDER_STEPS - 1;
    ui->sfxLevel       = SETTINGS_SLIDER_STEPS - 1;
    ui->musicVisualPos = (float)ui->musicLevel;
    ui->sfxVisualPos   = (float)ui->sfxLevel;

    ui->bg          = LoadTextureSafe(SETTINGS_IMG_BG);
    ui->full        = LoadTextureSafe(SETTINGS_IMG_FULL);
    ui->closeNormal = LoadTextureSafe(SETTINGS_IMG_CLOSE_NORMAL);
    ui->closeHover  = LoadTextureSafe(SETTINGS_IMG_CLOSE_HOVER);

    const char *barPaths[SETTINGS_SLIDER_STEPS] = {
        SETTINGS_IMG_BAR_0,
        SETTINGS_IMG_BAR_1,
        SETTINGS_IMG_BAR_2,
        SETTINGS_IMG_BAR_3,
        SETTINGS_IMG_BAR_4,
        SETTINGS_IMG_BAR_5,
        SETTINGS_IMG_BAR_6,
        SETTINGS_IMG_BAR_7,
        SETTINGS_IMG_BAR_8
    };

    for (int i = 0; i < SETTINGS_SLIDER_STEPS; i++) {
        ui->barSteps[i] = LoadTextureSafe(barPaths[i]);
    }

    Settings_Open(ui);
    Settings_Resize(ui, screenWidth, screenHeight);
}

SettingsAction Settings_Update(SettingsUI *ui, MenuUI *menu, float dt) {
    if (!ui) return SETTINGS_ACTION_NONE;

    UpdateAndSyncMenuAudio(ui, menu);

    if (ui->openAnim < 1.0f) {
        ui->openAnim += dt * 3.5f;
        if (ui->openAnim > 1.0f) ui->openAnim = 1.0f;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE)) {
            ui->openAnim = 1.0f;
        } else {
            return SETTINGS_ACTION_NONE;
        }
    }

    Vector2 mouse = GetMousePosition();
    bool prevHoverClose = ui->hoverCloseBtn;
    int prevSfxLevel    = ui->sfxLevel;

    ui->hoverCloseBtn = CheckCollisionPointRec(mouse, ui->closeBtnRect);
    ui->hoverMusicBar = CheckCollisionPointRec(mouse, ui->musicBarRect);
    ui->hoverSfxBar   = CheckCollisionPointRec(mouse, ui->sfxBarRect);

    if (ui->hoverCloseBtn && !prevHoverClose) {
        PlayMenuPreviewSfx(menu, false);
    }

    if (IsKeyPressed(KEY_ESCAPE) ||
        (ui->hoverCloseBtn && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))) {
        PlayMenuPreviewSfx(menu, true);
        ui->draggingMusic = false;
        ui->draggingSfx   = false;
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        return SETTINGS_ACTION_BACK;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (ui->hoverMusicBar) {
            ui->draggingMusic = true;
            ui->activeRow = 0;
        } else if (ui->hoverSfxBar) {
            ui->draggingSfx = true;
            ui->activeRow = 1;
        }
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        ui->draggingMusic = false;
        ui->draggingSfx   = false;
    }

    float targetMusicPos = (float)ui->musicLevel;
    float targetSfxPos   = (float)ui->sfxLevel;

    if (ui->draggingMusic) {
        float exactPos = CalculateSliderExactPosFromMouse(ui->musicBarRect, mouse.x);
        ui->musicLevel = ClampLevel((int)roundf(exactPos));
        targetMusicPos = exactPos;
    }
    if (ui->draggingSfx) {
        float exactPos = CalculateSliderExactPosFromMouse(ui->sfxBarRect, mouse.x);
        ui->sfxLevel   = ClampLevel((int)roundf(exactPos));
        targetSfxPos   = exactPos;
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) ||
        IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        ui->activeRow = 1 - ui->activeRow;
        PlayMenuPreviewSfx(menu, false);
    }

    int delta = 0;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))  delta = -1;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) delta = 1;

    if (delta != 0) {
        if (ui->activeRow == 0) {
            ui->musicLevel = ClampLevel(ui->musicLevel + delta);
            targetMusicPos = (float)ui->musicLevel;
        } else {
            ui->sfxLevel   = ClampLevel(ui->sfxLevel + delta);
            targetSfxPos   = (float)ui->sfxLevel;
        }
    }

    UpdateAndSyncMenuAudio(ui, menu);
    if (ui->sfxLevel != prevSfxLevel) {
        PlayMenuPreviewSfx(menu, false);
    }

    float smooth = Clamp01(dt * 24.0f);
    ui->musicVisualPos += (targetMusicPos - ui->musicVisualPos) * smooth;
    ui->sfxVisualPos   += (targetSfxPos   - ui->sfxVisualPos)   * smooth;

    bool anyHover = (ui->hoverCloseBtn || ui->hoverMusicBar || ui->hoverSfxBar ||
                     ui->draggingMusic || ui->draggingSfx);
    SetMouseCursor(anyHover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

    return SETTINGS_ACTION_NONE;
}

static void DrawSmoothSliderBar(const SettingsUI *ui, Rectangle rect, float visualPos, bool isActive) {
    int lowIdx  = ClampLevel((int)floorf(visualPos));
    int highIdx = ClampLevel(lowIdx + 1);
    float blend = Clamp01(visualPos - (float)lowIdx);

    Texture2D lowTex  = ui->barSteps[lowIdx];
    Texture2D highTex = ui->barSteps[highIdx];

    if (lowTex.id > 0) {
        DrawTextureFit(lowTex, rect, WHITE);
    }
    if (highTex.id > 0 && highIdx != lowIdx && blend > 0.01f) {
        DrawTextureFit(highTex, rect, Fade(WHITE, blend));
    }

    if (isActive) {
        DrawRectangleLinesEx(rect, 1.5f, Fade(SETTINGS_ACCENT_MINT, 0.75f));
    }
}

void Settings_Draw(const SettingsUI *ui) {
    if (!ui) return;

    float t = ui->openAnim;

    float bgAlpha   = Clamp01(t / 0.45f);
    float scale     = 0.93f + 0.07f * (1.0f - powf(1.0f - bgAlpha, 3.0f));
    float fullAlpha = Clamp01((t - 0.35f) / 0.65f);

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.72f * bgAlpha));

    Rectangle animRect = {
        ui->panelRect.x + ui->panelRect.width * (1.0f - scale) * 0.5f,
        ui->panelRect.y + ui->panelRect.height * (1.0f - scale) * 0.5f,
        ui->panelRect.width * scale,
        ui->panelRect.height * scale
    };

    if (ui->bg.id > 0) {
        DrawTextureFit(ui->bg, animRect, Fade(WHITE, bgAlpha));
    } else {
        DrawRectangleRec(animRect, Fade((Color){ 20, 26, 34, 245 }, bgAlpha));
    }

    if (ui->full.id > 0 && fullAlpha > 0.0f) {
        DrawTextureFit(ui->full, animRect, Fade(WHITE, fullAlpha));
    }

    if (t >= 0.95f) {
        Texture2D closeTex = (ui->hoverCloseBtn && ui->closeHover.id > 0)
                             ? ui->closeHover
                             : ui->closeNormal;
        if (closeTex.id > 0) {
            DrawTextureFit(closeTex, ui->closeBtnRect, WHITE);
        }

        DrawSmoothSliderBar(ui, ui->musicBarRect, ui->musicVisualPos, (ui->hoverMusicBar || ui->draggingMusic));
        DrawSmoothSliderBar(ui, ui->sfxBarRect,   ui->sfxVisualPos,   (ui->hoverSfxBar || ui->draggingSfx));
    }
}

void Settings_Unload(SettingsUI *ui) {
    if (!ui) return;

    UnloadTextureSafe(&ui->bg);
    UnloadTextureSafe(&ui->full);
    UnloadTextureSafe(&ui->closeNormal);
    UnloadTextureSafe(&ui->closeHover);

    for (int i = 0; i < SETTINGS_SLIDER_STEPS; i++) {
        UnloadTextureSafe(&ui->barSteps[i]);
    }
}

float Settings_GetMusicVolume(const SettingsUI *ui) {
    if (!ui) return 1.0f;
    return (float)ClampLevel(ui->musicLevel) / (float)(SETTINGS_SLIDER_STEPS - 1);
}

float Settings_GetSfxVolume(const SettingsUI *ui) {
    if (!ui) return 1.0f;
    return (float)ClampLevel(ui->sfxLevel) / (float)(SETTINGS_SLIDER_STEPS - 1);
}