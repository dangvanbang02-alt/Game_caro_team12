#include "HUD.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define HUD_ACCENT_MINT  (Color){ 44, 229, 167, 255 }
#define HUD_ACCENT_RED   (Color){ 255, 70, 85, 255 }
#define HUD_PANEL_DARK   (Color){ 16, 20, 27, 230 }

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

static Rectangle CalcCenteredSquareInCell(Rectangle cellRect, float fillRatio, float scaleMultiplier) {
    float baseSide = fminf(cellRect.width, cellRect.height) * fillRatio * scaleMultiplier;
    return (Rectangle){
        cellRect.x + (cellRect.width  - baseSide) * 0.5f,
        cellRect.y + (cellRect.height - baseSide) * 0.5f,
        baseSide,
        baseSide
    };
}

static void DrawTextureCenteredKeepAspect(Texture2D tex, Rectangle targetBox, Color tint) {
    if (tex.id <= 0 || tex.width <= 0 || tex.height <= 0) return;
    float texAspect = (float)tex.width / (float)tex.height;
    float boxAspect = targetBox.width / targetBox.height;

    Rectangle dest = targetBox;
    if (texAspect > boxAspect) {
        dest.height = targetBox.width / texAspect;
        dest.y += (targetBox.height - dest.height) * 0.5f;
    } else {
        dest.width = targetBox.height * texAspect;
        dest.x += (targetBox.width - dest.width) * 0.5f;
    }
    DrawTextureFit(tex, dest, tint);
}

static void PlayHudCellHoverOnce(GameHUD *hud) {
    if (!hud || !hud->cellHoverSfxLoaded) return;
    StopSound(hud->cellHoverSfx);
    PlaySound(hud->cellHoverSfx);
}

static void PlayMenuClickFromHud(MenuUI *menu) {
    if (!menu || !menu->audioLoaded) return;
    StopSound(menu->hoverSfx);
    StopSound(menu->clickSfx);
    PlaySound(menu->clickSfx);
}

void HUD_ResetBoardVisuals(GameHUD *hud) {
    if (!hud) return;
    hud->hoveredCellRow = -1;
    hud->hoveredCellCol = -1;
    memset(hud->cellHoverAnim, 0, sizeof(hud->cellHoverAnim));
    memset(hud->cellPlaceAnim, 0, sizeof(hud->cellPlaceAnim));
}

void HUD_SetSfxVolume(GameHUD *hud, float volume) {
    if (!hud) return;
    hud->sfxVolume = Clamp01(volume);
    if (hud->cellHoverSfxLoaded) {
        SetSoundVolume(hud->cellHoverSfx, hud->sfxVolume);
    }
}

