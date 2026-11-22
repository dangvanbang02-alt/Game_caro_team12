#include "GameTimer.h"

void Timer_Init(GameTimer* timer, float duration) {
    timer->elapsedTime = 0.0f;
    timer->remainingTime = duration;
    timer->isRunning = false;
}

void Timer_Start(GameTimer* timer) {
    timer->isRunning = true;
}

void Timer_Update(GameTimer* timer, float deltaTime) {
    if (!timer->isRunning) return;

    timer->elapsedTime += deltaTime;
    timer->remainingTime -= deltaTime;

    if (timer->remainingTime <= 0.0f) {
        timer->remainingTime = 0.0f;
        timer->isRunning = false;
    }
}

void Timer_Reset(GameTimer* timer, float duration) {
    Timer_Init(timer, duration);
}

void Timer_Stop(GameTimer* timer) {
    timer->isRunning = false;
}

bool Timer_IsExpired(const GameTimer* timer) {
    return timer->remainingTime <= 0.0f;
}
