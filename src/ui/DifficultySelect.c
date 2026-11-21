#include "DifficultySelect.h"
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

static void PlayHoverSfx(const DifficultySelectUI *ui) {
    if (ui && ui->hasHoverSound && ui->hoverSound.frameCount > 0) {
        PlaySound(ui->hoverSound);
    }
}

static void PlayClickSfx(const DifficultySelectUI *ui) {
    if (ui && ui->hasClickSound && ui->clickSound.frameCount > 0) {
        PlaySound(ui->clickSound);
    }
}

static void DrawCornerMarks(Rectangle r, float size, Color bgColor) {
    // 4 góc cắt vát giống khung ngoài trong ảnh tham chiếu
    Vector2 tl0 = { r.x, r.y + size };
    Vector2 tl1 = { r.x + size, r.y };
    DrawTriangle(tl0, (Vector2){ r.x, r.y }, tl1, bgColor);

    Vector2 tr0 = { r.x + r.width - size, r.y };
    Vector2 tr1 = { r.x + r.width, r.y + size };
    DrawTriangle(tr0, (Vector2){ r.x + r.width, r.y }, tr1, bgColor);

    Vector2 bl0 = { r.x, r.y + r.height - size };
    Vector2 bl1 = { r.x + size, r.y + r.height };
    DrawTriangle(bl0, (Vector2){ r.x, r.y + r.height }, bl1, bgColor);

    Vector2 br0 = { r.x + r.width - size, r.y + r.height };
    Vector2 br1 = { r.x + r.width, r.y + r.height - size };
    DrawTriangle(br0, (Vector2){ r.x + r.width, r.y + r.height }, br1, bgColor);
}

static void DrawFallbackCardArt(Rectangle dst, const DifficultyCard *card, float hover) {
    DrawRectangleRec(dst, DIFF_COLOR_PANEL);

    // Dải màu dọc bên trong tạo chiều sâu khi chưa nạp ảnh .png
    Rectangle stripe = {
        dst.x,
        dst.y,
        dst.width,
        dst.height * (0.45f + 0.06f * hover)
    };
    DrawRectangleRec(stripe, Fade(card->accentColor, 0.25f + 0.15f * hover));

    float cx = dst.x + dst.width * 0.5f;
    float cy = dst.y + dst.height * 0.46f;
    float r = fminf(dst.width, dst.height) * 0.13f;

    DrawPoly((Vector2){ cx, cy }, 6, r, 0.0f, DIFF_COLOR_PANEL_LITE);
    DrawPolyLinesEx((Vector2){ cx, cy }, 6, r, 0.0f, 2.5f, card->accentColor);
    DrawCircleV((Vector2){ cx, cy }, r * 0.35f, DIFF_COLOR_CREAM);
}

void DifficultySelect_Init(DifficultySelectUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;

    memset(ui, 0, sizeof(*ui));

    const char *labels[DIFFICULTY_COUNT]    = { "EASY", "MEDIUM", "HARD" };
    const char *subtitles[DIFFICULTY_COUNT] = {
        "Casual AI & Relaxed Pace",
        "Balanced Tactical Bot",
        "Aggressive High-Depth AI"
    };
    const char *paths[DIFFICULTY_COUNT] = {
        DIFFICULTY_IMG_EASY,
        DIFFICULTY_IMG_MEDIUM,
        DIFFICULTY_IMG_HARD
    };
    Color accents[DIFFICULTY_COUNT] = {
        DIFF_COLOR_EASY,
        DIFF_COLOR_MEDIUM,
        DIFF_COLOR_HARD
    };

    for (int i = 0; i < DIFFICULTY_COUNT; i++) {
        ui->cards[i].level = (DifficultyLevel)i;
        ui->cards[i].label = labels[i];
        ui->cards[i].subtitle = subtitles[i];
        ui->cards[i].accentColor = accents[i];
        ui->cards[i].tex = LoadTexture(paths[i]);
        ui->cards[i].cardRect = (Rectangle){ 0.0f, 0.0f, 0.0f, 0.0f };
        ui->cards[i].hoverAnim = 0.0f;
    }

    ui->focusedIndex = 1; // Mặc định focus vào MEDIUM
    ui->selectedLevel = DIFFICULTY_NONE;

    DifficultySelect_Resize(ui, screenWidth, screenHeight);
}

