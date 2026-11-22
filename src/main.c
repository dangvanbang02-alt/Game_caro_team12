#include "raylib/include/raylib.h"
#include "Core/Game/Game.h"
#include "Core/History/MatchHistory.h"
#include "Core/History/MoveHistory.h"
#include "Core/GameTimer/GameTimer.h"
#include "Core/Board/Board.h"
#include "Core/Player/Player.h"
#include "Core/Stats/Stats.h"
#include "ui/menu.h"
#include "ui/Play.h"
#include "ui/PVP.h"
#include "ui/PVE.h"
#include "ui/HUD.h"
#include "ui/Settings.h"
#include "ui/Load.h"
#include "ui/GameOver.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE "VALORANT CARO"
typedef enum AppScreen {
    SCREEN_MENU = 0,
    SCREEN_PLAY,
    SCREEN_PVP_SETUP,
    SCREEN_PVE_SETUP,
    SCREEN_GAME,
    SCREEN_SETTINGS,
    SCREEN_LOAD,
    SCREEN_GAME_OVER
} AppScreen;

typedef struct App {
    AppScreen screen;

    MenuUI menu;
    PlayUI play;
    PvpUI pvp;
    PveUI pve;
    GameHUD hud;
    SettingsUI settings;
    LoadUI load;
    GameOverUI gameOver;

    Game game;
    bool gameInitialized;
    bool isPve;
    int humanPiece;          /* 1 = ALPHA/X, 2 = OMEGA/O */
    int roundNumber;
    bool roundProcessed;
    float botThinkDelay;
} App;

static void CopyText(char* destination, size_t capacity,
    const char* source, const char* fallback)
{
    const char* text = (source && source[0] != '\0') ? source : fallback;
    if (!destination || capacity == 0) return;
    snprintf(destination, capacity, "%s", text ? text : "");
}

static int CurrentHudPiece(const App* app)
{
    return app->game.currentPlayer == CELL_X ? 1 : 2;
}

static void BuildHudBoard(const App* app,
    int output[HUD_MAX_BOARD_DIM][HUD_MAX_BOARD_DIM])
{
    int row, col;

    memset(output, 0, sizeof(int) * HUD_MAX_BOARD_DIM * HUD_MAX_BOARD_DIM);

    for (row = 0; row < BOARD_SIZE; ++row) {
        for (col = 0; col < BOARD_SIZE; ++col) {
            if (app->game.board.cells[row][col] == CELL_X) {
                output[row][col] = 1;
            }
            else if (app->game.board.cells[row][col] == CELL_O) {
                output[row][col] = 2;
            }
        }
    }
}

static void SyncHudState(App* app)
{
    if (!app->gameInitialized) return;

    HUD_SetMatchState(&app->hud,
        app->roundNumber,
        CurrentHudPiece(app),
        app->hud.alphaButtons,
        app->hud.omegaButtons,
        app->game.players[0].name,
        app->game.players[1].name);
}

static Rectangle GetBoardRectangle(void)
{
    float side = (float)GetScreenHeight() * 0.76f;
    float maxSide = (float)GetScreenWidth() * 0.48f;

    if (side > maxSide) side = maxSide;
    if (side > 620.0f) side = 620.0f;

    return (Rectangle) {
        ((float)GetScreenWidth() - side) * 0.5f,
            ((float)GetScreenHeight() - side) * 0.5f,
            side,
            side
    };
}

static void StartMatch(App* app,
    bool isPve,
    const char* alphaName,
    const char* omegaName,
    HudAgentId alphaAgent,
    HudAgentId omegaAgent,
    int humanPiece,
    int aiDepth)
{
    GameConfig config;

    app->isPve = isPve;
    app->humanPiece = humanPiece;
    app->roundNumber = 1;
    app->roundProcessed = false;
    app->botThinkDelay = 0.25f;

    Game_init(&app->game,
        isPve ? GAME_MODE_PVE : GAME_MODE_PVP,
        aiDepth >= 4 ? AI_DIFFICULTY_HARD : AI_DIFFICULTY_MEDIUM);

    config = app->game.config;
    config.aiDepth = aiDepth;
    Game_setConfig(&app->game, &config);

    
    app->game.aiPlayer = (humanPiece == 1) ? CELL_O : CELL_X;

    Game_setPlayerName(&app->game, CELL_X, alphaName);
    Game_setPlayerName(&app->game, CELL_O, omegaName);
    app->gameInitialized = true;

    HUD_ResetMatchScore(&app->hud);
    HUD_SetMatchState(&app->hud, 1, 1, 0, 0,
        app->game.players[0].name,
        app->game.players[1].name);
    HUD_SetSelectedAgents(&app->hud, alphaAgent, omegaAgent);
    HUD_SetTurnPngVisible(&app->hud, true);
    HUD_SetPaused(&app->hud, false);
    HUD_ResetBoardVisuals(&app->hud);

    Menu_StopMusic(&app->menu);
    Play_StopMusic(&app->play);
    GameOver_StopMusic(&app->gameOver);
    HUD_StartMusic(&app->hud);

    app->screen = SCREEN_GAME;
}

