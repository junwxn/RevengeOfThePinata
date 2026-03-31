#include "pch.h"

#include "GameStateManager.h"
#include "MainMenu.h"
#include "Tutorial.h"
#include "level1.h"
#include "Level2.h"
#include "Level3.h"
#include "BossLevel.h"
#include "Victory.h"
#include "GameOver.h"
#include "TestLevel.h"
#include "Transition.h"
#include "Splash.h"
#include "Audio.h"

int current = 0, previous = 0, next = 0;

FP fpLoad = nullptr, fpInitialize = nullptr, fpDraw = nullptr, fpFree = nullptr, fpUnload = nullptr;
void (*fpUpdate)(float) = nullptr;

// New static function used to change states when needed
// Previous one called every frame
static void GSM_MapFunctionsToCurrentState()
{
	switch (current) {
	case GS_SPLASH:
		fpLoad = Splash_Load;
		fpInitialize = Splash_Init;
		fpUpdate = Splash_Update;
		fpDraw = Splash_Draw;
		fpFree = Splash_Free;
		fpUnload = Splash_Unload;
		break;

	case GS_MAINMENU:
		fpLoad = MainMenu_Load;
		fpInitialize = MainMenu_Init;
		fpUpdate = MainMenu_Update;
		fpDraw = MainMenu_Draw;
		fpFree = MainMenu_Free;
		fpUnload = MainMenu_Unload;
		break;
	case GS_TUTORIAL:
		fpLoad = Tutorial_Load;
		fpInitialize = Tutorial_Init;
		fpUpdate = Tutorial_Update;
		fpDraw = Tutorial_Draw;
		fpFree = Tutorial_Free;
		fpUnload = Tutorial_Unload;
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
	case GS_TEST:
		fpLoad = TestLevel_Load;
		fpInitialize = TestLevel_Init;
		fpUpdate = TestLevel_Update;
		fpDraw = TestLevel_Draw;
		fpFree = TestLevel_Free;
		fpUnload = TestLevel_Unload;
		break;
	case GS_RESTART:
	case GS_QUIT:
	default:
		fpLoad = nullptr;
		fpInitialize = nullptr;
		fpUpdate = nullptr;
		fpDraw = nullptr;
		fpFree = nullptr;
		fpUnload = nullptr;
		break;
	}
}

void GSM_Initialize(int startingState)
{
	current = previous = next = startingState;

	Transition_Init();

	GSM_MapFunctionsToCurrentState();

	//std::cout << "Initial load of state: " << current << std::endl;

	if (fpLoad) fpLoad();
	if (fpInitialize) fpInitialize();
}

void GSM_Update(float dt)
{
	if (AEInputCheckTriggered(AEVK_LBUTTON))
	{
		if (current == GS_MAINMENU ||
			current == GS_GAMEOVER ||
			current == GS_VICTORY)
		{
			gAudio.PlayClickSFX();
		}
	}

	if (Transition_IsActive())
	{
		Transition_Update(dt);
	}

	if (Transition_IsSwitchReady())
	{
		bool wasTransitionActive = Transition_IsActive();

		if (fpFree) fpFree();
		if (fpUnload) fpUnload();

		previous = current;
		current = Transition_GetState();
		next = current;

		GSM_MapFunctionsToCurrentState();

		if (fpLoad) fpLoad();
		if (fpInitialize) fpInitialize();

		if (wasTransitionActive)
			Transition_BeginFadeIn();
		else
			Transition_Reset();

		return;
	}

	if (Transition_IsActive())
		return;

	if (fpUpdate)
		fpUpdate(dt);
}

void GSM_Draw()
{
	if (!Transition_IsActive())
	{
		if (fpDraw)
			fpDraw();
	}

	Transition_Draw();
}