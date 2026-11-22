#include "Load.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static void SanitizeFileName(const char *rawName, char *outFileName, size_t maxLen) {
    size_t j = 0;
    if (rawName != NULL) {
        for (size_t i = 0; rawName[i] != '\0' && j + 1 < maxLen; i++) {
            char c = rawName[i];
            if ((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == '_' || c == '-' || c == ' ') {
                outFileName[j++] = (c == ' ') ? '_' : c;
            }
        }
    }
    if (j == 0 && maxLen > 8) {
        strcpy(outFileName, "autosave");
        return;
    }
    outFileName[j] = '\0';
}

static void KeepMenuBgmAlive(MenuUI *menu) {
    if (!menu || !menu->bgmLoaded) return;
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

static void PlayMenuUiSfx(MenuUI *menu, bool isClick) {
    if (!menu || !menu->audioLoaded) return;
    Sound sfx = isClick ? menu->clickSfx : menu->hoverSfx;
    StopSound(sfx);
    PlaySound(sfx);
}

bool Load_SaveMatch(const SavedMatchData *matchData) {
    if (!matchData) return false;

    if (!DirectoryExists(LOAD_SAVE_DIR)) {
        MakeDirectory(LOAD_SAVE_DIR);
    }

    char cleanName[64];
    SanitizeFileName(matchData->saveName, cleanName, sizeof(cleanName));

    long now = (long)time(NULL);
    char fullPath[260];
    int seq = 1;
    do {
        snprintf(fullPath, sizeof(fullPath), "%s/%s_%ld_%02d.sav", LOAD_SAVE_DIR, cleanName, now, seq);
        seq++;
    } while (FileExists(fullPath) && seq < 100);

    FILE *fp = fopen(fullPath, "wb");
    if (!fp) return false;

    size_t written = fwrite(matchData, sizeof(SavedMatchData), 1, fp);
    fclose(fp);
    return (written == 1);
}

static bool ReadSaveFile(const char *path, SavedMatchData *outData) {
    if (!path || !outData || !FileExists(path)) return false;
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;

    size_t readCount = fread(outData, sizeof(SavedMatchData), 1, fp);
    fclose(fp);
    return (readCount == 1);
}

static int CompareSlotsNewestFirst(const void *a, const void *b) {
    const LoadSlotEntry *sa = (const LoadSlotEntry *)a;
    const LoadSlotEntry *sb = (const LoadSlotEntry *)b;
    if (sb->modTime > sa->modTime) return 1;
    if (sb->modTime < sa->modTime) return -1;
    return strcmp(sb->filePath, sa->filePath);
}

static bool EnsureSlotCapacity(LoadUI *ui, int needed) {
    if (needed <= ui->slotCapacity) return true;
    int newCap = (ui->slotCapacity == 0) ? 16 : (ui->slotCapacity * 2);
    while (newCap < needed) newCap *= 2;
    LoadSlotEntry *newBuf = (LoadSlotEntry *)realloc(ui->slots, (size_t)newCap * sizeof(LoadSlotEntry));
    if (!newBuf) return false;
    ui->slots = newBuf;
    ui->slotCapacity = newCap;
    return true;
}

void Load_RefreshList(LoadUI *ui) {
    if (!ui) return;
    ui->slotCount = 0;

    if (!DirectoryExists(LOAD_SAVE_DIR)) {
        ui->selectedIndex = -1;
        ui->scrollOffset  = 0;
        return;
    }

    FilePathList files = LoadDirectoryFilesEx(LOAD_SAVE_DIR, ".sav", false);
    for (unsigned int i = 0; i < files.count; i++) {
        SavedMatchData temp;
        if (ReadSaveFile(files.paths[i], &temp)) {
            if (!EnsureSlotCapacity(ui, ui->slotCount + 1)) break;
            LoadSlotEntry *entry = &ui->slots[ui->slotCount];
            snprintf(entry->filePath, sizeof(entry->filePath), "%s", files.paths[i]);
            entry->modTime = GetFileModTime(files.paths[i]);
            entry->data = temp;
            if (entry->data.saveName[0] == '\0') {
                snprintf(entry->data.saveName, sizeof(entry->data.saveName), "%s", GetFileNameWithoutExt(files.paths[i]));
            }
            ui->slotCount++;
        }
    }
    UnloadDirectoryFiles(files);

    if (ui->slotCount > 1) {
        qsort(ui->slots, (size_t)ui->slotCount, sizeof(LoadSlotEntry), CompareSlotsNewestFirst);
    }

    if (ui->slotCount == 0) {
        ui->selectedIndex = -1;
        ui->scrollOffset  = 0;
    } else {
        if (ui->selectedIndex < 0 || ui->selectedIndex >= ui->slotCount) {
            ui->selectedIndex = 0;
        }
        int maxScroll = (ui->slotCount > LOAD_VISIBLE_SLOTS) ? (ui->slotCount - LOAD_VISIBLE_SLOTS) : 0;
        if (ui->scrollOffset > maxScroll) ui->scrollOffset = maxScroll;
        if (ui->scrollOffset < 0) ui->scrollOffset = 0;
    }
}

void Load_Resize(LoadUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;
    float sw = (float)screenWidth;
    float sh = (float)screenHeight;

    Texture2D refTex = (ui->full.id > 0) ? ui->full : ui->bg;
    float panelW = fminf(700.0f, sw * 0.58f);
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

    float closeSize = p.width * 0.066f;
    ui->closeBtnRect = (Rectangle){
        p.x + p.width * 0.888f,
        p.y + p.height * 0.088f,
        closeSize,
        closeSize
    };

    float slotX = p.x + p.width * 0.082f;
    float slotW = p.width * 0.815f;
    float slotH = p.height * 0.175f;
    ui->slotRects[0] = (Rectangle){ slotX, p.y + p.height * 0.258f, slotW, slotH };
    ui->slotRects[1] = (Rectangle){ slotX, p.y + p.height * 0.505f, slotW, slotH };

    ui->scrollTrackRect = (Rectangle){
        slotX + slotW + p.width * 0.012f,
        ui->slotRects[0].y,
        p.width * 0.014f,
        (ui->slotRects[1].y + ui->slotRects[1].height) - ui->slotRects[0].y
    };

    float btnW = p.width * 0.245f;
    float btnH = p.height * 0.102f;
    float btnY = p.y + p.height * 0.785f;

    ui->loadBtnRect   = (Rectangle){ p.x + p.width * 0.215f, btnY, btnW, btnH };
    ui->deleteBtnRect = (Rectangle){ p.x + p.width * 0.540f, btnY, btnW, btnH };
}

void Load_Open(LoadUI *ui) {
    if (!ui) return;
    ui->openAnim          = 0.0f;
    ui->hoverCloseBtn     = false;
    ui->hoverLoadBtn      = false;
    ui->hoverDeleteBtn    = false;
    ui->pressLoadBtn      = false;
    ui->pressDeleteBtn    = false;
    ui->draggingScrollbar = false;
    ui->hoveredRow        = -1;
    ui->statusTimer       = 0.0f;
    ui->statusMsg[0]      = '\0';
    Load_RefreshList(ui);
}

void Load_Init(LoadUI *ui, int screenWidth, int screenHeight) {
    if (!ui) return;
    memset(ui, 0, sizeof(LoadUI));

    ui->bg           = LoadTextureSafe(LOAD_IMG_BG);
    ui->full         = LoadTextureSafe(LOAD_IMG_FULL);
    ui->closeNormal  = LoadTextureSafe(LOAD_IMG_CLOSE_NORMAL);
    ui->closeHover   = LoadTextureSafe(LOAD_IMG_CLOSE_HOVER);
    ui->deleteNormal = LoadTextureSafe(LOAD_IMG_DELETE_NORMAL);
    ui->deleteHover  = LoadTextureSafe(LOAD_IMG_DELETE_HOVER);
    ui->loadNormal   = LoadTextureSafe(LOAD_IMG_LOAD_NORMAL);
    ui->loadHover    = LoadTextureSafe(LOAD_IMG_LOAD_HOVER);

    Load_Open(ui);
    Load_Resize(ui, screenWidth, screenHeight);
}

LoadAction Load_Update(LoadUI *ui, MenuUI *menu, float dt) {
    if (!ui) return LOAD_ACTION_NONE;

    KeepMenuBgmAlive(menu);

    if (ui->statusTimer > 0.0f) {
        ui->statusTimer -= dt;
        if (ui->statusTimer < 0.0f) ui->statusTimer = 0.0f;
    }

    if (ui->openAnim < 1.0f) {
        ui->openAnim += dt * 3.5f;
        if (ui->openAnim > 1.0f) ui->openAnim = 1.0f;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE)) {
            ui->openAnim = 1.0f;
        } else {
            return LOAD_ACTION_NONE;
        }
    }

    Vector2 mouse = GetMousePosition();
    bool prevHoverClose  = ui->hoverCloseBtn;
    bool prevHoverLoad   = ui->hoverLoadBtn;
    bool prevHoverDelete = ui->hoverDeleteBtn;

    ui->hoverCloseBtn  = CheckCollisionPointRec(mouse, ui->closeBtnRect);
    ui->hoverLoadBtn   = CheckCollisionPointRec(mouse, ui->loadBtnRect);
    ui->hoverDeleteBtn = CheckCollisionPointRec(mouse, ui->deleteBtnRect);
    ui->pressLoadBtn   = ui->hoverLoadBtn && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    ui->pressDeleteBtn = ui->hoverDeleteBtn && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    ui->hoveredRow = -1;
    for (int i = 0; i < LOAD_VISIBLE_SLOTS; i++) {
        int dataIdx = ui->scrollOffset + i;
        if (dataIdx < ui->slotCount && CheckCollisionPointRec(mouse, ui->slotRects[i])) {
            ui->hoveredRow = i;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (ui->selectedIndex != dataIdx) {
                    PlayMenuUiSfx(menu, false);
                }
                ui->selectedIndex = dataIdx;
            }
        }
    }

    if ((ui->hoverCloseBtn && !prevHoverClose) ||
        (ui->hoverLoadBtn && !prevHoverLoad) ||
        (ui->hoverDeleteBtn && !prevHoverDelete)) {
        PlayMenuUiSfx(menu, false);
    }

    int maxScroll = (ui->slotCount > LOAD_VISIBLE_SLOTS) ? (ui->slotCount - LOAD_VISIBLE_SLOTS) : 0;

    if (maxScroll > 0) {
        float wheel = GetMouseWheelMove();
        if (wheel < 0.0f && ui->scrollOffset < maxScroll) {
            ui->scrollOffset++;
        } else if (wheel > 0.0f && ui->scrollOffset > 0) {
            ui->scrollOffset--;
        }

        Rectangle hitTrack = {
            ui->scrollTrackRect.x - 6.0f,
            ui->scrollTrackRect.y,
            ui->scrollTrackRect.width + 12.0f,
            ui->scrollTrackRect.height
        };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, hitTrack)) {
            ui->draggingScrollbar = true;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            ui->draggingScrollbar = false;
        }
        if (ui->draggingScrollbar) {
            float ratio = Clamp01((mouse.y - ui->scrollTrackRect.y) / ui->scrollTrackRect.height);
            ui->scrollOffset = (int)roundf(ratio * (float)maxScroll);
        }
    } else {
        ui->draggingScrollbar = false;
    }

    if (ui->slotCount > 0) {
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            ui->selectedIndex = (ui->selectedIndex + 1) % ui->slotCount;
            if (ui->selectedIndex < ui->scrollOffset) ui->scrollOffset = ui->selectedIndex;
            if (ui->selectedIndex >= ui->scrollOffset + LOAD_VISIBLE_SLOTS) {
                ui->scrollOffset = ui->selectedIndex - LOAD_VISIBLE_SLOTS + 1;
            }
            PlayMenuUiSfx(menu, false);
        }
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            ui->selectedIndex = (ui->selectedIndex <= 0) ? (ui->slotCount - 1) : (ui->selectedIndex - 1);
            if (ui->selectedIndex < ui->scrollOffset) ui->scrollOffset = ui->selectedIndex;
            if (ui->selectedIndex >= ui->scrollOffset + LOAD_VISIBLE_SLOTS) {
                ui->scrollOffset = ui->selectedIndex - LOAD_VISIBLE_SLOTS + 1;
            }
            PlayMenuUiSfx(menu, false);
        }
    }

    if (IsKeyPressed(KEY_ESCAPE) ||
        (ui->hoverCloseBtn && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))) {
        PlayMenuUiSfx(menu, true);
        ui->draggingScrollbar = false;
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        return LOAD_ACTION_BACK;
    }

    bool triggerDelete = (ui->hoverDeleteBtn && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) ||
                         IsKeyPressed(KEY_DELETE);
    if (triggerDelete) {
        PlayMenuUiSfx(menu, true);
        if (ui->selectedIndex >= 0 && ui->selectedIndex < ui->slotCount) {
            remove(ui->slots[ui->selectedIndex].filePath);
            snprintf(ui->statusMsg, sizeof(ui->statusMsg), "DELETED SAVE SLOT");
            ui->statusTimer = 1.8f;
            Load_RefreshList(ui);
        }
    }

    bool triggerLoad = (ui->hoverLoadBtn && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) ||
                       IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    if (triggerLoad) {
        PlayMenuUiSfx(menu, true);
        if (ui->selectedIndex >= 0 && ui->selectedIndex < ui->slotCount) {
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            return LOAD_ACTION_LOADED;
        } else {
            snprintf(ui->statusMsg, sizeof(ui->statusMsg), "NO SAVE FILE SELECTED");
            ui->statusTimer = 1.8f;
        }
    }

    bool anyHover = (ui->hoverCloseBtn || ui->hoverLoadBtn || ui->hoverDeleteBtn ||
                     ui->hoveredRow != -1 || ui->draggingScrollbar);
    SetMouseCursor(anyHover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

    return LOAD_ACTION_NONE;
}