bool HUD_UpdateBoardCell(GameHUD *hud,
                         MenuUI *menu,
                         Rectangle boardRect,
                         int rows,
                         int cols,
                         const int board[HUD_MAX_BOARD_DIM][HUD_MAX_BOARD_DIM],
                         bool canInteract,
                         int *outClickedRow,
                         int *outClickedCol,
                         float dt) {
    if (outClickedRow) *outClickedRow = -1;
    if (outClickedCol) *outClickedCol = -1;
    if (!hud || !board || rows <= 0 || cols <= 0) return false;

    if (rows > HUD_MAX_BOARD_DIM) rows = HUD_MAX_BOARD_DIM;
    if (cols > HUD_MAX_BOARD_DIM) cols = HUD_MAX_BOARD_DIM;

    int prevHoverR = hud->hoveredCellRow;
    int prevHoverC = hud->hoveredCellCol;
    hud->hoveredCellRow = -1;
    hud->hoveredCellCol = -1;

    float cellW = boardRect.width / (float)cols;
    float cellH = boardRect.height / (float)rows;

    if (canInteract && !hud->isPaused && !hud->isSaveNaming && !hud->matchStopped) {
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, boardRect)) {
            int c = (int)((mouse.x - boardRect.x) / cellW);
            int r = (int)((mouse.y - boardRect.y) / cellH);
            if (r >= 0 && r < rows && c >= 0 && c < cols && board[r][c] == 0) {
                hud->hoveredCellRow = r;
                hud->hoveredCellCol = c;
            }
        }
    }

    if (hud->hoveredCellRow != -1 &&
        (hud->hoveredCellRow != prevHoverR || hud->hoveredCellCol != prevHoverC)) {
        PlayHudCellHoverOnce(hud);
    }

    bool clicked = false;
    if (hud->hoveredCellRow != -1 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int r = hud->hoveredCellRow;
        int c = hud->hoveredCellCol;
        PlayMenuClickFromHud(menu);
        hud->cellPlaceAnim[r][c] = 0.01f;
        hud->cellHoverAnim[r][c] = 0.0f;
        if (outClickedRow) *outClickedRow = r;
        if (outClickedCol) *outClickedCol = c;
        clicked = true;
    }

    float hoverSpeed = Clamp01(dt * 18.0f);
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            float targetH = (board[r][c] == 0 && hud->hoveredCellRow == r && hud->hoveredCellCol == c) ? 1.0f : 0.0f;
            hud->cellHoverAnim[r][c] += (targetH - hud->cellHoverAnim[r][c]) * hoverSpeed;

            if (board[r][c] != 0) {
                if (hud->cellPlaceAnim[r][c] <= 0.0f) {
                    hud->cellPlaceAnim[r][c] = 1.0f;
                } else if (hud->cellPlaceAnim[r][c] < 1.0f) {
                    hud->cellPlaceAnim[r][c] += dt * 6.5f;
                    if (hud->cellPlaceAnim[r][c] > 1.0f) hud->cellPlaceAnim[r][c] = 1.0f;
                }
            } else {
                hud->cellPlaceAnim[r][c] = 0.0f;
            }
        }
    }

    if (!hud->isPaused && !hud->isSaveNaming) {
        SetMouseCursor((hud->hoveredCellRow != -1 || hud->hoverPauseIcon)
                       ? MOUSE_CURSOR_POINTING_HAND
                       : MOUSE_CURSOR_DEFAULT);
    }

    return clicked;
}

void HUD_DrawBoardCell(const GameHUD *hud,
                       Rectangle cellRect,
                       int row,
                       int col,
                       int cellValue,
                       bool isWinningCell) {
    if (!hud || row < 0 || row >= HUD_MAX_BOARD_DIM || col < 0 || col >= HUD_MAX_BOARD_DIM) return;

    Rectangle frameBox = CalcCenteredSquareInCell(cellRect, 0.90f, 1.0f);
    if (hud->png.cellEmpty.id > 0) {
        DrawTextureCenteredKeepAspect(hud->png.cellEmpty, frameBox, WHITE);
    }

    if (cellValue == 0) {
        float hAnim = hud->cellHoverAnim[row][col];
        if (hAnim > 0.01f) {
            if (hud->png.cellHoverFrame.id > 0) {
                DrawTextureCenteredKeepAspect(hud->png.cellHoverFrame, frameBox, Fade(WHITE, hAnim));
            }
            Texture2D previewTex = hud->p1.isTurn ? hud->png.cellHoverAlpha : hud->png.cellHoverOmega;
            if (previewTex.id > 0) {
                Rectangle previewBox = CalcCenteredSquareInCell(cellRect, 0.84f, 0.94f + 0.06f * hAnim);
                DrawTextureCenteredKeepAspect(previewTex, previewBox, Fade(WHITE, 0.88f * hAnim));
            }
        }
        return;
    }

    float pAnim = hud->cellPlaceAnim[row][col];
    if (pAnim <= 0.0f) pAnim = 1.0f;

    bool isAlpha = (cellValue == 1);
    Texture2D previewTex = isAlpha ? hud->png.cellHoverAlpha : hud->png.cellHoverOmega;
    Texture2D finalTex   = isAlpha ? hud->png.pieceAlpha     : hud->png.pieceOmega;

    if (pAnim < 1.0f) {
        float ease = 1.0f - powf(1.0f - pAnim, 3.0f);
        float morphScale = 1.18f - 0.18f * ease;

        if (previewTex.id > 0 && ease < 0.75f) {
            Rectangle pBox = CalcCenteredSquareInCell(cellRect, 0.78f, morphScale);
            DrawTextureCenteredKeepAspect(previewTex, pBox, Fade(WHITE, 1.0f - ease));
        }
        if (finalTex.id > 0) {
            Rectangle fBox = CalcCenteredSquareInCell(cellRect, 0.68f, morphScale);
            DrawTextureCenteredKeepAspect(finalTex, fBox, Fade(WHITE, ease));
        }
    } else {
        float winScale = isWinningCell ? (1.0f + 0.07f * sinf((float)GetTime() * 8.0f)) : 1.0f;
        Rectangle fBox = CalcCenteredSquareInCell(cellRect, 0.68f, winScale);
        if (finalTex.id > 0) {
            DrawTextureCenteredKeepAspect(finalTex, fBox, WHITE);
        }
    }

    if (isWinningCell) {
        Color winCol = isAlpha ? HUD_ACCENT_MINT : HUD_ACCENT_RED;
        DrawRectangleLinesEx(frameBox, 2.5f, winCol);
    }
}

