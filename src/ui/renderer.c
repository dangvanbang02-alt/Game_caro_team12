#include "Renderer.h"
#include <math.h>
#include <stdio.h>

static float Clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static float LerpFloat(float a, float b, float t) {
    return a + (b - a) * t;
}

static bool IsInWinLine(int r, int c, const BoardCoord winLine[5], int winCount) {
    if (!winLine || winCount <= 0) return false;
    int limit = (winCount < 5) ? winCount : 5;
    for (int i = 0; i < limit; i++) {
        if (winLine[i].valid && winLine[i].row == r && winLine[i].col == c) {
            return true;
        }
    }
    return false;
}

static void DrawPieceX(float cx, float cy, float radius, Color color, float scale) {
    float r = radius * scale;
    float thick = fmaxf(2.5f, radius * 0.22f);
    DrawLineEx((Vector2){ cx - r, cy - r }, (Vector2){ cx + r, cy + r }, thick, color);
    DrawLineEx((Vector2){ cx + r, cy - r }, (Vector2){ cx - r, cy + r }, thick, color);
}

static void DrawPieceO(float cx, float cy, float radius, Color color, float scale) {
    float r = radius * scale;
    float thick = fmaxf(2.5f, radius * 0.22f);
    DrawRing((Vector2){ cx, cy }, fmaxf(1.0f, r - thick * 0.5f), r + thick * 0.5f, 0.0f, 360.0f, 36, color);
}

void Renderer_Init(BoardRenderer *ren, int rows, int cols, int screenWidth, int screenHeight) {
    if (!ren) return;

    ren->rows = (rows > 0 && rows <= RENDERER_MAX_BOARD) ? rows : RENDERER_DEFAULT_ROWS;
    ren->cols = (cols > 0 && cols <= RENDERER_MAX_BOARD) ? cols : RENDERER_DEFAULT_COLS;

    for (int r = 0; r < RENDERER_MAX_BOARD; r++) {
        for (int c = 0; c < RENDERER_MAX_BOARD; c++) {
            ren->popAnim[r][c] = 0.0f;
        }
    }

    Renderer_Resize(ren, screenWidth, screenHeight);
}

void Renderer_Resize(BoardRenderer *ren, int screenWidth, int screenHeight) {
    if (!ren) return;
    if (ren->rows <= 0) ren->rows = RENDERER_DEFAULT_ROWS;
    if (ren->cols <= 0) ren->cols = RENDERER_DEFAULT_COLS;

    float sw = (float)((screenWidth > 0) ? screenWidth : 1280);
    float sh = (float)((screenHeight > 0) ? screenHeight : 720);

    // Chừa 2 bên cho HUD người chơi trái/phải và phía dưới cho thanh ROUND
    float sideMargin = sw * 0.23f;
    float topMargin = sh * 0.03f;
    float bottomHudH = sh * 0.09f;

    float panelW = sw - sideMargin * 2.0f;
    if (panelW < 240.0f) panelW = sw * 0.5f;
    float panelH = sh - topMargin - bottomHudH - sh * 0.02f;
    if (panelH < 240.0f) panelH = sh * 0.75f;

    float panelX = (sw - panelW) * 0.5f;
    float panelY = topMargin;
    ren->panelRect = (Rectangle){ panelX, panelY, panelW, panelH };

    // Dành chỗ cho tiêu đề GAME BOARD phía trên và dòng kích thước phía dưới
    float headerH = fmaxf(34.0f, panelH * 0.075f);
    float footerH = fmaxf(28.0f, panelH * 0.06f);
    float padX = panelW * 0.05f;

    float availW = panelW - padX * 2.0f;
    float availH = panelH - headerH - footerH;

    ren->cellSize = floorf(fminf(availW / (float)ren->cols, availH / (float)ren->rows));
    if (ren->cellSize < 12.0f) ren->cellSize = 12.0f;

    float boardW = ren->cellSize * (float)ren->cols;
    float boardH = ren->cellSize * (float)ren->rows;

    float boardX = panelX + (panelW - boardW) * 0.5f;
    float boardY = panelY + headerH + (availH - boardH) * 0.5f;
    ren->boardRect = (Rectangle){ boardX, boardY, boardW, boardH };
}

BoardCoord Renderer_ScreenToBoard(const BoardRenderer *ren, Vector2 mousePos) {
    BoardCoord res = { -1, -1, false };
    if (!ren || ren->cellSize <= 0.0f) return res;

    if (!CheckCollisionPointRec(mousePos, ren->boardRect)) {
        return res;
    }

    int col = (int)((mousePos.x - ren->boardRect.x) / ren->cellSize);
    int row = (int)((mousePos.y - ren->boardRect.y) / ren->cellSize);

    if (row >= 0 && row < ren->rows && col >= 0 && col < ren->cols) {
        res.row = row;
        res.col = col;
        res.valid = true;
    }
    return res;
}

