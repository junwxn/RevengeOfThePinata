#include "pch.h"
#include "Victory.h"
#include "GameStateManager.h"
#include "Utils.h"

static AEGfxVertexList* rectMesh = nullptr;
static s8 fontTitle = -1;
static s8 fontBody = -1;
static float animTimer = 0.0f;

void Victory_Load() {
	rectMesh = CreateRectMesh(0xFFFFFFFF);
	fontTitle = AEGfxCreateFont("Assets/liberation-mono.ttf", 72);
	fontBody = AEGfxCreateFont("Assets/liberation-mono.ttf", 36);
}

void Victory_Init() {
	AEGfxSetCamPosition(0.0f, 0.0f);
	animTimer = 0.0f;
}

void Victory_Update(float dt) {
	if (!AESysDoesWindowExist()) {
		next = GS_QUIT;
		return;
	}

	animTimer += dt;

	// Any key or mouse click returns to main menu
	if (AEInputCheckTriggered(AEVK_LBUTTON) ||
		AEInputCheckTriggered(AEVK_RETURN) ||
		AEInputCheckTriggered(AEVK_SPACE) ||
		AEInputCheckTriggered(AEVK_ESCAPE)) {
		next = GS_MAINMENU;
	}
}

void Victory_Draw() {
	AESysFrameStart();
	AEGfxSetBackgroundColor(0.05f, 0.1f, 0.2f);

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// Gold background panel
	DrawMesh(rectMesh, 900, 400, -450, 0, 0, 255, 200, 50, 180);

	// Text
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// Title with bob
	{
		float bobY = 100.0f + sinf(animTimer * 2.0f) * 15.0f;
		const char* title = "Congratulations!";
		float tw, th;
		AEGfxGetPrintSize(fontTitle, title, 1.0f, &tw, &th);
		AEGfxPrint(fontTitle, title, -tw * 0.5f, bobY / 450.0f, 1.0f, 1.0f, 0.85f, 0.0f, 1.0f);
	}

	// Subtitle
	{
		const char* sub = "You Win!";
		float tw, th;
		AEGfxGetPrintSize(fontTitle, sub, 1.0f, &tw, &th);
		AEGfxPrint(fontTitle, sub, -tw * 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	}

	// Prompt
	{
		const char* prompt = "Click or press any key to continue...";
		float tw, th;
		AEGfxGetPrintSize(fontBody, prompt, 1.0f, &tw, &th);
		float alpha = (sinf(animTimer * 3.0f) + 1.0f) * 0.5f;
		AEGfxPrint(fontBody, prompt, -tw * 0.5f, -200.0f / 450.0f, 1.0f, 0.8f, 0.8f, 0.8f, alpha);
	}

	AESysFrameEnd();
}

void Victory_Free() {
}

void Victory_Unload() {
	if (rectMesh) {
		AEGfxMeshFree(rectMesh);
		rectMesh = nullptr;
	}
	if (fontTitle >= 0) {
		AEGfxDestroyFont(fontTitle);
		fontTitle = -1;
	}
	if (fontBody >= 0) {
		AEGfxDestroyFont(fontBody);
		fontBody = -1;
	}
}