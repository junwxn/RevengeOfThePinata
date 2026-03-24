#include "pch.h"
#include "GameOver.h"
#include "GameStateManager.h"
#include "Player.h"
#include "Utils.h"

static AEGfxVertexList* rectMesh = nullptr;
static s8 fontTitle = -1;
static s8 fontBody = -1;
static float animTimer = 0.0f;
static float entranceTimer = 0.0f;

// --- Helpers ---
static float Clamp01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
static float Smoothstep(float t) { t = Clamp01(t); return t * t * (3.0f - 2.0f * t); }
static float Lerp(float a, float b, float t) { return a + (b - a) * t; }

// --- Button ---
struct GOButton {
	float x, y, w, h;
	const char* label;
	bool hovered;
	float hoverT;
};

static GOButton buttons[2];

static bool IsInside(float wx, float wy, const GOButton& b) {
	return wx >= b.x - b.w * 0.5f && wx <= b.x + b.w * 0.5f &&
	       wy >= b.y - b.h * 0.5f && wy <= b.y + b.h * 0.5f;
}

static void DrawStyledButton(float cx, float cy, float baseW, float baseH, float hT, float alpha) {
	float w = baseW * (1.0f + 0.05f * hT);
	float h = baseH * (1.0f + 0.05f * hT);
	// Darker red tones for GameOver, lerp to gold on hover
	float fr = Lerp(160, 255, hT), fg = Lerp(40, 190, hT), fb = Lerp(40, 50, hT);
	float br = Lerp(100, 200, hT), bg = Lerp(20, 150, hT), bb = Lerp(20, 30, hT);
	// Shadow
	DrawMesh(rectMesh, w, h, cx - w * 0.5f + 3, cy - 3, 0, 0, 0, 0, 60 * alpha);
	// Border
	DrawMesh(rectMesh, w + 4, h + 4, cx - (w + 4) * 0.5f, cy, 0, br, bg, bb, 255 * alpha);
	// Fill
	DrawMesh(rectMesh, w, h, cx - w * 0.5f, cy, 0, fr, fg, fb, 240 * alpha);
	// Top highlight
	DrawMesh(rectMesh, w - 8, 2, cx - (w - 8) * 0.5f, cy + h * 0.5f - 3, 0, 255, 255, 255, (20 + hT * 30) * alpha);
}

void GameOver_Load() {
	rectMesh = CreateRectMesh(0xFFFFFFFF);
	fontTitle = AEGfxCreateFont("Assets/fonts/Stick-Regular.ttf", 72);
	fontBody = AEGfxCreateFont("Assets/fonts/Stick-Regular.ttf", 36);
}

void GameOver_Init() {
	g_PlayerAttackCharges = DEFAULT_ATTACK_CHARGES;  // Reset charges on death
	AEGfxSetCamPosition(0.0f, 0.0f);
	animTimer = 0.0f;
	entranceTimer = 0.0f;

	const char* labels[] = { "Retry", "Main Menu" };
	for (int i = 0; i < 2; i++) {
		buttons[i].x       = 0.0f;
		buttons[i].y       = -60.0f - 90.0f * i;
		buttons[i].w       = 300.0f;
		buttons[i].h       = 60.0f;
		buttons[i].label   = labels[i];
		buttons[i].hovered = false;
		buttons[i].hoverT  = 0.0f;
	}
}

void GameOver_Update(float dt) {
	if (!AESysDoesWindowExist()) {
		next = GS_QUIT;
		return;
	}

	animTimer += dt;
	entranceTimer += dt;

	// Mouse -> world coords
	s32 mx, my;
	AEInputGetCursorPosition(&mx, &my);
	float worldX = (float)mx - 800.0f;
	float worldY = 450.0f - (float)my;

	for (int i = 0; i < 2; i++)
		buttons[i].hovered = IsInside(worldX, worldY, buttons[i]);

	// Smooth hover
	for (int i = 0; i < 2; i++) {
		buttons[i].hoverT += (buttons[i].hovered ? 1.0f : -1.0f) * dt * 6.0f;
		buttons[i].hoverT = Clamp01(buttons[i].hoverT);
	}

	if (AEInputCheckTriggered(AEVK_LBUTTON)) {
		if (buttons[0].hovered) next = previous;
		if (buttons[1].hovered) next = GS_MAINMENU;
	}

	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		next = GS_MAINMENU;
	}
}

