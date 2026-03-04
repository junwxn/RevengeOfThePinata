#pragma once

void Pause_Load();
void Pause_Init();
bool Pause_Update();   // returns true if paused (caller should skip game logic)
void Pause_Draw();
void Pause_Unload();
