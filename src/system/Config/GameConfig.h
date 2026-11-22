#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H
#include <stddef.h>

typedef struct {
    unsigned int turnTimeLimitMs;
    unsigned int matchTimeLimitMs;
    int aiDepth;
} GameConfig;

void GameConfig_init(GameConfig* config);

#endif
