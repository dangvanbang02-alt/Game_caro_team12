#include "GameConfig.h"

void GameConfig_init(GameConfig* config)
{
    if (config == NULL)
        return;

    config->turnTimeLimitMs = 30000;
    config->matchTimeLimitMs = 600000;
    config->aiDepth = 3;
}
