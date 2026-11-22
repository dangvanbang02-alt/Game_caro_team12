#include "Play.h"
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

static void PlayMenuHoverOnce(MenuUI *menu) {
    if (!menu || !menu->audioLoaded) return;
    StopSound(menu->hoverSfx);
    PlaySound(menu->hoverSfx);
}

static void PlayModeClickOnce(PlayUI *ui, MenuUI *menu) {
    if (menu && menu->audioLoaded) {
        StopSound(menu->hoverSfx);
    }
    if (ui && ui->modeClickLoaded) {
        StopSound(ui->modeClickSfx);
        PlaySound(ui->modeClickSfx);
    }
}

static void UpdatePlayMusicTransition(PlayUI *ui, MenuUI *menu, float dt) {
    if (!ui) return;

    if (ui->bgmFade < 1.0f) {
        ui->bgmFade += dt * 1.6f;
        if (ui->bgmFade > 1.0f) ui->bgmFade = 1.0f;
    }

    if (menu && menu->bgmLoaded) {
        if (ui->bgmFade < 1.0f) {
            float menuVol = ui->masterMusicVol * (1.0f - ui->bgmFade);
            SetMusicVolume(menu->bgm, menuVol);
            UpdateMusicStream(menu->bgm);
        } else if (IsMusicStreamPlaying(menu->bgm)) {
            StopMusicStream(menu->bgm);
        }
    }

    if (ui->bgmLoaded) {
        SetMusicVolume(ui->bgm, ui->masterMusicVol * ui->bgmFade);
        UpdateMusicStream(ui->bgm);
        float totalLen = GetMusicTimeLength(ui->bgm);
        float played   = GetMusicTimePlayed(ui->bgm);
        if (!IsMusicStreamPlaying(ui->bgm) ||
            (totalLen > 0.0f && played >= totalLen - 0.05f)) {
            StopMusicStream(ui->bgm);
            SeekMusicStream(ui->bgm, 0.0f);
            PlayMusicStream(ui->bgm);
        }
    }
}

void Play_SetMusicVolume(PlayUI *ui, float volume) {
    if (!ui) return;
    ui->masterMusicVol = Clamp01(volume);
    if (ui->bgmLoaded) {
        SetMusicVolume(ui->bgm, ui->masterMusicVol * ui->bgmFade);
    }
}

void Play_SetSfxVolume(PlayUI *ui, float volume) {
    if (!ui) return;
    ui->masterSfxVol = Clamp01(volume);
    if (ui->modeClickLoaded) {
        SetSoundVolume(ui->modeClickSfx, ui->masterSfxVol);
    }
}

void Play_StopMusic(PlayUI *ui) {
    if (!ui) return;
    if (ui->bgmLoaded && IsMusicStreamPlaying(ui->bgm)) {
        StopMusicStream(ui->bgm);
    }
    ui->bgmFade = 0.0f;
}

void Play_Resize(PlayUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;
    float sw = (float)screenWidth;
    float sh = (float)screenHeight;

    Texture2D refTex = (ui->full.id > 0) ? ui->full : ui->bg;
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

    float cardW = p.width * 0.375f;
    float cardH = p.height * 0.315f;
    float cardY = p.y + p.height * 0.515f;

    ui->pvpCardRect = (Rectangle){
        p.x + p.width * 0.105f,
        cardY,
        cardW,
        cardH
    };

    ui->pveCardRect = (Rectangle){
        p.x + p.width * 0.520f,
        cardY,
        cardW,
        cardH
    };
}

void Play_Open(PlayUI *ui) {
    if (!ui) return;
    ui->openAnim          = 0.0f;
    ui->pvpHoverAnim      = 0.0f;
    ui->pveHoverAnim      = 0.0f;
    ui->selectedModeIndex = -1;
    ui->hoverPvpCard      = false;
    ui->hoverPveCard      = false;
    ui->pressPvpCard      = false;
    ui->pressPveCard      = false;
    ui->chosenMode        = PLAY_MODE_NONE;
    ui->bgmFade           = 0.0f;

    if (ui->bgmLoaded) {
        StopMusicStream(ui->bgm);
        SeekMusicStream(ui->bgm, 0.0f);
        PlayMusicStream(ui->bgm);
    }
}

