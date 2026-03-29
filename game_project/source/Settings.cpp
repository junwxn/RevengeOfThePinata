#include "pch.h" // Has to be first file to include in .cpp
#include "Settings.h"

void Settings_Init() {

	//Free before creation
	if (fontTitle != -1) { AEGfxDestroyFont(fontTitle); fontTitle = -1; }

	fontTitle = AEGfxCreateFont("Assets/fonts/Stick-Regular.ttf", 145);

}
void Settings_Update(float dt) {

}

void Settings_Draw() {

}

void Settings_Free() {

}

void Settings_Unload() {

}