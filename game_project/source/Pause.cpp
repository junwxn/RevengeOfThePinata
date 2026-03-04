#include "pch.h"
#include "Pause.h"
#include "GameStateManager.h"
#include "Utils.h"

// --- Button struct ---
struct PauseButton {
	float x, y, w, h;
	const char* label;
	bool hovered;
};

// --- File-scoped statics ---
static AEGfxVertexList* rectMesh = nullptr;
static s8 pauseFont = -1;
static bool paused = false;
static PauseButton buttons[3];

static bool IsInside(float wx, float wy, const PauseButton& b) {
	return wx >= b.x - b.w * 0.5f && wx <= b.x + b.w * 0.5f &&
	       wy >= b.y - b.h * 0.5f && wy <= b.y + b.h * 0.5f;
}

void Pause_Load() {
	rectMesh  = CreateRectMesh(0xFFFFFFFF);
	pauseFont = AEGfxCreateFont("Assets/liberation-mono.ttf", 36);
}

void Pause_Init() {
	paused = false;
	const char* labels[] = { "Resume", "Main Menu", "Restart" };
	for (int i = 0; i < 3; i++) {
		buttons[i].x       = 0.0f;
		buttons[i].y       = 20.0f - 80.0f * i;
		buttons[i].w       = 300.0f;
		buttons[i].h       = 60.0f;
		buttons[i].label   = labels[i];
		buttons[i].hovered = false;
	}
}

bool Pause_Update() {
	// Toggle pause
	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		paused = !paused;
	}

	if (!paused) return false;

	// Mouse -> world coords (window 1600x900, origin at center)
	s32 mx, my;
	AEInputGetCursorPosition(&mx, &my);
	float worldX = (float)mx - 800.0f;
	float worldY = 450.0f - (float)my;

	for (int i = 0; i < 3; i++)
		buttons[i].hovered = IsInside(worldX, worldY, buttons[i]);

	if (AEInputCheckTriggered(AEVK_LBUTTON)) {
		if (buttons[0].hovered) paused = false;        // Resume
		if (buttons[1].hovered) next = GS_MAINMENU;    // Main Menu
		if (buttons[2].hovered) next = GS_RESTART;     // Restart
	}

	return true;
}

void Pause_Draw() {
	if (!paused) return;

	// Reset camera to origin so UI draws in screen space
	AEGfxSetCamPosition(0.0f, 0.0f);
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// Dark overlay
	DrawMesh(rectMesh, 1600, 900, -800, 0, 0, 0, 0, 0, 180);

	// Buttons: magenta normal, golden hover
	for (int i = 0; i < 3; i++) {
		float w = buttons[i].w;
		float h = buttons[i].h;
		float cr = 200, cg = 50, cb = 120;
		if (buttons[i].hovered) {
			w *= 1.05f; h *= 1.05f;
			cr = 255; cg = 200; cb = 50;
		}
		DrawMesh(rectMesh, w, h, buttons[i].x - w * 0.5f, buttons[i].y, 0, cr, cg, cb, 255);
	}

	// Text
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// "PAUSED" title in gold
	float tw, th;
	AEGfxGetPrintSize(pauseFont, "PAUSED", 2.0f, &tw, &th);
	AEGfxPrint(pauseFont, "PAUSED", -tw * 0.5f, 200.0f / 450.0f, 2.0f, 1.0f, 0.85f, 0.0f, 1.0f);

	// Button labels in white
	for (int i = 0; i < 3; i++) {
		AEGfxGetPrintSize(pauseFont, buttons[i].label, 1.0f, &tw, &th);
		float nx = buttons[i].x / 800.0f - tw * 0.5f;
		float ny = buttons[i].y / 450.0f - th * 0.5f;
		AEGfxPrint(pauseFont, buttons[i].label, nx, ny, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	}
}

void Pause_Unload() {
	if (rectMesh) {
		AEGfxMeshFree(rectMesh);
		rectMesh = nullptr;
	}
	if (pauseFont >= 0) {
		AEGfxDestroyFont(pauseFont);
		pauseFont = -1;
	}
}
