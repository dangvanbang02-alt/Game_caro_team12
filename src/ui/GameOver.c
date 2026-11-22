#include "GameOver.h"
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

static void PlayGameOverClickOnce(MenuUI *menu) {
    if (!menu || !menu->audioLoaded) return;
    StopSound(menu->hoverSfx);
    StopSound(menu->clickSfx);
    PlaySound(menu->clickSfx);
}

static void UpdateLoopingMusic(Music *music, bool loaded, float volume) {
    if (!loaded || !music) return;
    SetMusicVolume(*music, volume);
    UpdateMusicStream(*music);
    float totalLen = GetMusicTimeLength(*music);
    float played   = GetMusicTimePlayed(*music);
    if (!IsMusicStreamPlaying(*music) ||
        (totalLen > 0.0f && played >= totalLen - 0.05f)) {
        StopMusicStream(*music);
        SeekMusicStream(*music, 0.0f);
        PlayMusicStream(*music);
    }
}

void GameOver_SetMusicVolume(GameOverUI *ui, float volume) {
    if (!ui) return;
    ui->musicVolume = Clamp01(volume);
    if (ui->winBgmLoaded) {
        SetMusicVolume(ui->winBgm, ui->musicVolume);
    }
    if (ui->loseBgmLoaded) {
        SetMusicVolume(ui->loseBgm, ui->musicVolume);
    }
    if (ui->drawBgmLoaded) {
        SetMusicVolume(ui->drawBgm, ui->musicVolume);
    }
}

void GameOver_StopMusic(GameOverUI *ui) {
    if (!ui) return;
    if (ui->winBgmLoaded && IsMusicStreamPlaying(ui->winBgm)) {
        StopMusicStream(ui->winBgm);
    }
    if (ui->loseBgmLoaded && IsMusicStreamPlaying(ui->loseBgm)) {
        StopMusicStream(ui->loseBgm);
    }
    if (ui->drawBgmLoaded && IsMusicStreamPlaying(ui->drawBgm)) {
        StopMusicStream(ui->drawBgm);
    }
}

void GameOver_Resize(GameOverUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;
    float sw = (float)screenWidth;
    float sh = (float)screenHeight;

    Texture2D refTex = (ui->alphaFull.id > 0) ? ui->alphaFull
                     : ((ui->omegaFull.id > 0) ? ui->omegaFull : ui->drawImg);
    float panelW = fminf(860.0f, sw * 0.72f);
    float aspect = (refTex.id > 0 && refTex.width > 0)
                   ? ((float)refTex.height / (float)refTex.width)
                   : 0.52f;
    float panelH = panelW * aspect;

    ui->panelRect = (Rectangle){
        (sw - panelW) * 0.5f,
        (sh - panelH) * 0.5f,
        panelW,
        panelH
    };
}

void GameOver_Open(GameOverUI *ui, const GameHUD *hud, bool isPveMode, int pvePlayerPiece) {
    if (!ui) return;

    ui->openAnim   = 0.0f;
    ui->pulseTimer = 0.0f;

    if (hud) {
        ui->winnerPiece = HUD_GetChampionPiece(hud);
        ui->alphaScore  = hud->alphaButtons;
        ui->omegaScore  = hud->omegaButtons;

        if (ui->winnerPiece == 3) {
            snprintf(ui->winnerName, sizeof(ui->winnerName), "ALPHA & OMEGA");
        } else if (ui->winnerPiece == 2) {
            snprintf(ui->winnerName, sizeof(ui->winnerName), "%s",
                     (hud->p2.name[0] != '\0') ? hud->p2.name : "OMEGA");
        } else {
            ui->winnerPiece = 1;
            snprintf(ui->winnerName, sizeof(ui->winnerName), "%s",
                     (hud->p1.name[0] != '\0') ? hud->p1.name : "ALPHA");
        }
    } else {
        ui->winnerPiece = 1;
        ui->alphaScore  = 5;
        ui->omegaScore  = 0;
        snprintf(ui->winnerName, sizeof(ui->winnerName), "ALPHA");
    }

    if (ui->winnerPiece == 3) {
        ui->activeMusic = GAMEOVER_MUSIC_DRAW;
    } else if (isPveMode) {
        ui->activeMusic = (ui->winnerPiece == pvePlayerPiece) ? GAMEOVER_MUSIC_WIN : GAMEOVER_MUSIC_LOSE;
    } else {
        ui->activeMusic = GAMEOVER_MUSIC_WIN;
    }

    GameOver_StopMusic(ui);

    if (ui->activeMusic == GAMEOVER_MUSIC_DRAW) {
        if (ui->drawBgmLoaded) {
            SeekMusicStream(ui->drawBgm, 0.0f);
            SetMusicVolume(ui->drawBgm, ui->musicVolume);
            PlayMusicStream(ui->drawBgm);
        }
    } else if (ui->activeMusic == GAMEOVER_MUSIC_WIN) {
        if (ui->winBgmLoaded) {
            SeekMusicStream(ui->winBgm, 0.0f);
            SetMusicVolume(ui->winBgm, ui->musicVolume);
            PlayMusicStream(ui->winBgm);
        }
    } else {
        if (ui->loseBgmLoaded) {
            SeekMusicStream(ui->loseBgm, 0.0f);
            SetMusicVolume(ui->loseBgm, ui->musicVolume);
            PlayMusicStream(ui->loseBgm);
        }
    }
}