static void StartPvpMatch(App* app)
{
    const PvpSetupResult* setup = Pvp_GetResult(&app->pvp);
    if (!setup) return;

    StartMatch(app, false,
        setup->alphaName, setup->omegaName,
        setup->alphaAgent, setup->omegaAgent,
        1, 2);
}

static void StartPveMatch(App* app)
{
    const PveSetupResult* setup = Pve_GetResult(&app->pve);
    int depth;

    if (!setup) return;

    switch (setup->difficulty) {
    case PVE_DIFF_EASY:   depth = 1; break;
    case PVE_DIFF_HARD:   depth = 4; break;
    case PVE_DIFF_MEDIUM:
    default:              depth = 2; break;
    }

    StartMatch(app, true,
        setup->alphaName, setup->omegaName,
        setup->alphaAgent, setup->omegaAgent,
        setup->playerSide == PVE_SIDE_ALPHA ? 1 : 2,
        depth);
}

static void ReturnToMenu(App* app)
{
    HUD_StopMusic(&app->hud);
    Play_StopMusic(&app->play);
    GameOver_StopMusic(&app->gameOver);
    Menu_StartMusic(&app->menu);
    app->screen = SCREEN_MENU;
}

static void OpenGameOver(App* app)
{
    HUD_StopMusic(&app->hud);
    GameOver_Open(&app->gameOver, &app->hud,
        app->isPve, app->humanPiece);
    app->screen = SCREEN_GAME_OVER;
}

static void CompleteRound(App* app)
{
    HudMatchStatus matchStatus;
    int winningPiece = 0;

    if (!app->gameInitialized || app->roundProcessed ||
        !Game_isFinished(&app->game)) {
        return;
    }

    app->roundProcessed = true;

    if (app->game.result == GAME_RESULT_X_WINS) {
        winningPiece = 1;
        matchStatus = HUD_AccumulateButtonPoint(&app->hud, winningPiece);
    }
    else if (app->game.result == GAME_RESULT_O_WINS) {
        winningPiece = 2;
        matchStatus = HUD_AccumulateButtonPoint(&app->hud, winningPiece);
    }
    else {
        matchStatus = HUD_OnRoundDraw(&app->hud);
    }

    if (matchStatus == HUD_MATCH_GAMEOVER_ALPHA ||
        matchStatus == HUD_MATCH_GAMEOVER_OMEGA ||
        matchStatus == HUD_MATCH_GAMEOVER_DRAW) {
        OpenGameOver(app);
        return;
    }

   
    ++app->roundNumber;
    Game_resetRound(&app->game);
    if ((app->roundNumber % 2) == 0) {
        app->game.currentPlayer = CELL_O;
    }
    else {
        app->game.currentPlayer = CELL_X;
    }

    app->roundProcessed = false;
    app->botThinkDelay = 0.25f;
    HUD_ResetBoardVisuals(&app->hud);
    HUD_SetMatchState(&app->hud,
        app->roundNumber,
        CurrentHudPiece(app),
        app->hud.alphaButtons,
        app->hud.omegaButtons,
        app->game.players[0].name,
        app->game.players[1].name);
}

