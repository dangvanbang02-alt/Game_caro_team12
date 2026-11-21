#include "GameMode.h"
#include <math.h>
#include <string.h>

static float Clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static float LerpFloat(float a, float b, float t) {
    return a + (b - a) * t;
}

static void PlayHoverSfx(const GameModeUI *ui) {
    if (ui && ui->hasHoverSound && ui->hoverSound.frameCount > 0) {
        PlaySound(ui->hoverSound);
    }
}

static void PlayClickSfx(const GameModeUI *ui) {
    if (ui && ui->hasClickSound && ui->clickSound.frameCount > 0) {
        PlaySound(ui->clickSound);
    }
}

static void DrawCornerBrackets(Rectangle r, float size, Color color) {
    // 4 góc trang trí giống khung ngoài trong ảnh mẫu
    Vector2 tl0 = { r.x, r.y + size };
    Vector2 tl1 = { r.x + size, r.y };
    DrawTriangle(tl0, (Vector2){ r.x, r.y }, tl1, color);

    Vector2 tr0 = { r.x + r.width - size, r.y };
    Vector2 tr1 = { r.x + r.width, r.y + size };
    DrawTriangle(tr0, (Vector2){ r.x + r.width, r.y }, tr1, color);

    Vector2 bl0 = { r.x, r.y + r.height - size };
    Vector2 bl1 = { r.x + size, r.y + r.height };
    DrawTriangle(bl0, (Vector2){ r.x, r.y + r.height }, bl1, color);

    Vector2 br0 = { r.x + r.width - size, r.y + r.height };
    Vector2 br1 = { r.x + r.width, r.y + r.height - size };
    DrawTriangle(br0, (Vector2){ r.x + r.width, r.y + r.height }, br1, color);
}

static void DrawDefaultModeEmblem(Rectangle r, GameModeType mode, float hover) {
    DrawRectangleRec(r, GM_COLOR_FRAME);

    float cx = r.x + r.width * 0.5f;
    float cy = r.y + r.height * 0.44f;
    float s = fminf(r.width, r.height) * (0.22f + 0.03f * hover);

    if (mode == GAME_MODE_PVP) {
        // Biểu tượng đối kháng chéo cho PVP
        DrawLineEx((Vector2){ cx - s, cy - s }, (Vector2){ cx + s, cy + s }, 5.0f, GM_COLOR_RED);
        DrawLineEx((Vector2){ cx + s, cy - s }, (Vector2){ cx - s, cy + s }, 5.0f, GM_COLOR_CREAM);
    } else {
        // Biểu tượng khiên chiến thuật cho PVE
        DrawPoly((Vector2){ cx, cy }, 6, s, 0.0f, GM_COLOR_GOLD);
        DrawPolyLinesEx((Vector2){ cx, cy }, 6, s * 1.18f, 0.0f, 2.5f, GM_COLOR_CREAM);
    }

    const char *label = (mode == GAME_MODE_PVP) ? "PVP" : "PVE";
    int fontSize = 26;
    int textW = MeasureText(label, fontSize);
    DrawText(
        label,
        (int)(r.x + 16.0f),
        (int)(r.y + r.height - fontSize - 14.0f),
        fontSize,
        GM_COLOR_CREAM
    );
    (void)textW;
}

void GameMode_Init(GameModeUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;

    memset(ui, 0, sizeof(*ui));

    // Nạp trực tiếp các tệp .png kèm theo
    ui->logoTex = LoadTexture(GAMEMODE_IMG_LOGO);

    ui->cards[0].mode = GAME_MODE_PVP;
    ui->cards[0].title = "PVP";
    ui->cards[0].desc = "2 Players Tactical Duel";
    ui->cards[0].tex = LoadTexture(GAMEMODE_IMG_PVP);

    ui->cards[1].mode = GAME_MODE_PVE;
    ui->cards[1].title = "PVE";
    ui->cards[1].desc = "Play vs AI Bot";
    ui->cards[1].tex = LoadTexture(GAMEMODE_IMG_PVE);

    ui->focusedIndex = 0;
    ui->selectedMode = GAME_MODE_NONE;

    GameMode_Resize(ui, screenWidth, screenHeight);
}

