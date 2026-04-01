/*************************************************************************
@file		AugmentEffects.h
@Author		Chiu Jun Wen j.chiu@digipen.edu
@Co-authors	nil
@brief		This file contains the function declarations for augment effects,
            including their initialization, updating, drawing, and freeing.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/
#pragma once
#include <vector>
#include <memory>

class Player;
class Enemy;

void AugmentEffects_Init(Player* player);
void AugmentEffects_Register();
void AugmentEffects_Update(float dt, Player& player, std::vector<std::unique_ptr<Enemy>>& wave);
void AugmentEffects_Draw(float camX, float camY);
void AugmentEffects_Free();

// Shield Dash state — accessed by Combat.cpp for damage reflection
bool AugmentEffects_IsShieldActive();