void HUD_Resize(GameHUD *hud, int screenWidth, int screenHeight) {
    if (!hud) return;
    float sw = (float)screenWidth;
    float sh = (float)screenHeight;

    float panelW = fminf(280.0f, sw * 0.22f);
    float panelH = sh * 0.28f;
    float marginX = sw * 0.025f;
    float topY    = sh * 0.14f;

    hud->alphaPanel = (Rectangle){ marginX, topY, panelW, panelH };
    hud->omegaPanel = (Rectangle){ sw - marginX - panelW, topY + panelH + sh * 0.04f, panelW, panelH };
    hud->bottomBar  = (Rectangle){ marginX, sh - sh * 0.08f, sw - marginX * 2.0f, sh * 0.055f };

    float pauseSize = fminf(46.0f, sh * 0.06f);
    hud->pauseIconRect = (Rectangle){ sw - marginX - pauseSize, sh * 0.03f, pauseSize, pauseSize };

    float pW = fminf(560.0f, sw * 0.48f);
    float pH = pW * 0.58f;
    hud->pausePanelRect = (Rectangle){ (sw - pW) * 0.5f, (sh - pH) * 0.5f, pW, pH };

    float cSize = pW * 0.07f;
    hud->closeBtnRect = (Rectangle){
        hud->pausePanelRect.x + pW * 0.89f,
        hud->pausePanelRect.y + pH * 0.08f,
        cSize, cSize
    };

    float btnW = pW * 0.36f;
    float btnH = pH * 0.14f;
    hud->saveBtnRect = (Rectangle){
        hud->pausePanelRect.x + (pW - btnW) * 0.5f,
        hud->pausePanelRect.y + pH * 0.44f,
        btnW, btnH
    };
    hud->exitBtnRect = (Rectangle){
        hud->pausePanelRect.x + (pW - btnW) * 0.5f,
        hud->pausePanelRect.y + pH * 0.65f,
        btnW, btnH
    };

    float nW = fminf(600.0f, sw * 0.50f);
    float nH = nW * 0.56f;
    hud->namePanelRect = (Rectangle){ (sw - nW) * 0.5f, (sh - nH) * 0.5f, nW, nH };
    hud->nameCloseBtnRect = (Rectangle){
        hud->namePanelRect.x + nW * 0.885f,
        hud->namePanelRect.y + nH * 0.085f,
        nW * 0.068f, nW * 0.068f
    };
    hud->nameInputBoxRect = (Rectangle){
        hud->namePanelRect.x + nW * 0.22f,
        hud->namePanelRect.y + nH * 0.43f,
        nW * 0.56f, nH * 0.16f
    };
    hud->nameConfirmBtnRect = (Rectangle){
        hud->namePanelRect.x + nW * 0.38f,
        hud->namePanelRect.y + nH * 0.68f,
        nW * 0.24f, nH * 0.11f
    };
}

