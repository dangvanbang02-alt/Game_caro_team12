#include "Menu.h"
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

static void PlayMenuClickOnce(MenuUI *menu) {
    if (!menu || !menu->audioLoaded) return;
    StopSound(menu->hoverSfx);
    StopSound(menu->clickSfx);
    PlaySound(menu->clickSfx);
}

void Menu_Resize(MenuUI *menu, int screenWidth, int screenHeight) {
    if (!menu) return;
    float sw = (float)screenWidth;
    float sh = (float)screenHeight;
    float btnW   = sw * 0.235f;
    float btnH   = sh * 0.072f;
    float btnX   = sw * 0.675f;
    float startY = sh * 0.455f;
    float gapY   = sh * 0.105f;

    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        menu->buttons[i].baseBounds = (Rectangle){
            btnX,
            startY + i * gapY,
            btnW,
            btnH
        };
    }

    float cornerSize = fminf(44.0f, sh * 0.055f);
    float margin     = fmaxf(18.0f, sw * 0.018f);
    menu->exitCornerBtn.baseBounds = (Rectangle){
        sw - cornerSize - margin,
        margin,
        cornerSize,
        cornerSize
    };
}

void Menu_ResetIntroAnim(MenuUI *menu) {
    if (!menu) return;
    menu->openAnim = 0.0f;
    menu->selectedIndex = -1;
    menu->exitRequested = false;

    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        menu->buttons[i].hoverAnim = 0.0f;
        menu->buttons[i].isHovered = false;
        menu->buttons[i].isPressed = false;
    }

    menu->exitCornerBtn.hoverAnim = 0.0f;
    menu->exitCornerBtn.isHovered = false;
    menu->exitCornerBtn.isPressed = false;
}

void Menu_Init(MenuUI *menu, int screenWidth, int screenHeight) {
    if (!menu) return;
    memset(menu, 0, sizeof(MenuUI));

    menu->background = LoadTextureSafe(MENU_IMG_BG);
    menu->full       = LoadTextureSafe(MENU_IMG_FULL);

    const char *normalPaths[MENU_BUTTON_COUNT] = {
        MENU_IMG_PLAY_1,
        MENU_IMG_LOAD_1,
        MENU_IMG_SETTINGS_1,
        MENU_IMG_EXIT_1
    };
    const char *activePaths[MENU_BUTTON_COUNT] = {
        MENU_IMG_PLAY_2,
        MENU_IMG_LOAD_2,
        MENU_IMG_SETTINGS_2,
        MENU_IMG_EXIT_2
    };
    const MenuAction actions[MENU_BUTTON_COUNT] = {
        MENU_ACTION_PLAY,
        MENU_ACTION_LOAD,
        MENU_ACTION_SETTINGS,
        MENU_ACTION_EXIT
    };

    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        menu->buttons[i].normal = LoadTextureSafe(normalPaths[i]);
        menu->buttons[i].active = LoadTextureSafe(activePaths[i]);
        menu->buttons[i].action = actions[i];
    }

    menu->exitCornerBtn.normal = LoadTextureSafe(MENU_IMG_CLOSE_1);
    menu->exitCornerBtn.active = LoadTextureSafe(MENU_IMG_CLOSE_2);
    menu->exitCornerBtn.action = MENU_ACTION_EXIT;

    if (IsAudioDeviceReady()) {
        if (FileExists(MENU_SFX_HOVER) && FileExists(MENU_SFX_CLICK)) {
            menu->hoverSfx = LoadSound(MENU_SFX_HOVER);
            menu->clickSfx = LoadSound(MENU_SFX_CLICK);
            menu->audioLoaded = true;
        }
        if (FileExists(MENU_BGM_PATH)) {
            menu->bgm = LoadMusicStream(MENU_BGM_PATH);
            menu->bgm.looping = true;
            menu->bgmLoaded = true;
        }
    }

    Menu_ResetIntroAnim(menu);
    Menu_Resize(menu, screenWidth, screenHeight);
}

void Menu_StartMusic(MenuUI *menu) {
    if (!menu || !menu->bgmLoaded) return;
    SeekMusicStream(menu->bgm, 0.0f);
    PlayMusicStream(menu->bgm);
}