static void DrawGameBoard(App* app)
{
    Rectangle boardRect = GetBoardRectangle();
    float cellWidth = boardRect.width / (float)BOARD_SIZE;
    float cellHeight = boardRect.height / (float)BOARD_SIZE;
    int hudBoard[HUD_MAX_BOARD_DIM][HUD_MAX_BOARD_DIM];
    int clickedRow = -1;
    int clickedCol = -1;
    bool canHumanMove;

    BuildHudBoard(app, hudBoard);

    canHumanMove = !app->hud.isPaused &&
        !app->hud.isSaveNaming &&
        !HUD_IsMatchStopped(&app->hud) &&
        (!app->isPve ||
            app->game.currentPlayer != app->game.aiPlayer);

    if (HUD_UpdateBoardCell(&app->hud, &app->menu,
        boardRect, BOARD_SIZE, BOARD_SIZE,
        (const int (*)[HUD_MAX_BOARD_DIM])hudBoard, canHumanMove,
        &clickedRow, &clickedCol, GetFrameTime())) {
        if (clickedRow >= 0 && clickedCol >= 0) {
            Game_playMove(&app->game, clickedRow, clickedCol);
            SyncHudState(app);
            CompleteRound(app);
        }
    }

    DrawRectangleRec(boardRect, (Color) { 20, 25, 31, 255 });
    DrawRectangleLinesEx(boardRect, 2.0f, (Color) { 75, 91, 102, 255 });

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            Rectangle cellRect = {
                boardRect.x + (float)col * cellWidth,
                boardRect.y + (float)row * cellHeight,
                cellWidth,
                cellHeight
            };

            HUD_DrawBoardCell(&app->hud, cellRect, row, col,
                hudBoard[row][col], false);
        }
    }

    for (int i = 1; i < BOARD_SIZE; ++i) {
        float x = boardRect.x + (float)i * cellWidth;
        float y = boardRect.y + (float)i * cellHeight;

        DrawLineEx((Vector2) { x, boardRect.y },
            (Vector2) {
            x, boardRect.y + boardRect.height
        },
            1.0f, (Color) { 70, 82, 92, 180 });
        DrawLineEx((Vector2) { boardRect.x, y },
            (Vector2) {
            boardRect.x + boardRect.width, y
        },
            1.0f, (Color) { 70, 82, 92, 180 });
    }
}

static void UpdateBot(App* app, float dt)
{
    if (!app->isPve || app->hud.isPaused || app->hud.isSaveNaming ||
        HUD_IsMatchStopped(&app->hud) ||
        app->game.currentPlayer != app->game.aiPlayer) {
        return;
    }

    app->botThinkDelay -= dt;
    if (app->botThinkDelay > 0.0f) return;

    Game_playAIMove(&app->game);
    app->botThinkDelay = 0.25f;
    SyncHudState(app);
    CompleteRound(app);
}

static void SaveCurrentMatch(App* app)
{
    SavedMatchData data;

    if (!app->gameInitialized) return;
    memset(&data, 0, sizeof(data));

    CopyText(data.saveName, sizeof(data.saveName),
        HUD_GetSaveName(&app->hud), "autosave");
    CopyText(data.p1Name, sizeof(data.p1Name),
        app->game.players[0].name, "ALPHA");
    CopyText(data.p2Name, sizeof(data.p2Name),
        app->game.players[1].name, "OMEGA");

    data.round = app->roundNumber;
    data.currentTurnPiece = CurrentHudPiece(app);
    data.p1Wins = app->hud.alphaButtons;
    data.p2Wins = app->hud.omegaButtons;
    data.alphaAgent = app->hud.p1.selectedAgent;
    data.omegaAgent = app->hud.p2.selectedAgent;
    data.gameMode = app->isPve ? PLAY_MODE_PVE : PLAY_MODE_PVP;
    data.difficulty = app->game.config.aiDepth;
    data.humanPiece = app->humanPiece;
    data.boardRows = BOARD_SIZE;
    data.boardCols = BOARD_SIZE;

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            Cell cell = app->game.board.cells[row][col];
            data.board[row][col] = (cell == CELL_X) ? 1 :
                (cell == CELL_O) ? 2 : 0;
        }
    }

    if (!Load_SaveMatch(&data)) {
        TraceLog(LOG_WARNING, "Could not save this match.");
    }
}