void HUD_Init(GameHUD *hud, int screenWidth, int screenHeight) {
    if (!hud) return;
    memset(hud, 0, sizeof(GameHUD));

    hud->round         = 1;
    hud->p1.piece      = 1;
    hud->p1.isTurn     = true;
    hud->p2.piece      = 2;
    hud->p2.isTurn     = false;
    hud->showTurnPng   = true;
    hud->prevAlphaTurn = true;
    hud->turnBlend     = 1.0f;
    hud->bgmBaseVolume = 1.0f;
    hud->sfxVolume     = 1.0f;
    hud->matchStatus   = HUD_MATCH_ONGOING;
    snprintf(hud->p1.name, sizeof(hud->p1.name), "ALPHA");
    snprintf(hud->p2.name, sizeof(hud->p2.name), "OMEGA");

    hud->png.pipEmpty         = LoadTextureSafe(HUD_PNG_PIP_EMPTY);
    hud->png.pipAlpha         = LoadTextureSafe(HUD_PNG_PIP_ALPHA);
    hud->png.pipOmega         = LoadTextureSafe(HUD_PNG_PIP_OMEGA);
    hud->png.agentAvatar[HUD_AGENT_PHOENIX] = LoadTextureSafe(HUD_PNG_AVATAR_PHOENIX);
    hud->png.agentAvatar[HUD_AGENT_JETT]    = LoadTextureSafe(HUD_PNG_AVATAR_JETT);
    hud->png.agentAvatar[HUD_AGENT_SAGE]    = LoadTextureSafe(HUD_PNG_AVATAR_SAGE);
    hud->png.turnAlphaPng     = LoadTextureSafe(HUD_PNG_TURN_ALPHA);
    hud->png.turnOmegaPng     = LoadTextureSafe(HUD_PNG_TURN_OMEGA);

    hud->png.pauseBg          = LoadTextureSafe(HUD_PNG_PAUSE_BG);
    hud->png.pauseFull        = LoadTextureSafe(HUD_PNG_PAUSE_FULL);
    hud->png.pauseIconNormal  = LoadTextureSafe(HUD_PNG_PAUSE_ICON_NORMAL);
    hud->png.pauseIconHover   = LoadTextureSafe(HUD_PNG_PAUSE_ICON_HOVER);
    hud->png.closeNormal      = LoadTextureSafe(HUD_PNG_CLOSE_NORMAL);
    hud->png.closeHover       = LoadTextureSafe(HUD_PNG_CLOSE_HOVER);

    hud->png.saveNameBg       = LoadTextureSafe(HUD_PNG_SAVE_NAME_BG);
    hud->png.saveNameFull     = LoadTextureSafe(HUD_PNG_SAVE_NAME_FULL);
    hud->png.saveConfirmNormal= LoadTextureSafe(HUD_PNG_SAVE_CONFIRM_NORM);
    hud->png.saveConfirmHover = LoadTextureSafe(HUD_PNG_SAVE_CONFIRM_HOVER);
    hud->png.nameCloseNormal  = LoadTextureSafe(HUD_PNG_NAME_CLOSE_NORM);
    hud->png.nameCloseHover   = LoadTextureSafe(HUD_PNG_NAME_CLOSE_HOVER);

    hud->png.pieceAlpha       = LoadTextureSafe(HUD_PNG_PIECE_ALPHA);
    hud->png.pieceOmega       = LoadTextureSafe(HUD_PNG_PIECE_OMEGA);
    hud->png.cellEmpty        = LoadTextureSafe(HUD_PNG_CELL_EMPTY);
    hud->png.cellHoverFrame   = LoadTextureSafe(HUD_PNG_CELL_HOVER_FRAME);
    hud->png.cellHoverOmega   = LoadTextureSafe(HUD_PNG_CELL_HOVER_OMEGA);
    hud->png.cellHoverAlpha   = LoadTextureSafe(HUD_PNG_CELL_HOVER_ALPHA);

    if (IsAudioDeviceReady()) {
        if (FileExists(HUD_BGM_GAMEPLAY)) {
            hud->bgm = LoadMusicStream(HUD_BGM_GAMEPLAY);
            hud->bgm.looping = true;
            hud->bgmLoaded = true;
        }
        if (FileExists(HUD_SFX_CELL_HOVER)) {
            hud->cellHoverSfx = LoadSound(HUD_SFX_CELL_HOVER);
            hud->cellHoverSfxLoaded = true;
        }
    }

    HUD_ResetBoardVisuals(hud);
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

    UnloadTextureSafe(&hud->png.pieceAlpha);
    UnloadTextureSafe(&hud->png.pieceOmega);
    UnloadTextureSafe(&hud->png.cellEmpty);
    UnloadTextureSafe(&hud->png.cellHoverFrame);
    UnloadTextureSafe(&hud->png.cellHoverOmega);
    UnloadTextureSafe(&hud->png.cellHoverAlpha);

    if (hud->cellHoverSfxLoaded) {
        StopSound(hud->cellHoverSfx);
        UnloadSound(hud->cellHoverSfx);
        hud->cellHoverSfxLoaded = false;
    }
    if (hud->bgmLoaded) {
        StopMusicStream(hud->bgm);
        UnloadMusicStream(hud->bgm);
        hud->bgmLoaded = false;
    }
}

