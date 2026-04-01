/*************************************************************************
@file		Pause.h
@Author		Chiu Jun Wen j.chiu@digipen.edu
@Co-authors	nil
@brief		This file contains the function declarations for the pause state,
            including its initialization, updating, rendering, and cleanup.
Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/
#pragma once

void Pause_Load();
void Pause_Init();
bool Pause_Update(bool isPlayerAlive);   // returns true if paused (caller should skip game logic)
void Pause_Draw(float camX, float camY);
void Pause_Unload();
