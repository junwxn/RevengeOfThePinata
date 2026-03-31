/*************************************************************************
@file		BossLevel.h
@Author		Chiu Jun Wen j.chiu@digipen.edu
@Co-authors	nil
@brief		This file contains the function declarations for the boss level,
            including its initialization, updating, drawing, and freeing.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/
#pragma once

void BossLevel_Load();
void BossLevel_Init();
void BossLevel_Update(float dt);
void BossLevel_Draw();
void BossLevel_Free();
void BossLevel_Unload();