void Renderer_Update(BoardRenderer *ren,
                     const int board[RENDERER_MAX_BOARD][RENDERER_MAX_BOARD],
                     float dt) {
    if (!ren || !board) return;
    if (dt <= 0.0f) dt = 0.016f;

    float speed = 14.0f;
    for (int r = 0; r < ren->rows; r++) {
        for (int c = 0; c < ren->cols; c++) {
            if (board[r][c] == CELL_EMPTY) {
                ren->popAnim[r][c] = 0.0f;
            } else {
                ren->popAnim[r][c] = Clamp01(
                    LerpFloat(ren->popAnim[r][c], 1.0f, Clamp01(speed * dt))
                );
            }
        }
    }
}

void Renderer_Draw(const BoardRenderer *ren,
                   const int board[RENDERER_MAX_BOARD][RENDERER_MAX_BOARD],
                   BoardCoord cursor,
                   BoardCoord lastMove,
                   const BoardCoord winLine[5],
                   int winCount) {
    if (!ren || !board) return;

    float x0 = ren->boardRect.x;
    float y0 = ren->boardRect.y;
    float s = ren->cellSize;

    // 1. Khung giữa (GAME BOARD) + đường nhấn đỏ phong cách Valorant
    DrawRectangleRec(ren->panelRect, VAL_COLOR_PANEL);
    DrawRectangleLinesEx(ren->panelRect, 1.5f, VAL_COLOR_GRID);
    DrawRectangle((int)ren->panelRect.x, (int)ren->panelRect.y, (int)ren->panelRect.width, 3, VAL_COLOR_RED);

    const char *title = "GAME BOARD";
    int titleSize = 20;
    int titleW = MeasureText(title, titleSize);
    DrawText(
        title,
        (int)(ren->panelRect.x + (ren->panelRect.width - titleW) * 0.5f),
        (int)(ren->panelRect.y + 10.0f),
        titleSize,
        VAL_COLOR_CREAM
    );

    // 2. Vẽ từng ô trên lưới
    for (int r = 0; r < ren->rows; r++) {
        for (int c = 0; c < ren->cols; c++) {
            Rectangle cellRect = { x0 + c * s, y0 + r * s, s, s };
            bool isWin = IsInWinLine(r, c, winLine, winCount);

            Color cellBg = isWin ? Fade(VAL_COLOR_RED, 0.25f) : VAL_COLOR_CELL;
            DrawRectangleRec(cellRect, cellBg);
            DrawRectangleLinesEx(cellRect, isWin ? 1.5f : 1.0f, isWin ? VAL_COLOR_RED : VAL_COLOR_GRID);
        }
    }

    // 3. Đánh dấu nước đi vừa đặt
    if (lastMove.valid && lastMove.row >= 0 && lastMove.row < ren->rows &&
        lastMove.col >= 0 && lastMove.col < ren->cols) {
        Rectangle lastRect = {
            x0 + lastMove.col * s + 2.0f,
            y0 + lastMove.row * s + 2.0f,
            s - 4.0f,
            s - 4.0f
        };
        DrawRectangleLinesEx(lastRect, 1.5f, Fade(VAL_COLOR_CREAM, 0.75f));
    }

    // 4. Highlight ô đang chọn / hover
    if (cursor.valid && cursor.row >= 0 && cursor.row < ren->rows &&
        cursor.col >= 0 && cursor.col < ren->cols) {
        Rectangle hoverRect = {
            x0 + cursor.col * s,
            y0 + cursor.row * s,
            s,
            s
        };
        DrawRectangleRec(hoverRect, Fade(VAL_COLOR_CREAM, 0.12f));
        DrawRectangleLinesEx(hoverRect, 2.0f, VAL_COLOR_RED);
    }

    // 5. Vẽ quân X và O
    float radius = s * 0.26f;
    for (int r = 0; r < ren->rows; r++) {
        for (int c = 0; c < ren->cols; c++) {
            int cell = board[r][c];
            if (cell == CELL_EMPTY) continue;

            float cx = x0 + (c + 0.5f) * s;
            float cy = y0 + (r + 0.5f) * s;
            float scale = 0.65f + 0.35f * ren->popAnim[r][c];

            if (cell == CELL_X) {
                DrawPieceX(cx, cy, radius, VAL_COLOR_BLUE, scale);
            } else if (cell == CELL_O) {
                DrawPieceO(cx, cy, radius, VAL_COLOR_CORAL, scale);
            }
        }
    }

    // 6. Nhãn kích thước bàn cờ ở dưới (ví dụ: 12 x 12 board)
    char infoText[64];
    snprintf(infoText, sizeof(infoText), "%d x %d board", ren->rows, ren->cols);
    int infoSize = 15;
    int infoW = MeasureText(infoText, infoSize);
    float infoY = ren->boardRect.y + ren->boardRect.height + 8.0f;
    float maxInfoY = ren->panelRect.y + ren->panelRect.height - infoSize - 6.0f;
    if (infoY > maxInfoY) infoY = maxInfoY;

    DrawText(
        infoText,
        (int)(ren->panelRect.x + (ren->panelRect.width - infoW) * 0.5f),
        (int)infoY,
        infoSize,
        VAL_COLOR_MUTED
    );
}