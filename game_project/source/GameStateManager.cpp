#include "pch.h"

#include "GameStateManager.h"
#include "Game.h"
#include "MainMenu.h"
#include "level1.h"

int current = 0, previous = 0, next = 0;

FP fpLoad = nullptr, fpInitialize = nullptr, fpDraw = nullptr, fpFree = nullptr, fpUnload = nullptr;
void (*fpUpdate)(float) = nullptr;

void GSM_Initialize(int startingState) {
	current = previous = next = startingState;
	//std::cout << "GSM:Initialize" << std::endl;
}

void GSM_Update() {
	std::cout << "GSM:Update" << std::endl;

	switch (current) {
	case GS_MAINMENU:
		fpLoad = MainMenu_Load;
		fpInitialize = MainMenu_Init;
		fpUpdate = MainMenu_Update;
		fpDraw = MainMenu_Draw;
		fpFree = MainMenu_Free;
		fpUnload = MainMenu_Unload;
	case GS_LEVEL1:
		fpLoad = Level1_Load;
		fpInitialize = Level1_Init;
		fpUpdate = Level1_Update;
		fpDraw = Level1_Draw;
		fpFree = Level1_Free;
		fpUnload = Level1_Unload;
		break;
	case GS_RESTART:
		break;
	case GS_QUIT:
		break;
	default:
		break;
	}
}