void HUD_SetMatchState(GameHUD *hud,
                       int round,
                       int currentTurnPiece,
                       int p1Wins,
                       int p2Wins,
                       const char *p1Name,
                       const char *p2Name) {
    if (!hud) return;
    hud->round = (round > 0) ? round : 1;

    if (p1Wins < 0) p1Wins = 0;
    if (p1Wins > HUD_MAX_PIPS) p1Wins = HUD_MAX_PIPS;
    if (p2Wins < 0) p2Wins = 0;
    if (p2Wins > HUD_MAX_PIPS) p2Wins = HUD_MAX_PIPS;

    hud->p1.wins      = p1Wins;
    hud->p2.wins      = p2Wins;
    hud->alphaButtons = p1Wins;
    hud->omegaButtons = p2Wins;

    bool alphaReached = (hud->alphaButtons >= HUD_MAX_PIPS);
    bool omegaReached = (hud->omegaButtons >= HUD_MAX_PIPS);

    if (alphaReached && omegaReached) {
        hud->matchStopped  = true;
        hud->championPiece = 3;
        hud->matchStatus   = HUD_MATCH_GAMEOVER_DRAW;
    } else if (alphaReached) {
        hud->matchStopped  = true;
        hud->championPiece = 1;
        hud->matchStatus   = HUD_MATCH_GAMEOVER_ALPHA;
    } else if (omegaReached) {
        hud->matchStopped  = true;
        hud->championPiece = 2;
        hud->matchStatus   = HUD_MATCH_GAMEOVER_OMEGA;
    } else {
        hud->matchStopped  = false;
        hud->championPiece = 0;
        hud->matchStatus   = HUD_MATCH_ONGOING;
    }

    hud->p1.isTurn = (currentTurnPiece == 1);
    hud->p2.isTurn = (currentTurnPiece == 2);

    if (p1Name && p1Name[0] != '\0') {
        snprintf(hud->p1.name, sizeof(hud->p1.name), "%s", p1Name);
    }
    if (p2Name && p2Name[0] != '\0') {
        snprintf(hud->p2.name, sizeof(hud->p2.name), "%s", p2Name);
    }
}

void HUD_ResetMatchScore(GameHUD *hud) {
    if (!hud) return;
    hud->round         = 1;
    hud->p1.wins       = 0;
    hud->p2.wins       = 0;
    hud->alphaButtons  = 0;
    hud->omegaButtons  = 0;
    hud->matchStopped  = false;
    hud->championPiece = 0;
    hud->matchStatus   = HUD_MATCH_ONGOING;
    hud->p1.isTurn     = true;
    hud->p2.isTurn     = false;
    memset(hud->pipAnim, 0, sizeof(hud->pipAnim));
    HUD_ResetBoardVisuals(hud);
}

