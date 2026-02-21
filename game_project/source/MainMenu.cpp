#include "pch.h"
#include "MainMenu.h"
#include "GameStateManager.h"

void MainMenu_Load() {

}
void MainMenu_Init() {

}
void MainMenu_Update(float dt) {

	if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist()) {
		next = GS_LEVEL1;
	}

}
void MainMenu_Draw() {
	AESysFrameStart();
	AEGfxSetBackgroundColor(0.0f, 0.23f, 0.34f);

	AESysFrameEnd();
}
void MainMenu_Free() {

}
void MainMenu_Unload() {

}