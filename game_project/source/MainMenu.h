/*************************************************************************
@file		MainMenu.h
@Author		Chiu Jun Wen j.chiu@digipen.edu
@Co-authors	nil
@brief		This file contains the function declarations for the main menu state,
            including its initialization, updating, rendering, and cleanup.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/
#pragma once

void MainMenu_Load();
void MainMenu_Init();
void MainMenu_Update(float dt);
void MainMenu_Draw();
void MainMenu_Free();
void MainMenu_Unload();