HudMatchStatus HUD_AccumulateButtonPoint(GameHUD *hud, int winnerPiece) {
    if (!hud) return HUD_MATCH_ONGOING;
    if (hud->matchStopped) return hud->matchStatus;

    if (winnerPiece == 1) {
        if (hud->alphaButtons < HUD_MAX_PIPS) {
            hud->alphaButtons++;
            hud->p1.wins = hud->alphaButtons;
        }
        if (hud->alphaButtons >= HUD_MAX_PIPS) {
            hud->matchStopped  = true;
            hud->championPiece = 1;
            hud->matchStatus   = HUD_MATCH_GAMEOVER_ALPHA;
            return hud->matchStatus;
        }
    } else if (winnerPiece == 2) {
        if (hud->omegaButtons < HUD_MAX_PIPS) {
            hud->omegaButtons++;
            hud->p2.wins = hud->omegaButtons;
        }
        if (hud->omegaButtons >= HUD_MAX_PIPS) {
            hud->matchStopped  = true;
            hud->championPiece = 2;
            hud->matchStatus   = HUD_MATCH_GAMEOVER_OMEGA;
            return hud->matchStatus;
        }
    } else {
        return hud->matchStatus;
    }

    hud->round++;
    int nextStarter = (hud->round % 2 == 1) ? 1 : 2;
    hud->p1.isTurn   = (nextStarter == 1);
    hud->p2.isTurn   = (nextStarter == 2);
    hud->matchStatus = HUD_MATCH_NEXT_ROUND;
    HUD_ResetBoardVisuals(hud);

    return hud->matchStatus;
}

HudMatchStatus HUD_OnRoundDraw(GameHUD *hud) {
    if (!hud) return HUD_MATCH_ONGOING;
    if (hud->matchStopped) return hud->matchStatus;

    if (hud->alphaButtons < HUD_MAX_PIPS) {
        hud->alphaButtons++;
        hud->p1.wins = hud->alphaButtons;
    }
    if (hud->omegaButtons < HUD_MAX_PIPS) {
        hud->omegaButtons++;
        hud->p2.wins = hud->omegaButtons;
    }

    bool alphaReached = (hud->alphaButtons >= HUD_MAX_PIPS);
    bool omegaReached = (hud->omegaButtons >= HUD_MAX_PIPS);

    if (alphaReached && omegaReached) {
        hud->matchStopped  = true;
        hud->championPiece = 3;
        hud->matchStatus   = HUD_MATCH_GAMEOVER_DRAW;
        return hud->matchStatus;
    }

    if (alphaReached) {
        hud->matchStopped  = true;
        hud->championPiece = 1;
        hud->matchStatus   = HUD_MATCH_GAMEOVER_ALPHA;
        return hud->matchStatus;
    }

    if (omegaReached) {
        hud->matchStopped  = true;
        hud->championPiece = 2;
        hud->matchStatus   = HUD_MATCH_GAMEOVER_OMEGA;
        return hud->matchStatus;
    }

    hud->round++;
    int nextStarter  = (hud->round % 2 == 1) ? 1 : 2;
    hud->p1.isTurn   = (nextStarter == 1);
    hud->p2.isTurn   = (nextStarter == 2);
    hud->matchStatus = HUD_MATCH_NEXT_ROUND;
    HUD_ResetBoardVisuals(hud);

    return hud->matchStatus;
}

bool HUD_IsMatchStopped(const GameHUD *hud) {
    if (!hud) return false;
    return hud->matchStopped;
}

HudMatchStatus HUD_GetMatchStatus(const GameHUD *hud) {
    if (!hud) return HUD_MATCH_ONGOING;
    return hud->matchStatus;
}

int HUD_GetChampionPiece(const GameHUD *hud) {
    if (!hud) return 0;
    return hud->championPiece;
}

void HUD_SetSelectedAgents(GameHUD *hud, HudAgentId alphaAgent, HudAgentId omegaAgent) {
    if (!hud) return;
    hud->p1.selectedAgent = alphaAgent;
    hud->p2.selectedAgent = omegaAgent;
}

void HUD_SetTurnPngVisible(GameHUD *hud, bool visible) {
    if (!hud) return;
    hud->showTurnPng = visible;
}

void HUD_StartMusic(GameHUD *hud) {
    if (!hud || !hud->bgmLoaded) return;
    SeekMusicStream(hud->bgm, 0.0f);
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
        SetMusicVolume(hud->bgm, hud->bgmBaseVolume);
    }
}