void DifficultySelect_Resize(DifficultySelectUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;

    float sw = (float)((screenWidth > 0) ? screenWidth : 1280);
    float sh = (float)((screenHeight > 0) ? screenHeight : 720);

    // Cụm 3 cột dọc sát nhau ở giữa màn hình giống image_00408d.png
    float groupW = fminf(sw * 0.56f, sh * 1.02f);
    float groupH = sh * 0.78f;
    if (groupW < 300.0f) groupW = sw * 0.85f;
    if (groupH < 260.0f) groupH = sh * 0.72f;

    float groupX = (sw - groupW) * 0.5f;
    float groupY = (sh - groupH) * 0.5f;
    ui->groupRect = (Rectangle){ groupX, groupY, groupW, groupH };

    float colW = groupW / (float)DIFFICULTY_COUNT;
    for (int i = 0; i < DIFFICULTY_COUNT; i++) {
        ui->cards[i].cardRect = (Rectangle){
            groupX + i * colW,
            groupY,
            colW,
            groupH
        };
    }
}

void DifficultySelect_SetAudio(DifficultySelectUI *ui, Sound hoverSound, Sound clickSound) {
    if (!ui) return;
    ui->hoverSound = hoverSound;
    ui->clickSound = clickSound;
    ui->hasHoverSound = (hoverSound.frameCount > 0);
    ui->hasClickSound = (clickSound.frameCount > 0);
}

void DifficultySelect_Reset(DifficultySelectUI *ui) {
    if (!ui) return;
    ui->focusedIndex = 1;
    ui->selectedLevel = DIFFICULTY_NONE;
    for (int i = 0; i < DIFFICULTY_COUNT; i++) {
        ui->cards[i].hoverAnim = 0.0f;
    }
}

DifficultySelectResult DifficultySelect_Update(DifficultySelectUI *ui, float dt) {
    if (!ui) return DIFFICULTY_RESULT_NONE;
    if (dt <= 0.0f) dt = 0.016f;

    if (IsKeyPressed(KEY_ESCAPE)) {
        PlayClickSfx(ui);
        return DIFFICULTY_RESULT_BACK;
    }

    Vector2 mouse = GetMousePosition();
    int hovered = -1;
    for (int i = 0; i < DIFFICULTY_COUNT; i++) {
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
        ui->focusedIndex = (ui->focusedIndex - 1 + DIFFICULTY_COUNT) % DIFFICULTY_COUNT;
        PlayHoverSfx(ui);
    } else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        ui->focusedIndex = (ui->focusedIndex + 1) % DIFFICULTY_COUNT;
        PlayHoverSfx(ui);
    }

    for (int i = 0; i < DIFFICULTY_COUNT; i++) {
        bool active = (i == hovered) || (i == ui->focusedIndex);
        float target = active ? 1.0f : 0.0f;
        ui->cards[i].hoverAnim = Clamp01(
            LerpFloat(ui->cards[i].hoverAnim, target, Clamp01(14.0f * dt))
        );
    }

    if (hovered != -1 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        ui->selectedLevel = ui->cards[hovered].level;
        PlayClickSfx(ui);
        return DIFFICULTY_RESULT_SELECTED;
    }

    if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) &&
        ui->focusedIndex >= 0 && ui->focusedIndex < DIFFICULTY_COUNT) {
        ui->selectedLevel = ui->cards[ui->focusedIndex].level;
        PlayClickSfx(ui);
        return DIFFICULTY_RESULT_SELECTED;
    }

    return DIFFICULTY_RESULT_NONE;
}