void GameMode_Resize(GameModeUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;

    float sw = (float)((screenWidth > 0) ? screenWidth : 1280);
    float sh = (float)((screenHeight > 0) ? screenHeight : 720);

    float padX = sw * 0.015f;
    float padY = sh * 0.015f;
    ui->outerFrame = (Rectangle){ padX, padY, sw - padX * 2.0f, sh - padY * 2.0f };

    float boxW = ui->outerFrame.width * 0.72f;
    float boxH = ui->outerFrame.height * 0.66f;
    ui->innerBox = (Rectangle){
        ui->outerFrame.x + (ui->outerFrame.width - boxW) * 0.5f,
        ui->outerFrame.y + ui->outerFrame.height * 0.24f,
        boxW,
        boxH
    };

    float logoW = boxW * 0.34f;
    float logoH = boxH * 0.36f;
    ui->logoRect = (Rectangle){
        ui->innerBox.x + (boxW - logoW) * 0.5f,
        ui->innerBox.y + boxH * 0.06f,
        logoW,
        logoH
    };

    float cardGap = boxW * 0.05f;
    float cardSidePad = boxW * 0.07f;
    float cardW = (boxW - cardSidePad * 2.0f - cardGap) * 0.5f;
    float cardH = boxH * 0.44f;
    float cardY = ui->innerBox.y + boxH * 0.48f;

    ui->cards[0].cardRect = (Rectangle){ ui->innerBox.x + cardSidePad, cardY, cardW, cardH };
    ui->cards[1].cardRect = (Rectangle){ ui->innerBox.x + cardSidePad + cardW + cardGap, cardY, cardW, cardH };
}

void GameMode_SetAudio(GameModeUI *ui, Sound hoverSound, Sound clickSound) {
    if (!ui) return;
    ui->hoverSound = hoverSound;
    ui->clickSound = clickSound;
    ui->hasHoverSound = (hoverSound.frameCount > 0);
    ui->hasClickSound = (clickSound.frameCount > 0);
}

void GameMode_Reset(GameModeUI *ui) {
    if (!ui) return;
    ui->focusedIndex = 0;
    ui->selectedMode = GAME_MODE_NONE;
    for (int i = 0; i < GAMEMODE_COUNT; i++) {
        ui->cards[i].hoverAnim = 0.0f;
    }
}

GameModeResult GameMode_Update(GameModeUI *ui, float dt) {
    if (!ui) return GAMEMODE_RESULT_NONE;
    if (dt <= 0.0f) dt = 0.016f;

    if (IsKeyPressed(KEY_ESCAPE)) {
        PlayClickSfx(ui);
        return GAMEMODE_RESULT_BACK;
    }

    Vector2 mouse = GetMousePosition();
    int hovered = -1;
    for (int i = 0; i < GAMEMODE_COUNT; i++) {
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
        ui->focusedIndex = (ui->focusedIndex - 1 + GAMEMODE_COUNT) % GAMEMODE_COUNT;
        PlayHoverSfx(ui);
    } else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        ui->focusedIndex = (ui->focusedIndex + 1) % GAMEMODE_COUNT;
        PlayHoverSfx(ui);
    }

    for (int i = 0; i < GAMEMODE_COUNT; i++) {
        bool isHovered = (i == hovered) || (i == ui->focusedIndex);
        float target = isHovered ? 1.0f : 0.0f;
        ui->cards[i].hoverAnim = Clamp01(
            LerpFloat(ui->cards[i].hoverAnim, target, Clamp01(14.0f * dt))
        );
    }

    if (hovered != -1 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        ui->selectedMode = ui->cards[hovered].mode;
        PlayClickSfx(ui);
        return GAMEMODE_RESULT_SELECTED;
    }

    if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) &&
        ui->focusedIndex >= 0 && ui->focusedIndex < GAMEMODE_COUNT) {
        ui->selectedMode = ui->cards[ui->focusedIndex].mode;
        PlayClickSfx(ui);
        return GAMEMODE_RESULT_SELECTED;
    }

    return GAMEMODE_RESULT_NONE;
}

