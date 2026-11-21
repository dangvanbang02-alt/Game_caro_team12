#ifndef INPUT_H
#define INPUT_H

#include "raylib.h"
#include "Renderer.h"
#include <stdbool.h>

typedef enum {
    INPUT_CMD_NONE = 0,
    INPUT_CMD_MOVE_UP,
    INPUT_CMD_MOVE_DOWN,
    INPUT_CMD_MOVE_LEFT,
    INPUT_CMD_MOVE_RIGHT,
    INPUT_CMD_CONFIRM,
    INPUT_CMD_BACK,
    INPUT_CMD_TOGGLE_BORDERLESS
} InputCommand;

typedef struct {
    Vector2 mousePos;
    Vector2 mouseDelta;
    bool leftPressed;
    bool leftReleased;
    bool rightPressed;
    BoardCoord hoveredCell;
    BoardCoord clickedCell;
    InputCommand command;
} InputState;

void Input_Init(InputState *input);
void Input_Update(InputState *input, const BoardRenderer *renderer);

#endif