void Play_Init(PlayUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;
    memset(ui, 0, sizeof(PlayUI));

    ui->masterMusicVol    = 1.0f;
    ui->masterSfxVol      = 1.0f;
    ui->selectedModeIndex = -1;

    ui->bg        = LoadTextureSafe(PLAY_IMG_BG);
    ui->full      = LoadTextureSafe(PLAY_IMG_FULL);
    ui->pvpNormal = LoadTextureSafe(PLAY_IMG_PVP_NORMAL);
    ui->pvpHover  = LoadTextureSafe(PLAY_IMG_PVP_HOVER);
    ui->pveNormal = LoadTextureSafe(PLAY_IMG_PVE_NORMAL);
    ui->pveHover  = LoadTextureSafe(PLAY_IMG_PVE_HOVER);

    if (IsAudioDeviceReady()) {
        if (FileExists(PLAY_BGM_PATH)) {
            ui->bgm = LoadMusicStream(PLAY_BGM_PATH);
            ui->bgm.looping = true;
            ui->bgmLoaded = true;
        }
        if (FileExists(PLAY_SFX_MODE_CLICK)) {
            ui->modeClickSfx = LoadSound(PLAY_SFX_MODE_CLICK);
            ui->modeClickLoaded = true;
        }
    }

    Play_Resize(ui, screenWidth, screenHeight);
}

PlayAction Play_Update(PlayUI *ui, MenuUI *menu, float dt) {
    if (!ui) return PLAY_ACTION_NONE;

    UpdatePlayMusicTransition(ui, menu, dt);

    if (ui->openAnim < 1.0f) {
        ui->openAnim += dt * 3.0f;
        if (ui->openAnim > 1.0f) ui->openAnim = 1.0f;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE)) {
            ui->openAnim = 1.0f;
        } else {
            return PLAY_ACTION_NONE;
        }
    }

    Vector2 mouse = GetMousePosition();
    bool prevHoverPvp = ui->hoverPvpCard;
    bool prevHoverPve = ui->hoverPveCard;
    int  prevSelected = ui->selectedModeIndex;

    ui->hoverPvpCard = CheckCollisionPointRec(mouse, ui->pvpCardRect);
    ui->hoverPveCard = CheckCollisionPointRec(mouse, ui->pveCardRect);
    ui->pressPvpCard = ui->hoverPvpCard && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    ui->pressPveCard = ui->hoverPveCard && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    if (ui->hoverPvpCard) {
        ui->selectedModeIndex = 0;
    } else if (ui->hoverPveCard) {
        ui->selectedModeIndex = 1;
    } else if (prevHoverPvp || prevHoverPve) {
        ui->selectedModeIndex = -1;
    }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        ui->selectedModeIndex = 0;
    } else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        ui->selectedModeIndex = 1;
    }

    if ((ui->hoverPvpCard && !prevHoverPvp) ||
        (ui->hoverPveCard && !prevHoverPve) ||
        (ui->selectedModeIndex != prevSelected && ui->selectedModeIndex >= 0)) {
        PlayMenuHoverOnce(menu);
    }

    float pvpTarget = (ui->hoverPvpCard || ui->selectedModeIndex == 0) ? 1.0f : 0.0f;
    float pveTarget = (ui->hoverPveCard || ui->selectedModeIndex == 1) ? 1.0f : 0.0f;
    ui->pvpHoverAnim += (pvpTarget - ui->pvpHoverAnim) * Clamp01(dt * 16.0f);
    ui->pveHoverAnim += (pveTarget - ui->pveHoverAnim) * Clamp01(dt * 16.0f);

    bool anyHover = (ui->hoverPvpCard || ui->hoverPveCard);
    SetMouseCursor(anyHover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

    if (IsKeyPressed(KEY_ESCAPE)) {
        PlayModeClickOnce(ui, menu);
        Play_StopMusic(ui);
        if (menu) {
            Menu_StartMusic(menu);
        }
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        return PLAY_ACTION_BACK;
    }

    bool choosePvp = (ui->hoverPvpCard && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) ||
                     (ui->selectedModeIndex == 0 && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)));
    if (choosePvp) {
        PlayModeClickOnce(ui, menu);
        ui->chosenMode = PLAY_MODE_PVP;
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        return PLAY_ACTION_SELECT_PVP;
    }

    bool choosePve = (ui->hoverPveCard && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) ||
                     (ui->selectedModeIndex == 1 && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)));
    if (choosePve) {
        PlayModeClickOnce(ui, menu);
        ui->chosenMode = PLAY_MODE_PVE;
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        return PLAY_ACTION_SELECT_PVE;
    }

    return PLAY_ACTION_NONE;
}