void HUD_Update(GameHUD *hud, float dt) {
    if (!hud) return;

    if (hud->bgmLoaded && hud->bgmPlaying) {
        UpdateMusicStream(hud->bgm);
        float totalLen = GetMusicTimeLength(hud->bgm);
        float played   = GetMusicTimePlayed(hud->bgm);
        if (!IsMusicStreamPlaying(hud->bgm) ||
            (totalLen > 0.0f && played >= totalLen - 0.05f)) {
            StopMusicStream(hud->bgm);
            SeekMusicStream(hud->bgm, 0.0f);
            PlayMusicStream(hud->bgm);
        }
    }

    if (hud->saveNoticeTimer > 0.0f) {
        hud->saveNoticeTimer -= dt;
        if (hud->saveNoticeTimer < 0.0f) hud->saveNoticeTimer = 0.0f;
    }

    if (hud->p1.isTurn != hud->prevAlphaTurn) {
        hud->prevAlphaTurn = hud->p1.isTurn;
        hud->switchAnim = 1.0f;
    }
    if (hud->switchAnim > 0.0f) {
        hud->switchAnim -= dt * 3.0f;
        if (hud->switchAnim < 0.0f) hud->switchAnim = 0.0f;
    }

    float targetBlend = hud->p1.isTurn ? 1.0f : 0.0f;
    hud->turnBlend += (targetBlend - hud->turnBlend) * Clamp01(dt * 10.0f);

    for (int i = 0; i < HUD_MAX_PIPS; i++) {
        float t1 = (i < hud->alphaButtons) ? 1.0f : 0.0f;
        float t2 = (i < hud->omegaButtons) ? 1.0f : 0.0f;
        hud->pipAnim[0][i] += (t1 - hud->pipAnim[0][i]) * Clamp01(dt * 12.0f);
        hud->pipAnim[1][i] += (t2 - hud->pipAnim[1][i]) * Clamp01(dt * 12.0f);
    }

    if (hud->isPaused) {
        hud->pauseOpenAnim += dt * 4.0f;
        if (hud->pauseOpenAnim > 1.0f) hud->pauseOpenAnim = 1.0f;
    } else {
        hud->pauseOpenAnim = 0.0f;
    }

    if (hud->isSaveNaming) {
        hud->nameOpenAnim += dt * 4.0f;
        if (hud->nameOpenAnim > 1.0f) hud->nameOpenAnim = 1.0f;
        hud->cursorBlinkTimer += dt;
        if (hud->cursorBlinkTimer >= 1.0f) hud->cursorBlinkTimer -= 1.0f;
    } else {
        hud->nameOpenAnim = 0.0f;
    }
}

void HUD_SetPaused(GameHUD *hud, bool paused) {
    if (!hud) return;
    hud->isPaused = paused;
    if (!paused) {
        hud->isSaveNaming = false;
    }
}