void Load_Draw(const LoadUI *ui) {
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

        if (ui->slotCount == 0) {
            const char *emptyTxt = "NO SAVED MATCHES";
            int fontSize = (int)(ui->slotRects[0].height * 0.38f);
            if (fontSize < 16) fontSize = 16;
            int tw = MeasureText(emptyTxt, fontSize);
            DrawText(emptyTxt,
                     (int)(ui->slotRects[0].x + (ui->slotRects[0].width - tw) * 0.5f),
                     (int)(ui->slotRects[0].y + (ui->slotRects[0].height - fontSize) * 0.5f),
                     fontSize,
                     Fade(LIGHTGRAY, 0.75f));
        } else {
            for (int i = 0; i < LOAD_VISIBLE_SLOTS; i++) {
                int dataIdx = ui->scrollOffset + i;
                if (dataIdx >= ui->slotCount) break;

                Rectangle r = ui->slotRects[i];
                const SavedMatchData *sd = &ui->slots[dataIdx].data;
                bool isSelected = (dataIdx == ui->selectedIndex);
                bool isHovered  = (ui->hoveredRow == i);

                if (isSelected) {
                    DrawRectangleRec(r, Fade(LOAD_ACCENT_MINT, 0.20f));
                    DrawRectangleLinesEx(r, 2.0f, LOAD_ACCENT_MINT);
                } else if (isHovered) {
                    DrawRectangleRec(r, Fade(WHITE, 0.08f));
                    DrawRectangleLinesEx(r, 1.5f, Fade(LOAD_ACCENT_MINT, 0.55f));
                }

                int titleSize = (int)(r.height * 0.38f);
                int subSize   = (int)(r.height * 0.26f);
                if (titleSize < 16) titleSize = 16;
                if (subSize < 12)   subSize = 12;

                float padX = r.width * 0.035f;
                char titleLine[96];
                snprintf(titleLine, sizeof(titleLine), "#%d  %s", dataIdx + 1, sd->saveName);

                DrawText(titleLine,
                         (int)(r.x + padX),
                         (int)(r.y + r.height * 0.14f),
                         titleSize,
                         isSelected ? LOAD_ACCENT_MINT : WHITE);

                char infoLine[96];
                snprintf(infoLine, sizeof(infoLine),
                         "ROUND %d  |  %s (%d) vs %s (%d)",
                         sd->round,
                         sd->p1Name[0] ? sd->p1Name : "ALPHA", sd->p1Wins,
                         sd->p2Name[0] ? sd->p2Name : "OMEGA", sd->p2Wins);

                DrawText(infoLine,
                         (int)(r.x + padX),
                         (int)(r.y + r.height * 0.58f),
                         subSize,
                         LIGHTGRAY);
            }

            if (ui->slotCount > LOAD_VISIBLE_SLOTS) {
                DrawRectangleRec(ui->scrollTrackRect, Fade(WHITE, 0.14f));
                int maxScroll = ui->slotCount - LOAD_VISIBLE_SLOTS;
                float thumbRatio = (float)LOAD_VISIBLE_SLOTS / (float)ui->slotCount;
                float thumbH = fmaxf(22.0f, ui->scrollTrackRect.height * thumbRatio);
                float posRatio = (maxScroll > 0) ? ((float)ui->scrollOffset / (float)maxScroll) : 0.0f;
                float thumbY = ui->scrollTrackRect.y + (ui->scrollTrackRect.height - thumbH) * posRatio;

                Rectangle thumbRect = {
                    ui->scrollTrackRect.x,
                    thumbY,
                    ui->scrollTrackRect.width,
                    thumbH
                };
                DrawRectangleRec(thumbRect, Fade(LOAD_ACCENT_MINT, 0.85f));
            }
        }

        Rectangle loadRect = ui->loadBtnRect;
        if (ui->pressLoadBtn) {
            loadRect.x += 1.5f;
            loadRect.y += 1.5f;
        }
        Texture2D loadTex = (ui->hoverLoadBtn && ui->loadHover.id > 0)
                            ? ui->loadHover
                            : ui->loadNormal;
        if (loadTex.id > 0) {
            DrawTextureFit(loadTex, loadRect, (ui->slotCount > 0) ? WHITE : Fade(WHITE, 0.45f));
        }

        Rectangle delRect = ui->deleteBtnRect;
        if (ui->pressDeleteBtn) {
            delRect.x += 1.5f;
            delRect.y += 1.5f;
        }
        Texture2D delTex = (ui->hoverDeleteBtn && ui->deleteHover.id > 0)
                           ? ui->deleteHover
                           : ui->deleteNormal;
        if (delTex.id > 0) {
            DrawTextureFit(delTex, delRect, (ui->slotCount > 0) ? WHITE : Fade(WHITE, 0.45f));
        }

        if (ui->statusTimer > 0.0f && ui->statusMsg[0] != '\0') {
            int msgSize = 16;
            int mw = MeasureText(ui->statusMsg, msgSize);
            DrawText(ui->statusMsg,
                     (int)(ui->panelRect.x + (ui->panelRect.width - mw) * 0.5f),
                     (int)(ui->panelRect.y + ui->panelRect.height * 0.91f),
                     msgSize,
                     LOAD_ACCENT_RED);
        }
    }
}

void Load_Unload(LoadUI *ui) {
    if (!ui) return;

    UnloadTextureSafe(&ui->bg);
    UnloadTextureSafe(&ui->full);
    UnloadTextureSafe(&ui->closeNormal);
    UnloadTextureSafe(&ui->closeHover);
    UnloadTextureSafe(&ui->deleteNormal);
    UnloadTextureSafe(&ui->deleteHover);
    UnloadTextureSafe(&ui->loadNormal);
    UnloadTextureSafe(&ui->loadHover);

    if (ui->slots != NULL) {
        free(ui->slots);
        ui->slots = NULL;
    }
    ui->slotCount = 0;
    ui->slotCapacity = 0;
}

const SavedMatchData *Load_GetSelectedData(const LoadUI *ui) {
    if (!ui || ui->selectedIndex < 0 || ui->selectedIndex >= ui->slotCount) {
        return NULL;
    }
    return &ui->slots[ui->selectedIndex].data;
}