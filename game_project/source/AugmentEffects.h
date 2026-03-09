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