void DifficultySelect_Draw(const DifficultySelectUI *ui) {
    if (!ui) return;

    ClearBackground(DIFF_COLOR_BG);

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    // Tiêu đề phía trên cụm cột
    const char *header = "SELECT DIFFICULTY";
    int headSize = 26;
    int headW = MeasureText(header, headSize);
    DrawText(
        header,
        (int)((sw - headW) * 0.5f),
        (int)fmaxf(14.0f, ui->groupRect.y - 44.0f),
        headSize,
        DIFF_COLOR_CREAM
    );

    for (int i = 0; i < DIFFICULTY_COUNT; i++) {
        const DifficultyCard *card = &ui->cards[i];
        float hAnim = card->hoverAnim;

        Rectangle r = card->cardRect;
        float liftY = -10.0f * hAnim;
        Rectangle dst = { r.x, r.y + liftY, r.width, r.height };

        if (card->tex.id > 0) {
            float zoomRatio = 1.0f - 0.07f * hAnim;
            float srcW = (float)card->tex.width * zoomRatio;
            float srcH = (float)card->tex.height * zoomRatio;
            float srcX = ((float)card->tex.width - srcW) * 0.5f;
            float srcY = ((float)card->tex.height - srcH) * 0.5f;

            Rectangle src = { srcX, srcY, srcW, srcH };
            DrawTexturePro(card->tex, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
        } else {
            DrawFallbackCardArt(dst, card, hAnim);
        }

        // Làm sáng nhẹ khi hover, làm dịu nhẹ những cột chưa focus
        if (hAnim > 0.02f) {
            DrawRectangleRec(dst, Fade(DIFF_COLOR_CREAM, 0.09f * hAnim));
        } else {
            DrawRectangleRec(dst, Fade(DIFF_COLOR_BG, 0.22f));
        }

        // Nhãn EASY / MEDIUM / HARD nằm giữa cột giống ảnh mẫu
        int lblSize = (int)(26 + 4 * hAnim);
        int lblW = MeasureText(card->label, lblSize);
        float lblY = dst.y + dst.height * 0.56f;

        DrawText(
            card->label,
            (int)(dst.x + (dst.width - lblW) * 0.5f + 2.0f),
            (int)(lblY + 2.0f),
            lblSize,
            Fade(BLACK, 0.75f)
        );
        DrawText(
            card->label,
            (int)(dst.x + (dst.width - lblW) * 0.5f),
            (int)lblY,
            lblSize,
            (hAnim > 0.25f) ? DIFF_COLOR_CREAM : Fade(DIFF_COLOR_CREAM, 0.90f)
        );

        // Mô tả ngắn hiện lên khi hover
        if (hAnim > 0.12f) {
            int subSize = 14;
            int subW = MeasureText(card->subtitle, subSize);
            DrawText(
                card->subtitle,
                (int)(dst.x + (dst.width - subW) * 0.5f),
                (int)(lblY + lblSize + 10.0f),
                subSize,
                Fade(DIFF_COLOR_CREAM, hAnim)
            );
        }

        // Thanh nhấn phía trên & viền khi hover
        Color activeBorder = (hAnim > 0.2f) ? DIFF_COLOR_RED : Fade(DIFF_COLOR_BORDER, 0.55f);
        if (hAnim > 0.2f) {
            DrawRectangle((int)dst.x, (int)dst.y, (int)dst.width, 4, DIFF_COLOR_RED);
        }
        DrawRectangleLinesEx(dst, 1.0f + 1.5f * hAnim, activeBorder);
    }

    DrawCornerMarks((Rectangle){ 8.0f, 8.0f, sw - 16.0f, sh - 16.0f }, fminf(sw, sh) * 0.045f, DIFF_COLOR_BG);

    const char *hint = "Click hoac Phim Trai/Phai + Enter de chon do kho  |  ESC: Quay lai";
    int hintSize = 15;
    int hintW = MeasureText(hint, hintSize);
    DrawText(
        hint,
        (int)((sw - hintW) * 0.5f),
        (int)(sh - 28.0f),
        hintSize,
        DIFF_COLOR_MUTED
    );
}

DifficultyLevel DifficultySelect_GetSelected(const DifficultySelectUI *ui) {
    return ui ? ui->selectedLevel : DIFFICULTY_NONE;
}

void DifficultySelect_Unload(DifficultySelectUI *ui) {
    if (!ui) return;

    for (int i = 0; i < DIFFICULTY_COUNT; i++) {
        if (ui->cards[i].tex.id > 0) {
            UnloadTexture(ui->cards[i].tex);
            ui->cards[i].tex.id = 0;
        }
    }
}