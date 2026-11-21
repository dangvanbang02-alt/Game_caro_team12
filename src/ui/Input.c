#include "Input.h"

void Input_Init(InputState *input) {
    if (!input) return;

    input->mousePos = (Vector2){ 0.0f, 0.0f };
    input->mouseDelta = (Vector2){ 0.0f, 0.0f };
    input->leftPressed = false;
    input->leftReleased = false;
    input->rightPressed = false;
    input->hoveredCell = (BoardCoord){ -1, -1, false };
    input->clickedCell = (BoardCoord){ -1, -1, false };
    input->command = INPUT_CMD_NONE;
}

void Input_Update(InputState *input, const BoardRenderer *renderer) {
    if (!input) return;

    input->mousePos = GetMousePosition();
    input->mouseDelta = GetMouseDelta();
    input->leftPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    input->leftReleased = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    input->rightPressed = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    input->clickedCell = (BoardCoord){ -1, -1, false };
    input->command = INPUT_CMD_NONE;

    if (renderer != NULL) {
        input->hoveredCell = Renderer_ScreenToBoard(renderer, input->mousePos);
        if (input->leftPressed && input->hoveredCell.valid) {
            input->clickedCell = input->hoveredCell;
            input->command = INPUT_CMD_CONFIRM;
            return;
        }
    } else {
        input->hoveredCell = (BoardCoord){ -1, -1, false };
    }

    // Điều hướng bàn phím
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        input->command = INPUT_CMD_MOVE_UP;
    } else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        input->command = INPUT_CMD_MOVE_DOWN;
    } else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        input->command = INPUT_CMD_MOVE_LEFT;
    } else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        input->command = INPUT_CMD_MOVE_RIGHT;
    } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        input->command = INPUT_CMD_CONFIRM;
    } else if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
        input->command = INPUT_CMD_BACK;
    } else if (IsKeyPressed(KEY_F11)) {
        input->command = INPUT_CMD_TOGGLE_BORDERLESS;
    }
}