#include <stdio.h>
#include "Board.h"
#include "Game.h"

int main(void)
{
    Game game;


    Game_init(&game, GAME_MODE_PVP, AI_DIFFICULTY_MEDIUM);

    printf("Gomoku initialized!\n");

    return 0;
}