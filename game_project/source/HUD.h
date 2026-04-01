/*************************************************************************
@file		HUD.h
@Author		Chiu Jun Wen j.chiu@digipen.edu
@Co-authors	nil
@brief		This file contains the function declarations for the HUD system,
            including its initialization, updating, rendering, and cleanup.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/
#pragma once
class Player;
void HUD_Load();
void HUD_Init();
void HUD_Draw(const Player* player, float camX, float camY);
void HUD_Free();
void HUD_Unload();