HudPauseAction HUD_HandlePauseInput(GameHUD *hud) {
    if (!hud) return HUD_PAUSE_ACTION_NONE;
    Vector2 mouse = GetMousePosition();

    hud->hoverPauseIcon = CheckCollisionPointRec(mouse, hud->pauseIconRect);
    if (!hud->isPaused && !hud->isSaveNaming) {
        if ((hud->hoverPauseIcon && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ESCAPE)) {
            HUD_SetPaused(hud, true);
        }
        return HUD_PAUSE_ACTION_NONE;
    }

    if (hud->isSaveNaming) {
        int key = GetCharPressed();
        size_t len = strlen(hud->saveNameInput);
        while (key > 0) {
            if (key >= 32 && key <= 125 && len < HUD_SAVE_NAME_MAX_LEN - 1) {
                hud->saveNameInput[len++] = (char)key;
                hud->saveNameInput[len]   = '\0';
                hud->emptyNameWarning     = false;
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && len > 0) {
            hud->saveNameInput[len - 1] = '\0';
        }

        hud->hoverNameCloseBtn   = CheckCollisionPointRec(mouse, hud->nameCloseBtnRect);
        hud->hoverNameConfirmBtn = CheckCollisionPointRec(mouse, hud->nameConfirmBtnRect);
        hud->pressNameConfirmBtn = ui_press_check: (hud->hoverNameConfirmBtn && IsMouseButtonDown(MOUSE_BUTTON_LEFT));

        if ((hud->hoverNameCloseBtn && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ESCAPE)) {
            hud->isSaveNaming = false;
            return HUD_PAUSE_ACTION_NONE;
        }

        if ((hud->hoverNameConfirmBtn && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ENTER)) {
            if (hud->saveNameInput[0] == '\0') {
                hud->emptyNameWarning = true;
                return HUD_PAUSE_ACTION_NONE;
            }
            hud->isSaveNaming    = false;
            hud->isPaused        = false;
            hud->saveNoticeTimer = 2.0f;
            return HUD_PAUSE_ACTION_SAVE_CONFIRMED;
        }
        return HUD_PAUSE_ACTION_NONE;
    }

    hud->hoverCloseBtn = CheckCollisionPointRec(mouse, hud->closeBtnRect);
    hud->hoverSaveBtn  = CheckCollisionPointRec(mouse, hud->saveBtnRect);
    hud->hoverExitBtn  = CheckCollisionPointRec(mouse, hud->exitBtnRect);
    hud->pressSaveBtn  = hud->hoverSaveBtn && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    hud->pressExitBtn  = hud->hoverExitBtn && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    if ((hud->hoverCloseBtn && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ESCAPE)) {
        HUD_SetPaused(hud, false);
        return HUD_PAUSE_ACTION_RESUME;
    }
    if (hud->hoverSaveBtn && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        hud->isSaveNaming        = true;
        hud->emptyNameWarning    = false;
        hud->saveNameInput[0]    = '\0';
        return HUD_PAUSE_ACTION_NONE;
    }
    if (hud->hoverExitBtn && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        HUD_SetPaused(hud, false);
        return HUD_PAUSE_ACTION_EXIT;
    }

    return HUD_PAUSE_ACTION_NONE;
}

const char *HUD_GetSaveName(const GameHUD *hud) {
    if (!hud) return "";
    return hud->saveNameInput;
}

static void DrawPlayerCard(const GameHUD *hud, Rectangle panel, const HudPlayerInfo *p, int playerIdx, Color accent) {
    DrawRectangleRec(panel, HUD_PANEL_DARK);
    DrawRectangleLinesEx(panel, p->isTurn ? 2.5f : 1.0f, p->isTurn ? accent : Fade(WHITE, 0.25f));

    float avSize = panel.height * 0.44f;
    Rectangle avRect = { panel.x + 14.0f, panel.y + 14.0f, avSize, avSize };
    if (p->selectedAgent > HUD_AGENT_NONE && p->selectedAgent < HUD_AGENT_COUNT &&
        hud->png.agentAvatar[p->selectedAgent].id > 0) {
        DrawTextureFit(hud->png.agentAvatar[p->selectedAgent], avRect, WHITE);
    }

    int nameSize = 20;
    DrawText(p->name, (int)(avRect.x + avSize + 12.0f), (int)(panel.y + 18.0f), nameSize, accent);

    float pipSize = fminf(26.0f, (panel.width - 28.0f) / (float)HUD_MAX_PIPS - 6.0f);
    float pipY    = panel.y + panel.height - pipSize - 16.0f;
    Texture2D filledPip = (playerIdx == 0) ? hud->png.pipAlpha : hud->png.pipOmega;

    for (int i = 0; i < HUD_MAX_PIPS; i++) {
        Rectangle pipRect = { panel.x + 14.0f + i * (pipSize + 6.0f), pipY, pipSize, pipSize };
        if (hud->png.pipEmpty.id > 0) {
            DrawTextureFit(hud->png.pipEmpty, pipRect, WHITE);
        }
        float anim = hud->pipAnim[playerIdx][i];
        if (anim > 0.01f && filledPip.id > 0) {
            DrawTextureFit(filledPip, pipRect, Fade(WHITE, anim));
        }
    }
}

void HUD_Draw(const GameHUD *hud) {
    if (!hud) return;

    DrawPlayerCard(hud, hud->alphaPanel, &hud->p1, 0, HUD_ACCENT_MINT);
    DrawPlayerCard(hud, hud->omegaPanel, &hud->p2, 1, HUD_ACCENT_