void GameOver_Init(GameOverUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;
    memset(ui, 0, sizeof(GameOverUI));

    ui->musicVolume = 1.0f;
    ui->activeMusic = GAMEOVER_MUSIC_WIN;

    ui->alphaBg   = LoadTextureSafe(GAMEOVER_IMG_ALPHA_BG);
    ui->alphaFull = LoadTextureSafe(GAMEOVER_IMG_ALPHA_FULL);
    ui->omegaBg   = LoadTextureSafe(GAMEOVER_IMG_OMEGA_BG);
    ui->omegaFull = LoadTextureSafe(GAMEOVER_IMG_OMEGA_FULL);
    ui->drawImg   = LoadTextureSafe(GAMEOVER_IMG_DRAW);

    if (IsAudioDeviceReady()) {
        if (FileExists(GAMEOVER_BGM_WIN)) {
            ui->winBgm = LoadMusicStream(GAMEOVER_BGM_WIN);
            ui->winBgm.looping = true;
            ui->winBgmLoaded = true;
        }
        if (FileExists(GAMEOVER_BGM_LOSE)) {
            ui->loseBgm = LoadMusicStream(GAMEOVER_BGM_LOSE);
            ui->loseBgm.looping = true;
            ui->loseBgmLoaded = true;
        }
        if (FileExists(GAMEOVER_BGM_DRAW)) {
            ui->drawBgm = LoadMusicStream(GAMEOVER_BGM_DRAW);
            ui->drawBgm.looping = true;
            ui->drawBgmLoaded = true;
        }
    }

    ui->winnerPiece = 1;
    snprintf(ui->winnerName, sizeof(ui->winnerName), "ALPHA");

    GameOver_Resize(ui, screenWidth, screenHeight);
}

GameOverAction GameOver_Update(GameOverUI *ui, MenuUI *menu, float dt) {
    if (!ui) return GAMEOVER_ACTION_NONE;

    if (ui->activeMusic == GAMEOVER_MUSIC_DRAW) {
        UpdateLoopingMusic(&ui->drawBgm, ui->drawBgmLoaded, ui->musicVolume);
    } else if (ui->activeMusic == GAMEOVER_MUSIC_WIN) {
        UpdateLoopingMusic(&ui->winBgm, ui->winBgmLoaded, ui->musicVolume);
    } else {
        UpdateLoopingMusic(&ui->loseBgm, ui->loseBgmLoaded, ui->musicVolume);
    }

    ui->pulseTimer += dt;

    if (ui->openAnim < 1.0f) {
        ui->openAnim += dt * 2.2f;
        if (ui->openAnim > 1.0f) ui->openAnim = 1.0f;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE)) {
            ui->openAnim = 1.0f;
        }
        return GAMEOVER_ACTION_NONE;
    }

    Vector2 mouse = GetMousePosition();
    bool overPanel = CheckCollisionPointRec(mouse, ui->panelRect);
    SetMouseCursor(overPanel ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

    bool exitRequested = (overPanel && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) ||
                         IsKeyPressed(KEY_ESCAPE) ||
                         IsKeyPressed(KEY_ENTER) ||
                         IsKeyPressed(KEY_KP_ENTER);

    if (exitRequested) {
        GameOver_StopMusic(ui);
        PlayGameOverClickOnce(menu);
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        return GAMEOVER_ACTION_BACK_TO_MENU;
    }

    return GAMEOVER_ACTION_NONE;
}

