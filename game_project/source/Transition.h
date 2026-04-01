#pragma once
#include "GameStateManager.h"
#include "GameStateList.h"

enum class TransitionSheet
{
    DEFAULT = 0,
    LEVEL1,
    LEVEL2,
    LEVEL3,
    BOSSLEVEL
};

enum class TransitionPhase
{
	TRANSITION_NONE,
	TRANSITION_OUT,
	TRANSITION_IN
};

void Transition_Init();
void Transition_Free();
void Transition_Start(GS_STATES nextState);
void Transition_Start(GS_STATES nextState, TransitionSheet sheet);
void Transition_StartImmediate(GS_STATES nextState);
void Transition_Update(float dt);
void Transition_Draw();

bool Transition_IsActive();
bool Transition_IsSwitchReady();
int Transition_GetState();
void Transition_BeginFadeIn();
void Transition_Reset();