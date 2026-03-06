#include "pch.h"
#include "Victory.h"
#include "GameStateManager.h"
#include "Utils.h"
#include <cstdlib>

static AEGfxVertexList* rectMesh = nullptr;
static s8 fontTitle = -1;
static s8 fontBody = -1;
static float animTimer = 0.0f;
static float entranceTimer = 0.0f;

// --- Helpers ---
static float Clamp01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
static float Smoothstep(float t) { t = Clamp01(t); return t * t * (3.0f - 2.0f * t); }

// --- Confetti ---
struct Confetti {
	float x, y, vx, vy, size, rot, rotSpeed, r, g, b;
};

static const int NUM_CONFETTI = 40;
static Confetti confetti[NUM_CONFETTI];

static float RandFloat(float lo, float hi) {
	return lo + (float)(rand() % 10000) / 10000.0f * (hi - lo);
}

static void SpawnConfetti(Confetti& c, bool randomY) {
	c.x = RandFloat(-800.0f, 800.0f);
	c.y = randomY ? RandFloat(-450.0f, 500.0f) : RandFloat(480.0f, 700.0f);
	c.vx = RandFloat(-30.0f, 30.0f);
	c.vy = RandFloat(-120.0f, -60.0f);
	c.size = RandFloat(4.0f, 12.0f);
	c.rot = RandFloat(0.0f, 6.28f);
	c.rotSpeed = RandFloat(-3.0f, 3.0f);
	// Pick from bright colors
	int color = rand() % 5;
	switch (color) {
		case 0: c.r = 255; c.g = 200; c.b = 50;  break; // gold
		case 1: c.r = 255; c.g = 80;  c.b = 180; break; // magenta
		case 2: c.r = 50;  c.g = 200; c.b = 200; break; // teal
		case 3: c.r = 255; c.g = 140; c.b = 30;  break; // orange
		case 4: c.r = 80;  c.g = 220; c.b = 80;  break; // green
	}
}

void Victory_Load() {
	rectMesh = CreateRectMesh(0xFFFFFFFF);
	fontTitle = AEGfxCreateFont("Assets/liberation-mono.ttf", 72);
	fontBody = AEGfxCreateFont("Assets/liberation-mono.ttf", 36);
}

void Victory_Init() {
	AEGfxSetCamPosition(0.0f, 0.0f);
	animTimer = 0.0f;
	entranceTimer = 0.0f;
	for (int i = 0; i < NUM_CONFETTI; i++)
		SpawnConfetti(confetti[i], true);
}

void Victory_Update(float dt) {
	if (!AESysDoesWindowExist()) {
		next = GS_QUIT;
		return;
	}

	animTimer += dt;
	entranceTimer += dt;

	// Update confetti
	for (int i = 0; i < NUM_CONFETTI; i++) {
		confetti[i].x += (confetti[i].vx + sinf(animTimer * 2.0f + confetti[i].x * 0.01f) * 40.0f) * dt;
		confetti[i].y += confetti[i].vy * dt;
		confetti[i].rot += confetti[i].rotSpeed * dt;
		if (confetti[i].y < -500.0f)
			SpawnConfetti(confetti[i], false);
	}

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
	AEGfxSetBackgroundColor(0.05f, 0.08f, 0.15f);

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// Confetti (behind panel)
	for (int i = 0; i < NUM_CONFETTI; i++) {
		DrawMesh(rectMesh, confetti[i].size, confetti[i].size,
		         confetti[i].x - confetti[i].size * 0.5f, confetti[i].y,
		         confetti[i].rot,
		         confetti[i].r, confetti[i].g, confetti[i].b, 200);
	}

	float panelEase = Smoothstep(entranceTimer * 2.0f);

	// Outer glow (pulsing golden)
	float glowPulse = 0.5f + 0.5f * sinf(animTimer * 1.5f);
	DrawMesh(rectMesh, 920, 420, -460, 0, 0, 255, 200, 50, (20 + glowPulse * 25) * panelEase);

	// Panel border
	DrawMesh(rectMesh, 904, 404, -452, 0, 0, 80, 50, 120, 255 * panelEase);

	// Panel fill
	DrawMesh(rectMesh, 900, 400, -450, 0, 0, 20, 15, 35, 220 * panelEase);

	// Text
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// Title with bob + entrance
	{
		float titleEase = Smoothstep(entranceTimer * 2.5f);
		float bobY = 100.0f + sinf(animTimer * 2.0f) * 15.0f;
		bobY += (1.0f - titleEase) * 40.0f;
		const char* title = "Congratulations!";
		float tw, th;
		AEGfxGetPrintSize(fontTitle, title, 1.0f, &tw, &th);
		AEGfxPrint(fontTitle, title, -tw * 0.5f, bobY / 450.0f, 1.0f, 1.0f, 0.85f, 0.0f, titleEase);
	}

	// Subtitle with entrance
	{
		float subEase = Smoothstep((entranceTimer - 0.1f) * 2.5f);
		const char* sub = "You Win!";
		float tw, th;
		AEGfxGetPrintSize(fontTitle, sub, 1.0f, &tw, &th);
		float subY = (1.0f - subEase) * 20.0f;
		AEGfxPrint(fontTitle, sub, -tw * 0.5f, subY / 450.0f, 1.0f, 1.0f, 1.0f, 1.0f, subEase);
	}

	// Prompt with entrance + pulse
	{
		float promptEase = Smoothstep((entranceTimer - 0.3f) * 2.5f);
		const char* prompt = "Click or press any key to continue...";
		float tw, th;
		AEGfxGetPrintSize(fontBody, prompt, 1.0f, &tw, &th);
		float alpha = (sinf(animTimer * 3.0f) + 1.0f) * 0.5f * promptEase;
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