void Menu_StopMusic(MenuUI *menu) {
    if (!menu) return;
    if (menu->bgmLoaded && IsMusicStreamPlaying(menu->bgm)) {
        StopMusicStream(menu->bgm);
    }
    if (menu->audioLoaded) {
        StopSound(menu->hoverSfx);
        StopSound(menu->clickSfx);
    }
}

void Menu_Update(MenuUI *menu, MenuAction *outAction, float dt) {
    if (outAction) *outAction = MENU_ACTION_NONE;
    if (!menu) return;

    if (menu->bgmLoaded) {
        UpdateMusicStream(menu->bgm);
        float totalLen = GetMusicTimeLength(menu->bgm);
        float played   = GetMusicTimePlayed(menu->bgm);
        if (!IsMusicStreamPlaying(menu->bgm) || (totalLen > 0.0f && played >= totalLen - 0.05f)) {
            StopMusicStream(menu->bgm);
            SeekMusicStream(menu->bgm, 0.0f);
            PlayMusicStream(menu->bgm);
        }
    }

    if (menu->openAnim < 1.0f) {
        menu->openAnim += dt * 1.4f;
        if (menu->openAnim > 1.0f) menu->openAnim = 1.0f;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            menu->openAnim = 1.0f;
        } else {
            return;
        }
    }

    Vector2 mouse = GetMousePosition();
    int prevSelected = menu->selectedIndex;
    int hoveredIndex = -1;

    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        MenuButton *btn = &menu->buttons[i];
        btn->isHovered = CheckCollisionPointRec(mouse, btn->baseBounds);
        btn->isPressed = btn->isHovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

        if (btn->isHovered) {
            hoveredIndex = i;
        }
    }

    bool prevCornerHover = menu->exitCornerBtn.isHovered;
    menu->exitCornerBtn.isHovered = CheckCollisionPointRec(mouse, menu->exitCornerBtn.baseBounds);
    menu->exitCornerBtn.isPressed = menu->exitCornerBtn.isHovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    if (menu->exitCornerBtn.isHovered && !prevCornerHover) {
        PlayMenuHoverOnce(menu);
    }

    float cornerTarget = menu->exitCornerBtn.isHovered ? 1.0f : 0.0f;
    menu->exitCornerBtn.hoverAnim += (cornerTarget - menu->exitCornerBtn.hoverAnim) * Clamp01(dt * 14.0f);

    if (hoveredIndex != -1) {
        menu->selectedIndex = hoveredIndex;
    } else if (prevSelected != -1 && !IsKeyPressed(KEY_DOWN) && !IsKeyPressed(KEY_S) &&
               !IsKeyPressed(KEY_UP) && !IsKeyPressed(KEY_W)) {
        menu->selectedIndex = -1;
    }

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        menu->selectedIndex = (menu->selectedIndex + 1) % MENU_BUTTON_COUNT;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        menu->selectedIndex = (menu->selectedIndex <= 0) ? (MENU_BUTTON_COUNT - 1) : (menu->selectedIndex - 1);
    }

    if (menu->selectedIndex != prevSelected && menu->selectedIndex >= 0) {
        PlayMenuHoverOnce(menu);
    }

    bool anyHover = menu->exitCornerBtn.isHovered;
    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        MenuButton *btn = &menu->buttons[i];
        bool activeState = (btn->isHovered || menu->selectedIndex == i);
        if (activeState) anyHover = true;

        float target = activeState ? 1.0f : 0.0f;
        btn->hoverAnim += (target - btn->hoverAnim) * Clamp01(dt * 14.0f);
    }

    SetMouseCursor(anyHover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

    bool cornerExitClicked = (menu->exitCornerBtn.isHovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT));
    if (cornerExitClicked || IsKeyPressed(KEY_ESCAPE)) {
        PlayMenuClickOnce(menu);
        menu->exitRequested = true;
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        if (outAction) *outAction = MENU_ACTION_EXIT;
        return;
    }

    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        MenuButton *btn = &menu->buttons[i];
        bool clicked  = (btn->isHovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT));
        bool keyEnter = (menu->selectedIndex == i && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)));

        if (clicked || keyEnter) {
            PlayMenuClickOnce(menu);
            if (btn->action == MENU_ACTION_EXIT) {
                menu->exitRequested = true;
            }
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            if (outAction) *outAction = btn->action;
            return;
        }
    }
}