static void LoadSelectedMatch(App* app)
{
    const SavedMatchData* data = Load_GetSelectedData(&app->load);
    GameConfig config;
    int humanPiece;

    if (!data) return;

    app->isPve = (data->gameMode == PLAY_MODE_PVE);
    humanPiece = (data->humanPiece == 2) ? 2 : 1;
    app->humanPiece = humanPiece;
    app->roundNumber = data->round > 0 ? data->round : 1;
    app->roundProcessed = false;

    Game_init(&app->game,
        app->isPve ? GAME_MODE_PVE : GAME_MODE_PVP,
        data->difficulty >= 4 ? AI_DIFFICULTY_HARD : AI_DIFFICULTY_MEDIUM);

    config = app->game.config;
    config.aiDepth = data->difficulty;
    if (config.aiDepth < 1 || config.aiDepth > 4) config.aiDepth = 2;
    Game_setConfig(&app->game, &config);

    app->game.aiPlayer = (humanPiece == 1) ? CELL_O : CELL_X;
    Game_setPlayerName(&app->game, CELL_X, data->p1Name);
    Game_setPlayerName(&app->game, CELL_O, data->p2Name);
    Board_clear(&app->game.board);

    for (int row = 0; row < BOARD_SIZE && row < data->boardRows; ++row) {
        for (int col = 0; col < BOARD_SIZE && col < data->boardCols; ++col) {
            if (data->board[row][col] == 1) {
                app->game.board.cells[row][col] = CELL_X;
            }
            else if (data->board[row][col] == 2) {
                app->game.board.cells[row][col] = CELL_O;
            }
        }
    }

    app->game.currentPlayer = (data->currentTurnPiece == 2) ? CELL_O : CELL_X;
    app->gameInitialized = true;
    app->botThinkDelay = 0.25f;

    HUD_ResetMatchScore(&app->hud);
    HUD_SetMatchState(&app->hud,
        app->roundNumber,
        data->currentTurnPiece,
        data->p1Wins,
        data->p2Wins,
        app->game.players[0].name,
        app->game.players[1].name);
    HUD_SetSelectedAgents(&app->hud,
        (HudAgentId)data->alphaAgent,
        (HudAgentId)data->omegaAgent);
    HUD_SetPaused(&app->hud, false);

    Menu_StopMusic(&app->menu);
    Play_StopMusic(&app->play);
    HUD_StartMusic(&app->hud);
    app->screen = SCREEN_GAME;
}

static void UpdateVolumes(App* app)
{
    float musicVolume = Settings_GetMusicVolume(&app->settings);
    float sfxVolume = Settings_GetSfxVolume(&app->settings);

    Play_SetMusicVolume(&app->play, musicVolume);
    Play_SetSfxVolume(&app->play, sfxVolume);
    HUD_SetMusicVolume(&app->hud, musicVolume);
    HUD_SetSfxVolume(&app->hud, sfxVolume);
    GameOver_SetMusicVolume(&app->gameOver, musicVolume);

    if (app->menu.bgmLoaded) {
        SetMusicVolume(app->menu.bgm, musicVolume);
    }
}

static void ResizeAllScreens(App* app)
{
    int width = GetScreenWidth();
    int height = GetScreenHeight();

    Menu_Resize(&app->menu, width, height);
    Play_Resize(&app->play, width, height);
    Pvp_Resize(&app->pvp, width, height);
    Pve_Resize(&app->pve, width, height);
    HUD_Resize(&app->hud, width, height);
    Settings_Resize(&app->settings, width, height);
    Load_Resize(&app->load, width, height);
    GameOver_Resize(&app->gameOver, width, height);
}

