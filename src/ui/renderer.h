#ifndef RENDERER_H
#define RENDERER_H

#include "raylib.h"
#include <stdbool.h>

#define RENDERER_DEFAULT_ROWS  12
#define RENDERER_DEFAULT_COLS  12
#define RENDERER_MAX_BOARD     30

// Bảng màu phong cách Valorant
#define VAL_COLOR_BG           (Color){ 15, 25, 35, 255 }    
#define VAL_COLOR_PANEL        (Color){ 24, 34, 46, 255 }   
#define VAL_COLOR_CELL         (Color){ 34, 47, 63, 255 }    
#define VAL_COLOR_GRID         (Color){ 64, 82, 102, 255 }   
#define VAL_COLOR_RED          (Color){ 255, 70, 85, 255 }  
#define VAL_COLOR_BLUE         (Color){ 56, 189, 248, 255 }  
#define VAL_COLOR_CORAL        (Color){ 248, 113, 113, 255 }  
#define VAL_COLOR_CREAM        (Color){ 236, 232, 225, 255 } 
#define VAL_COLOR_MUTED        (Color){ 148, 163, 184, 255 } 

typedef enum {
    CELL_EMPTY = 0,
    CELL_X = 1,
    CELL_O = 2
} CellState;

typedef struct {
    int row;
    int col;
    bool valid;
} BoardCoord;

typedef struct {
    int rows;
    int cols;
    Rectangle panelRect;                                  
    Rectangle boardRect;                                 
    float cellSize;                                      
    float popAnim[RENDERER_MAX_BOARD][RENDERER_MAX_BOARD]; 
} BoardRenderer;

void Renderer_Init(BoardRenderer *ren, int rows, int cols, int screenWidth, int screenHeight);
void Renderer_Resize(BoardRenderer *ren, int screenWidth, int screenHeight);
BoardCoord Renderer_ScreenToBoard(const BoardRenderer *ren, Vector2 mousePos);
void Renderer_Update(BoardRenderer *ren,
                     const int board[RENDERER_MAX_BOARD][RENDERER_MAX_BOARD],
                     float dt);
void Renderer_Draw(const BoardRenderer *ren,
                   const int board[RENDERER_MAX_BOARD][RENDERER_MAX_BOARD],
                   BoardCoord cursor,
                   BoardCoord lastMove,
                   const BoardCoord winLine[5],
                   int winCount);

#endif