void Menu_Draw(const MenuUI *menu) {
    if (!menu) return;

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float t  = menu->openAnim;
    float bgAlpha = Clamp01(t / 0.5f);
    float easeOut = 1.0f - powf(1.0f - bgAlpha, 3.0f);
    float scale   = 1.04f - 0.04f * easeOut;
    float fullAlpha = Clamp01((t - 0.4f) / 0.6f);

    Rectangle bgRect = {
        sw * (1.0f - scale) * 0.5f,
        sh * (1.0f - scale) * 0.5f,
        sw * scale,
        sh * scale
    };

    ClearBackground(BLACK);

    if (menu->background.id > 0) {
        DrawTextureFit(menu->background, bgRect, Fade(WHITE, bgAlpha));
    }
    if (menu->full.id > 0 && fullAlpha > 0.0f) {
        DrawTextureFit(menu->full, bgRect, Fade(WHITE, fullAlpha));
    }

    if (t >= 0.95f) {
        for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
            const MenuButton *btn = &menu->buttons[i];
            float slideOffset = -10.0f * btn->hoverAnim;
            Rectangle drawRect = {
                btn->baseBounds.x + slideOffset,
                btn->baseBounds.y,
                btn->baseBounds.width,
                btn->baseBounds.height
            };
            if (btn->isPressed) {
                drawRect.x += 2.0f;
                drawRect.y += 2.0f;
                drawRect.width -= 4.0f;
                drawRect.height -= 4.0f;
            }
            if (menu->full.id <= 0 && btn->normal.id > 0) {
                DrawTextureFit(btn->normal, drawRect, WHITE);
            }
            if (btn->hoverAnim > 0.01f) {
                if (btn->active.id > 0) {
                    DrawTextureFit(btn->active, drawRect, Fade(WHITE, btn->hoverAnim));
                } else {
                    DrawRectangleRec(drawRect, Fade(MENU_ACCENT_MINT, 0.35f * btn->hoverAnim));
                    DrawRectangleLinesEx(drawRect, 2.0f, Fade(MENU_ACCENT_MINT, btn->hoverAnim));
                }
            }
        }

        const MenuButton *cornerBtn = &menu->exitCornerBtn;
        Rectangle cRect = cornerBtn->baseBounds;
        if (cornerBtn->isPressed) {
            cRect.x += 1.5f;
            cRect.y += 1.5f;
            cRect.width -= 3.0f;
            cRect.height -= 3.0f;
        }
        if (cornerBtn->normal.id > 0) {
            DrawTextureFit(cornerBtn->normal, cRect, WHITE);
        }
        if (cornerBtn->active.id > 0 && cornerBtn->hoverAnim > 0.01f) {
            DrawTextureFit(cornerBtn->active, cRect, Fade(WHITE, cornerBtn->hoverAnim));
        }
    }
}

void Menu_Unload(MenuUI *menu) {
    if (!menu) return;
    UnloadTextureSafe(&menu->background);
    UnloadTextureSafe(&menu->full);

    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        UnloadTextureSafe(&menu->buttons[i].normal);
        UnloadTextureSafe(&menu->buttons[i].active);
    }

    UnloadTextureSafe(&menu->exitCornerBtn.normal);
    UnloadTextureSafe(&menu->exitCornerBtn.active);

    if (menu->audioLoaded) {
        StopSound(menu->hoverSfx);
        StopSound(menu->clickSfx);
        UnloadSound(menu->hoverSfx);
        UnloadSound(menu->clickSfx);
        menu->audioLoaded = false;
    }
    if (menu->bgmLoaded) {
        StopMusicStream(menu->bgm);
        UnloadMusicStream(menu->bgm);
        menu->bgmLoaded = false;
    }
}

bool Menu_IsExitRequested(const MenuUI *menu) {
    if (!menu) return false;
    return menu->exitRequested;
}