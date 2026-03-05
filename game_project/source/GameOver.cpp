#include "pch.h"
#include "GameOver.h"
#include "GameStateManager.h"
#include "Utils.h"

static AEGfxVertexList* rectMesh = nullptr;
static s8 fontTitle = -1;
static s8 fontBody = -1;
static float animTimer = 0.0f;

// --- Button helpers ---
struct GOButton {
	float x, y, w, h;
	const char* label;
	bool hovered;
};

static GOButton buttons[2];

static bool IsInside(float wx, float wy, const GOButton& b) {
	return wx >= b.x - b.w * 0.5f && wx <= b.x + b.w * 0.5f &&
	       wy >= b.y - b.h * 0.5f && wy <= b.y + b.h * 0.5f;
}

void GameOver_Load() {
	rectMesh = CreateRectMesh(0xFFFFFFFF);
	fontTitle = AEGfxCreateFont("Assets/liberation-mono.ttf", 72);
	fontBody = AEGfxCreateFont("Assets/liberation-mono.ttf", 36);
}

void GameOver_Init() {
	AEGfxSetCamPosition(0.0f, 0.0f);
	animTimer = 0.0f;

	const char* labels[] = { "Retry", "Main Menu" };
	for (int i = 0; i < 2; i++) {
		buttons[i].x       = 0.0f;
		buttons[i].y       = -60.0f - 90.0f * i;
		buttons[i].w       = 300.0f;
		buttons[i].h       = 60.0f;
		buttons[i].label   = labels[i];
		buttons[i].hovered = false;
	}
}

void GameOver_Update(float dt) {
	if (!AESysDoesWindowExist()) {
		next = GS_QUIT;
		return;
	}

	animTimer += dt;

	// Mouse -> world coords
	s32 mx, my;
	AEInputGetCursorPosition(&mx, &my);
	float worldX = (float)mx - 800.0f;
	float worldY = 450.0f - (float)my;

	for (int i = 0; i < 2; i++)
		buttons[i].hovered = IsInside(worldX, worldY, buttons[i]);

	if (AEInputCheckTriggered(AEVK_LBUTTON)) {
		if (buttons[0].hovered) next = GS_MAINMENU;  // Retry -> back to menu to start fresh
		if (buttons[1].hovered) next = GS_MAINMENU;
	}

	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		next = GS_MAINMENU;
	}
}

void GameOver_Draw() {
	AESysFrameStart();
	AEGfxSetBackgroundColor(0.1f, 0.02f, 0.02f);

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// Dark red background panel
	DrawMesh(rectMesh, 900, 500, -450, 0, 0, 150, 20, 20, 180);

	// Buttons
	for (int i = 0; i < 2; i++) {
		float w = buttons[i].w;
		float h = buttons[i].h;
		float cr = 200, cg = 50, cb = 50;
		if (buttons[i].hovered) {
			w *= 1.05f; h *= 1.05f;
			cr = 255; cg = 200; cb = 50;
		}
		DrawMesh(rectMesh, w, h, buttons[i].x - w * 0.5f, buttons[i].y, 0, cr, cg, cb, 255);
	}

	// Text
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// Title with shake
	{
		float shakeX = sinf(animTimer * 8.0f) * 3.0f;
		const char* title = "GAME OVER";
		float tw, th;
		AEGfxGetPrintSize(fontTitle, title, 1.0f, &tw, &th);
		AEGfxPrint(fontTitle, title, -tw * 0.5f + shakeX / 800.0f, 100.0f / 450.0f, 1.0f, 1.0f, 0.2f, 0.2f, 1.0f);
	}

	// Subtitle
	{
		const char* sub = "You have been defeated...";
		float tw, th;
		AEGfxGetPrintSize(fontBody, sub, 1.0f, &tw, &th);
		AEGfxPrint(fontBody, sub, -tw * 0.5f, 20.0f / 450.0f, 1.0f, 0.8f, 0.8f, 0.8f, 1.0f);
	}

	// Button labels
	for (int i = 0; i < 2; i++) {
		float tw, th;
		AEGfxGetPrintSize(fontBody, buttons[i].label, 1.0f, &tw, &th);
		float nx = buttons[i].x / 800.0f - tw * 0.5f;
		float ny = buttons[i].y / 450.0f - th * 0.5f;
		AEGfxPrint(fontBody, buttons[i].label, nx, ny, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	}

	AESysFrameEnd();
}

void GameOver_Free() {
}

void GameOver_Unload() {
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