static void DrawModeCard(Texture2D normalTex, Texture2D hoverTex, Rectangle baseRect, float anim, bool pressed) {
    float scale = 1.0f + 0.03f * anim;
    if (pressed) scale = 0.98f;

    Rectangle r = {
        baseRect.x + baseRect.width * (1.0f - scale) * 0.5f,
        baseRect.y + baseRect.height * (1.0f - scale) * 0.5f,
        baseRect.width * scale,
        baseRect.height * scale
    };

    if (normalTex.id > 0) {
        DrawTextureFit(normalTex, r, WHITE);
    }
    if (hoverTex.id > 0 && anim > 0.01f) {
        DrawTextureFit(hoverTex, r, Fade(WHITE, anim));
    }
}

void Play_Draw(const PlayUI *ui) {
    if (!ui) return;

    float t = ui->openAnim;

    float bgAlpha   = Clamp01(t / 0.45f);
    float scale     = 0.93f + 0.07f * (1.0f - powf(1.0f - bgAlpha, 3.0f));
    float fullAlpha = Clamp01((t - 0.35f) / 0.65f);

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.74f * bgAlpha));

    Rectangle animRect = {
        ui->panelRect.x + ui->panelRect.width * (1.0f - scale) * 0.5f,
        ui->panelRect.y + ui->panelRect.height * (1.0f - scale) * 0.5f,
        ui->panelRect.width * scale,
        ui->panelRect.height * scale
    };

    if (ui->bg.id > 0) {
        DrawTextureFit(ui->bg, animRect, Fade(WHITE, bgAlpha));
    } else {
        DrawRectangleRec(animRect, Fade((Color){ 18, 22, 28, 245 }, bgAlpha));
    }

    if (ui->full.id > 0 && fullAlpha > 0.0f) {
        DrawTextureFit(ui->full, animRect, Fade(WHITE, fullAlpha));
    }

    if (t >= 0.95f) {
        DrawModeCard(ui->pvpNormal, ui->pvpHover, ui->pvpCardRect, ui->pvpHoverAnim, ui->pressPvpCard);
        DrawModeCard(ui->pveNormal, ui->pveHover, ui->pveCardRect, ui->pveHoverAnim, ui->pressPveCard);
    }
}

void Play_Unload(PlayUI *ui) {
    if (!ui) return;

    UnloadTextureSafe(&ui->bg);
    UnloadTextureSafe(&ui->full);
    UnloadTextureSafe(&ui->pvpNormal);
    UnloadTextureSafe(&ui->pvpHover);
    UnloadTextureSafe(&ui->pveNormal);
    UnloadTextureSafe(&ui->pveHover);

    if (ui->modeClickLoaded) {
        StopSound(ui->modeClickSfx);
        UnloadSound(ui->modeClickSfx);
        ui->modeClickLoaded = false;
    }

    if (ui->bgmLoaded) {
        StopMusicStream(ui->bgm);
        UnloadMusicStream(ui->bgm);
        ui->bgmLoaded = false;
    }
}

PlayGameMode Play_GetChosenMode(const PlayUI *ui) {
    if (!ui) return PLAY_MODE_NONE;
    return ui->chosenMode;
}