void GameOver_Draw(const GameOverUI *ui) {
    if (!ui) return;

    float t         = ui->openAnim;
    float bgAlpha   = Clamp01(t / 0.45f);
    float scale     = 0.94f + 0.06f * (1.0f - powf(1.0f - bgAlpha, 3.0f));
    float fullAlpha = Clamp01((t - 0.35f) / 0.65f);

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.80f * bgAlpha));

    Rectangle animRect = {
        ui->panelRect.x + ui->panelRect.width * (1.0f - scale) * 0.5f,
        ui->panelRect.y + ui->panelRect.height * (1.0f - scale) * 0.5f,
        ui->panelRect.width * scale,
        ui->panelRect.height * scale
    };

    Color accent = GAMEOVER_ACCENT_MINT;

    if (ui->winnerPiece == 3) {
        accent = GAMEOVER_ACCENT_GOLD;
        if (ui->drawImg.id > 0) {
            DrawTextureFit(ui->drawImg, animRect, Fade(WHITE, bgAlpha));
        } else {
            DrawRectangleRec(animRect, Fade((Color){ 16, 20, 27, 245 }, bgAlpha));
        }
    } else {
        bool isAlpha      = (ui->winnerPiece != 2);
        Texture2D bgTex   = isAlpha ? ui->alphaBg   : ui->omegaBg;
        Texture2D fullTex = isAlpha ? ui->alphaFull : ui->omegaFull;
        accent            = isAlpha ? GAMEOVER_ACCENT_MINT : GAMEOVER_ACCENT_RED;

        if (bgTex.id > 0) {
            DrawTextureFit(bgTex, animRect, Fade(WHITE, bgAlpha));
        } else {
            DrawRectangleRec(animRect, Fade((Color){ 16, 20, 27, 245 }, bgAlpha));
        }

        if (fullTex.id > 0 && fullAlpha > 0.0f) {
            DrawTextureFit(fullTex, animRect, Fade(WHITE, fullAlpha));
        }
    }

    if (t >= 0.95f) {
        Rectangle p = ui->panelRect;

        int nameFontSize = (int)(p.height * 0.052f);
        if (nameFontSize < 16) nameFontSize = 16;

        Rectangle nameMask = {
            p.x + p.width * 0.34f,
            p.y + p.height * 0.34f,
            p.width * 0.32f,
            p.height * 0.09f
        };
        DrawRectangleRec(nameMask, Fade((Color){ 14, 18, 22, 235 }, 0.92f));

        int nameW = MeasureText(ui->winnerName, nameFontSize);
        DrawText(ui->winnerName,
                 (int)(p.x + (p.width - nameW) * 0.5f),
                 (int)(nameMask.y + (nameMask.height - nameFontSize) * 0.5f),
                 nameFontSize,
                 accent);

        int scoreFontSize = (int)(p.height * 0.068f);
        if (scoreFontSize < 20) scoreFontSize = 20;

        char leftScoreBuf[8];
        char rightScoreBuf[8];
        snprintf(leftScoreBuf, sizeof(leftScoreBuf), "%d", ui->alphaScore);
        snprintf(rightScoreBuf, sizeof(rightScoreBuf), "%d", ui->omegaScore);

        Rectangle leftMask = {
            p.x + p.width * 0.265f,
            p.y + p.height * 0.455f,
            p.width * 0.075f,
            p.height * 0.11f
        };
        Rectangle rightMask = {
            p.x + p.width * 0.660f,
            p.y + p.height * 0.455f,
            p.width * 0.075f,
            p.height * 0.11f
        };
        DrawRectangleRec(leftMask, Fade((Color){ 14, 18, 22, 235 }, 0.92f));
        DrawRectangleRec(rightMask, Fade((Color){ 14, 18, 22, 235 }, 0.92f));

        int lw = MeasureText(leftScoreBuf, scoreFontSize);
        int rw = MeasureText(rightScoreBuf, scoreFontSize);

        DrawText(leftScoreBuf,
                 (int)(leftMask.x + (leftMask.width - lw) * 0.5f),
                 (int)(leftMask.y + (leftMask.height - scoreFontSize) * 0.5f),
                 scoreFontSize,
                 GAMEOVER_ACCENT_MINT);

        DrawText(rightScoreBuf,
                 (int)(rightMask.x + (rightMask.width - rw) * 0.5f),
                 (int)(rightMask.y + (rightMask.height - scoreFontSize) * 0.5f),
                 scoreFontSize,
                 GAMEOVER_ACCENT_RED);

        float hintAlpha = 0.45f + 0.35f * sinf(ui->pulseTimer * 3.5f);
        const char *hint = "CLICK ANYWHERE OR PRESS ESC TO RETURN TO MENU";
        int hintSize = (int)(p.height * 0.032f);
        if (hintSize < 12) hintSize = 12;
        int hw = MeasureText(hint, hintSize);
        DrawText(hint,
                 (int)(p.x + (p.width - hw) * 0.5f),
                 (int)(p.y + p.height * 0.88f),
                 hintSize,
                 Fade(WHITE, hintAlpha));
    }
}

void GameOver_Unload(GameOverUI *ui) {
    if (!ui) return;
    UnloadTextureSafe(&ui->alphaBg);
    UnloadTextureSafe(&ui->alphaFull);
    UnloadTextureSafe(&ui->omegaBg);
    UnloadTextureSafe(&ui->omegaFull);
    UnloadTextureSafe(&ui->drawImg);

    if (ui->winBgmLoaded) {
        StopMusicStream(ui->winBgm);
        UnloadMusicStream(ui->winBgm);
        ui->winBgmLoaded = false;
    }
    if (ui->loseBgmLoaded) {
        StopMusicStream(ui->loseBgm);
        UnloadMusicStream(ui->loseBgm);
        ui->loseBgmLoaded = false;
    }
    if (ui->drawBgmLoaded) {
        StopMusicStream(ui->drawBgm);
        UnloadMusicStream(ui->drawBgm);
        ui->drawBgmLoaded = false;
    }
}