void GameOver_Draw() {
	AESysFrameStart();
	AEGfxSetBackgroundColor(0.06f, 0.01f, 0.01f);

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// Vignette: layered dark rects at screen edges
	// Top
	DrawMesh(rectMesh, 1600, 200, -800, 350, 0, 0, 0, 0, 120);
	DrawMesh(rectMesh, 1600, 100, -800, 400, 0, 0, 0, 0, 80);
	// Bottom
	DrawMesh(rectMesh, 1600, 200, -800, -350, 0, 0, 0, 0, 120);
	DrawMesh(rectMesh, 1600, 100, -800, -400, 0, 0, 0, 0, 80);
	// Left
	DrawMesh(rectMesh, 200, 900, -800, 0, 0, 0, 0, 0, 100);
	DrawMesh(rectMesh, 100, 900, -800, 0, 0, 0, 0, 0, 60);
	// Right
	DrawMesh(rectMesh, 200, 900, 600, 0, 0, 0, 0, 0, 100);
	DrawMesh(rectMesh, 100, 900, 700, 0, 0, 0, 0, 0, 60);

	float panelEase = Smoothstep(entranceTimer * 2.0f);

	// Panel border
	DrawMesh(rectMesh, 904, 504, -452, 0, 0, 100, 20, 20, 255 * panelEase);
	// Panel fill
	DrawMesh(rectMesh, 900, 500, -450, 0, 0, 20, 10, 15, 220 * panelEase);
	// Pulsing dark-red overlay
	float redPulse = 0.5f + 0.5f * sinf(animTimer * 1.5f);
	DrawMesh(rectMesh, 900, 500, -450, 0, 0, 120, 10, 10, (20 + redPulse * 30) * panelEase);

	// Styled buttons with entrance stagger
	for (int i = 0; i < 2; i++) {
		float delay = i * 0.08f + 0.2f;
		float ease = Smoothstep((entranceTimer - delay) * 2.5f);
		float yOff = (1.0f - ease) * 30.0f;
		DrawStyledButton(buttons[i].x, buttons[i].y - yOff,
		                 buttons[i].w, buttons[i].h,
		                 buttons[i].hoverT, ease);
	}

	// Text
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// Title with decaying shake
	{
		float titleEase = Smoothstep(entranceTimer * 2.5f);
		float shakeAmp = fmaxf(0.0f, 4.0f - animTimer * 0.3f);
		float shakeX = sinf(animTimer * 8.0f) * shakeAmp;
		const char* title = "GAME OVER";
		float tw, th;
		AEGfxGetPrintSize(fontTitle, title, 1.0f, &tw, &th);
		float titleY = 100.0f + (1.0f - titleEase) * 30.0f;
		AEGfxPrint(fontTitle, title, -tw * 0.5f + shakeX / 800.0f, titleY / 450.0f, 1.0f, 1.0f, 0.2f, 0.2f, titleEase);
	}

	// Subtitle
	{
		float subEase = Smoothstep((entranceTimer - 0.1f) * 2.5f);
		const char* sub = "You have been defeated...";
		float tw, th;
		AEGfxGetPrintSize(fontBody, sub, 1.0f, &tw, &th);
		AEGfxPrint(fontBody, sub, -tw * 0.5f, 20.0f / 450.0f, 1.0f, 0.8f, 0.8f, 0.8f, subEase);
	}

	// Button labels
	for (int i = 0; i < 2; i++) {
		float delay = i * 0.08f + 0.2f;
		float ease = Smoothstep((entranceTimer - delay) * 2.5f);
		float yOff = (1.0f - ease) * 30.0f;
		float tw, th;
		AEGfxGetPrintSize(fontBody, buttons[i].label, 1.0f, &tw, &th);
		float nx = buttons[i].x / 800.0f - tw * 0.5f;
		float ny = (buttons[i].y - yOff) / 450.0f - th * 0.5f;
		AEGfxPrint(fontBody, buttons[i].label, nx, ny, 1.0f, 1.0f, 1.0f, 1.0f, ease);
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
