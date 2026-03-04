#include "pch.h"

#include "GameStateManager.h"
#include "MainMenu.h"
#include "level1.h"
#include "Level2.h"
#include "Level3.h"
#include "BossLevel.h"
#include "Victory.h"
#include "GameOver.h"

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
		break;
	case GS_LEVEL1:
		fpLoad = Level1_Load;
		fpInitialize = Level1_Init;
		fpUpdate = Level1_Update;
		fpDraw = Level1_Draw;
		fpFree = Level1_Free;
		fpUnload = Level1_Unload;
		break;
	case GS_LEVEL2:
		fpLoad = Level2_Load;
		fpInitialize = Level2_Init;
		fpUpdate = Level2_Update;
		fpDraw = Level2_Draw;
		fpFree = Level2_Free;
		fpUnload = Level2_Unload;
		break;
	case GS_LEVEL3:
		fpLoad = Level3_Load;
		fpInitialize = Level3_Init;
		fpUpdate = Level3_Update;
		fpDraw = Level3_Draw;
		fpFree = Level3_Free;
		fpUnload = Level3_Unload;
		break;
	case GS_BOSSLEVEL:
		fpLoad = BossLevel_Load;
		fpInitialize = BossLevel_Init;
		fpUpdate = BossLevel_Update;
		fpDraw = BossLevel_Draw;
		fpFree = BossLevel_Free;
		fpUnload = BossLevel_Unload;
		break;
	case GS_VICTORY:
		fpLoad = Victory_Load;
		fpInitialize = Victory_Init;
		fpUpdate = Victory_Update;
		fpDraw = Victory_Draw;
		fpFree = Victory_Free;
		fpUnload = Victory_Unload;
		break;
	case GS_GAMEOVER:
		fpLoad = GameOver_Load;
		fpInitialize = GameOver_Init;
		fpUpdate = GameOver_Update;
		fpDraw = GameOver_Draw;
		fpFree = GameOver_Free;
		fpUnload = GameOver_Unload;
		break;
	case GS_RESTART:
		break;
	case GS_QUIT:
		break;
	default:
		break;
	}
}