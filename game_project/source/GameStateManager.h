/*************************************************************************
@file		GameStateManager.h
@Author		Chiu Jun Wen j.chiu@digipen.edu
@Co-authors	nil
@brief		This file contains the function declarations for managing game
			states, including their initialization, updating, rendering,
			and transitioning between states.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#pragma once

typedef void(*FP)(void);

extern int current, previous, next;

extern  FP fpLoad, fpInitialize, fpDraw, fpFree, fpUnload;

extern void(*fpUpdate)(float dt);

void GSM_Initialize(int startingState);
//void GSM_Update();
void GSM_Update(float dt);
void GSM_Draw();