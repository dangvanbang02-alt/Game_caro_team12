#include "Window.h"

static void SyncWindowSize(GameWindow *win, bool markResized) {
    if (!win || !win->initialized) return;

    int nw = GetScreenWidth();
    int nh = GetScreenHeight();
    if (nw > 0 && nh > 0 && (nw != win->width || nh != win->height)) {
        win->width = nw;
        win->height = nh;
        if (markResized) {
            win->resized = true;
        }
    }
}

bool Window_Init(GameWindow *win, int width, int height, int targetFps, const char *title) {
    if (!win) return false;

    win->width = (width > 0) ? width : WINDOW_DEFAULT_WIDTH;
    win->height = (height > 0) ? height : WINDOW_DEFAULT_HEIGHT;
    win->targetFps = (targetFps > 0) ? targetFps : WINDOW_DEFAULT_FPS;
    win->title = (title != NULL) ? title : "VALORANT CARO";
    win->borderless = false;
    win->audioReady = false;
    win->initialized = false;
    win->resized = false;
    win->deltaTime = 0.0f;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(win->width, win->height, win->title);

    if (!IsWindowReady()) {
        return false;
    }

    SetTargetFPS(win->targetFps);

    InitAudioDevice();
    win->audioReady = IsAudioDeviceReady();
    win->initialized = true;
    win->width = GetScreenWidth();
    win->height = GetScreenHeight();
    return true;
}

void Window_Update(GameWindow *win) {
    if (!win || !win->initialized) return;

    win->resized = false;

    float dt = GetFrameTime();
    if (dt <= 0.0f) {
        dt = 1.0f / (float)win->targetFps;
    } else if (dt > WINDOW_MAX_DELTA_TIME) {
        dt = WINDOW_MAX_DELTA_TIME;
    }
    win->deltaTime = dt;

    if (IsWindowResized() && !win->borderless) {
        SyncWindowSize(win, true);
    }
}

void Window_BeginFrame(const GameWindow *win) {
    BeginDrawing();
    Color bg = (win != NULL) ? WINDOW_BG_COLOR : BLACK;
    ClearBackground(bg);
}

void Window_EndFrame(void) {
    EndDrawing();
}

void Window_ToggleBorderless(GameWindow *win) {
    if (!win || !win->initialized) return;

    if (!win->borderless) {
        SetWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
        win->borderless = true;
    } else {
        ClearWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
        SetWindowSize(WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT);
        win->width = WINDOW_DEFAULT_WIDTH;
        win->height = WINDOW_DEFAULT_HEIGHT;
        win->borderless = false;
    }

    SyncWindowSize(win, false);
    win->resized = true;
}

Vector2 Window_GetMousePosition(const GameWindow *win) {
    (void)win;
    return GetMousePosition();
}

bool Window_ShouldClose(const GameWindow *win) {
    if (!win || !win->initialized) return true;
    return WindowShouldClose();
}

void Window_Close(GameWindow *win) {
    if (!win) return;

    if (win->audioReady) {
        CloseAudioDevice();
        win->audioReady = false;
    }

    if (win->initialized) {
        CloseWindow();
        win->initialized = false;
    }

    win->resized = false;
    win->deltaTime = 0.0f;
}