#ifndef WINDOW_H
#define WINDOW_H

#include "raylib.h"
#include <stdbool.h>

#define WINDOW_DEFAULT_WIDTH   1280
#define WINDOW_DEFAULT_HEIGHT  720
#define WINDOW_DEFAULT_FPS     60
#define WINDOW_BG_COLOR        (Color){ 15, 25, 35, 255 }
#define WINDOW_MAX_DELTA_TIME  0.1f

typedef struct {
    int width;
    int height;
    int targetFps;
    const char *title;
    bool borderless;
    bool audioReady;
    bool initialized;
    bool resized;
    float deltaTime;
} GameWindow;

bool Window_Init(GameWindow *win, int width, int height, int targetFps, const char *title);
void Window_Update(GameWindow *win);
void Window_BeginFrame(const GameWindow *win);
void Window_EndFrame(void);
void Window_ToggleBorderless(GameWindow *win);
Vector2 Window_GetMousePosition(const GameWindow *win);
bool Window_ShouldClose(const GameWindow *win);
void Window_Close(GameWindow *win);

#endif