int main(void)
{
    App app;
    bool running = true;

    memset(&app, 0, sizeof(app));

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
    if (!IsWindowReady()) return 1;

    InitAudioDevice();
    SetTargetFPS(60);
    srand((unsigned int)time(NULL));
    SetExitKey(KEY_NULL);

    app.screen = SCREEN_MENU;
    app.humanPiece = 1;

    Menu_Init(&app.menu, GetScreenWidth(), GetScreenHeight());
    Play_Init(&app.play, GetScreenWidth(), GetScreenHeight());
    Pvp_Init(&app.pvp, GetScreenWidth(), GetScreenHeight());
    Pve_Init(&app.pve, GetScreenWidth(), GetScreenHeight());
    HUD_Init(&app.hud, GetScreenWidth(), GetScreenHeight());
    Settings_Init(&app.settings, GetScreenWidth(), GetScreenHeight());
    Load_Init(&app.load, GetScreenWidth(), GetScreenHeight());
    GameOver_Init(&app.gameOver, GetScreenWidth(), GetScreenHeight());

    Menu_StartMusic(&app.menu);

    while (running && !WindowShouldClose()) {
        float dt = GetFrameTime();
        ResizeAllScreens(&app);

        switch (app.screen) {
        case SCREEN_MENU: {
            MenuAction action = MENU_ACTION_NONE;
            Menu_Update(&app.menu, &action, dt);

            if (action == MENU_ACTION_PLAY) {
                Play_Open(&app.play);
                app.screen = SCREEN_PLAY;
            }
            else if (action == MENU_ACTION_LOAD) {
                Load_Open(&app.load);
                app.screen = SCREEN_LOAD;
            }
            else if (action == MENU_ACTION_SETTINGS) {
                Settings_Open(&app.settings);
                app.screen = SCREEN_SETTINGS;
            }
            else if (action == MENU_ACTION_EXIT) {
                running = false;
            }
        } break;

        case SCREEN_PLAY: {
            PlayAction action = Play_Update(&app.play, &app.menu, dt);
            if (action == PLAY_ACTION_BACK) {
                Play_StopMusic(&app.play);
                Menu_StartMusic(&app.menu);
                app.screen = SCREEN_MENU;
            }
            else if (action == PLAY_ACTION_SELECT_PVP) {
                Pvp_Open(&app.pvp);
                app.screen = SCREEN_PVP_SETUP;
            }
            else if (action == PLAY_ACTION_SELECT_PVE) {
                Pve_Open(&app.pve);
                app.screen = SCREEN_PVE_SETUP;
            }
        } break;

        case SCREEN_PVP_SETUP: {
            PvpAction action = Pvp_Update(&app.pvp, &app.menu, &app.play, dt);
            if (action == PVP_ACTION_BACK) {
                Play_Open(&app.play);
                app.screen = SCREEN_PLAY;
            }
            else if (action == PVP_ACTION_READY) {
                StartPvpMatch(&app);
            }
        } break;

        case SCREEN_PVE_SETUP: {
            PveAction action = Pve_Update(&app.pve, &app.menu, &app.play, dt);
            if (action == PVE_ACTION_BACK) {
                Play_Open(&app.play);
                app.screen = SCREEN_PLAY;
            }
            else if (action == PVE_ACTION_READY) {
                StartPveMatch(&app);
            }
        } break;

        case SCREEN_GAME: {
            HUD_Update(&app.hud, dt);

            if (app.hud.isPaused || app.hud.isSaveNaming) {
                Game_pause(&app.game);
            }
            else {
                Game_resume(&app.game);
            }

            HudPauseAction pauseAction = HUD_HandlePauseInput(&app.hud);
            if (pauseAction == HUD_PAUSE_ACTION_SAVE_CONFIRMED) {
                SaveCurrentMatch(&app);
            }
            else if (pauseAction == HUD_PAUSE_ACTION_EXIT) {
                ReturnToMenu(&app);
                break;
            }

            if (!app.hud.isPaused && !app.hud.isSaveNaming) {
                Game_tick(&app.game, (unsigned int)(dt * 1000.0f));
                CompleteRound(&app);
                UpdateBot(&app, dt);
            }
        } break;

        case SCREEN_SETTINGS: {
            SettingsAction action = Settings_Update(&app.settings, &app.menu, dt);
            UpdateVolumes(&app);
            if (action == SETTINGS_ACTION_BACK) {
                app.screen = SCREEN_MENU;
            }
        } break;

        case SCREEN_LOAD: {
            LoadAction action = Load_Update(&app.load, &app.menu, dt);
            if (action == LOAD_ACTION_BACK) {
                app.screen = SCREEN_MENU;
            }
            else if (action == LOAD_ACTION_LOADED) {
                LoadSelectedMatch(&app);
            }
        } break;

        case SCREEN_GAME_OVER: {
            GameOverAction action = GameOver_Update(&app.gameOver, &app.menu, dt);
            if (action == GAMEOVER_ACTION_BACK_TO_MENU) {
                HUD_ResetMatchScore(&app.hud);
                ReturnToMenu(&app);
            }
        } break;
        }

        BeginDrawing();
        ClearBackground((Color) { 10, 13, 18, 255 });

        switch (app.screen) {
        case SCREEN_MENU:
            Menu_Draw(&app.menu);
            break;
        case SCREEN_PLAY:
            Menu_Draw(&app.menu);
            Play_Draw(&app.play);
            break;
        case SCREEN_PVP_SETUP:
            Menu_Draw(&app.menu);
            Pvp_Draw(&app.pvp);
            break;
        case SCREEN_PVE_SETUP:
            Menu_Draw(&app.menu);
            Pve_Draw(&app.pve);
            break;
        case SCREEN_GAME:
            DrawGameBoard(&app);
            HUD_Draw(&app.hud);
            break;
        case SCREEN_SETTINGS:
            Menu_Draw(&app.menu);
            Settings_Draw(&app.settings);
            break;
        case SCREEN_LOAD:
            Menu_Draw(&app.menu);
            Load_Draw(&app.load);
            break;
        case SCREEN_GAME_OVER:
            GameOver_Draw(&app.gameOver);
            break;
        }

        EndDrawing();
    }

    GameOver_Unload(&app.gameOver);
    Load_Unload(&app.load);
    Settings_Unload(&app.settings);
    HUD_UnloadPngAssets(&app.hud);
    Pve_Unload(&app.pve);
    Pvp_Unload(&app.pvp);
    Play_Unload(&app.play);
    Menu_Unload(&app.menu);

    if (IsAudioDeviceReady()) CloseAudioDevice();
    CloseWindow();

    return 0;
}