void GameMode_Draw(const GameModeUI *ui) {
    if (!ui) return;

    ClearBackground(GM_COLOR_BG);

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    // Khung ngoài + 4 góc trang trí
    DrawRectangleRec(ui->outerFrame, GM_COLOR_FRAME);
    DrawRectangleLinesEx(ui->outerFrame, 2.0f, GM_COLOR_BORDER);
    DrawCornerBrackets(ui->outerFrame, fminf(sw, sh) * 0.055f, GM_COLOR_BG);

    // Tiêu đề phía trên
    const char *mainTitle = "CHON CHE DO";
    int mainSize = 26;
    int mainW = MeasureText(mainTitle, mainSize);
    DrawText(
        mainTitle,
        (int)((sw - mainW) * 0.5f),
        (int)(ui->outerFrame.y + ui->outerFrame.height * 0.145f),
        mainSize,
        GM_COLOR_CREAM
    );

    // Hộp trung tâm chứa logo GAME MODE và 2 thẻ PVP / PVE
    DrawRectangleRec(ui->innerBox, GM_COLOR_INNER_BOX);

    if (ui->logoTex.id > 0) {
        Rectangle lSrc = { 0.0f, 0.0f, (float)ui->logoTex.width, (float)ui->logoTex.height };
        float lLift = -3.0f * fmaxf(ui->cards[0].hoverAnim, ui->cards[1].hoverAnim);
        Rectangle lDst = { ui->logoRect.x, ui->logoRect.y + lLift, ui->logoRect.width, ui->logoRect.height };
        DrawTexturePro(ui->logoTex, lSrc, lDst, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    } else {
        const char *gmTxt = "GAME MODE";
        int gmSize = 30;
        int gmW = MeasureText(gmTxt, gmSize);
        DrawText(
            gmTxt,
            (int)(ui->logoRect.x + (ui->logoRect.width - gmW) * 0.5f),
            (int)(ui->logoRect.y + ui->logoRect.height * 0.55f),
            gmSize,
            GM_COLOR_CREAM
        );
    }

    for (int i = 0; i < GAMEMODE_COUNT; i++) {
        const GameModeCard *card = &ui->cards[i];
        float hAnim = card->hoverAnim;

        Rectangle r = card->cardRect;
        float expand = 4.0f * hAnim;
        float liftY = -8.0f * hAnim;

        Rectangle dst = {
            r.x - expand * 0.5f,
            r.y + liftY - expand * 0.5f,
            r.width + expand,
            r.height + expand
        };

        if (card->tex.id > 0) {
            // Hiệu ứng zoom nhẹ ảnh bên trong khi hover
            float zoomRatio = 1.0f - 0.08f * hAnim;
            float srcW = (float)card->tex.width * zoomRatio;
            float srcH = (float)card->tex.height * zoomRatio;
            float srcX = ((float)card->tex.width - srcW) * 0.5f;
            float srcY = ((float)card->tex.height - srcH) * 0.5f;

            Rectangle src = { srcX, srcY, srcW, srcH };
            DrawTexturePro(card->tex, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
        } else {
            DrawDefaultModeEmblem(dst, card->mode, hAnim);
        }

        // Lớp sáng nhẹ + viền đỏ chiến thuật khi hover
        if (hAnim > 0.02f) {
            DrawRectangleRec(dst, Fade(GM_COLOR_CREAM, 0.10f * hAnim));
            DrawRectangle((int)dst.x, (int)dst.y, (int)dst.width, 3, GM_COLOR_RED);
        }

        Color borderCol = (hAnim > 0.2f) ? GM_COLOR_RED : GM_COLOR_CREAM;
        DrawRectangleLinesEx(dst, 1.5f + 1.5f * hAnim, borderCol);

        // Chú thích mô tả khi hover
        if (hAnim > 0.15f) {
            int descSize = 15;
            int descW = MeasureText(card->desc, descSize);
            DrawText(
                card->desc,
                (int)(dst.x + (dst.width - descW) * 0.5f),
                (int)(dst.y + dst.height + 8.0f),
                descSize,
                Fade(GM_COLOR_CREAM, hAnim)
            );
        }
    }

    const char *hint = "Click hoac Phim Trai/Phai + Enter de chon che do  |  ESC: Quay lai";
    int hintSize = 15;
    int hintW = MeasureText(hint, hintSize);
    DrawText(
        hint,
        (int)((sw - hintW) * 0.5f),
        (int)(ui->outerFrame.y + ui->outerFrame.height - 26.0f),
        hintSize,
        GM_COLOR_MUTED
    );
}

GameModeType GameMode_GetSelected(const GameModeUI *ui) {
    return ui ? ui->selectedMode : GAME_MODE_NONE;
}

void GameMode_Unload(GameModeUI *ui) {
    if (!ui) return;

    if (ui->logoTex.id > 0) {
        UnloadTexture(ui->logoTex);
        ui->logoTex.id = 0;
    }

    for (int i = 0; i < GAMEMODE_COUNT; i++) {
        if (ui->cards[i].tex.id > 0) {
            UnloadTexture(ui->cards[i].tex);
            ui->cards[i].tex.id = 0;
        }
    }
}