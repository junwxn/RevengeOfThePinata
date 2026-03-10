#pragma once
class Player;
void HUD_Load();
void HUD_Init();
void HUD_Draw(const Player* player, float camX, float camY);
void HUD_Free();
void HUD_Unload();
