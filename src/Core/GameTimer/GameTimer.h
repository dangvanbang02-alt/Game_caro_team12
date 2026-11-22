#ifndef GAME_TIMER_H
#define GAME_TIMER_H
#include <stdbool.h>
typedef struct {
    float elapsedTime;
    float remainingTime;
    bool isRunning;
} GameTimer;

void Timer_Init(GameTimer* timer, float duration);
void Timer_Start(GameTimer* timer);
void Timer_Update(GameTimer* timer, float deltaTime);
void Timer_Reset(GameTimer* timer, float duration);
void Timer_Stop(GameTimer* timer);
bool Timer_IsExpired(const GameTimer* timer);

#endif