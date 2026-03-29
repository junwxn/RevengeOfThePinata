#pragma once
// #include "Utils.h"
#include "Audio.h"

void Settings_Init();
void Settings_Update(float dt);
void Settings_Draw();
void Settings_Free();
void Settings_Unload();

static s8 fontTitle = -1;