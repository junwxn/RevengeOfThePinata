#include "pch.h"
#include "Pause.h"
#include "GameStateManager.h"
#include "Utils.h"
#include "Audio.h"
#include "Player.h"
#include "Transition.h"

// --- Button struct ---
struct PauseButton {
	float x, y, w, h;
	const char* label;
	bool hovered;
	float hoverT;
};

// --- File-scoped statics ---
static AEGfxVertexList* rectMesh = nullptr;
static s8 pauseFont = -1;
static bool paused = false;
static bool wasPaused = false;
static PauseButton buttons[3];
static PauseButton muteButton;
static float entranceTimer = 0.0f;
static float pauseAnimTimer = 0.0f;

// --- Helpers ---
static float Clamp01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
static float Smoothstep(float t) { t = Clamp01(t); return t * t * (3.0f - 2.0f * t); }
static float Lerp(float a, float b, float t) { return a + (b - a) * t; }

static bool IsInside(float wx, float wy, const PauseButton& b) {
	return wx >= b.x - b.w * 0.5f && wx <= b.x + b.w * 0.5f &&
	       wy >= b.y - b.h * 0.5f && wy <= b.y + b.h * 0.5f;
}

static void DrawStyledButton(float cx, float cy, float baseW, float baseH, float hT, float alpha, float camX, float camY) {
	float w = baseW * (1.0f + 0.05f * hT);
	float h = baseH * (1.0f + 0.05f * hT);
	float fr = Lerp(180, 255, hT), fg = Lerp(45, 190, hT), fb = Lerp(110, 50, hT);
	float br = Lerp(120, 200, hT), bg = Lerp(30, 150, hT), bb = Lerp(75, 30, hT);
	// Shadow
	DrawMesh(rectMesh, w, h, cx - w * 0.5f + 3 + camX, cy - 3 + camY, 0, 0, 0, 0, 60 * alpha);
	// Border
	DrawMesh(rectMesh, w + 4, h + 4, cx - (w + 4) * 0.5f + camX, cy + camY, 0, br, bg, bb, 255 * alpha);
	// Fill
	DrawMesh(rectMesh, w, h, cx - w * 0.5f + camX, cy + camY, 0, fr, fg, fb, 240 * alpha);
	// Top highlight
	DrawMesh(rectMesh, w - 8, 2, cx - (w - 8) * 0.5f + camX, cy + h * 0.5f - 3 + camY, 0, 255, 255, 255, (30 + hT * 30) * alpha);
}

void Pause_Load() {
	rectMesh  = CreateRectMesh(0xFFFFFFFF);
	pauseFont = AEGfxCreateFont("Assets/fonts/Stick-Regular.ttf", 36);
}

void Pause_Init() {
	paused = false;
	wasPaused = false;
	entranceTimer = 0.0f;
	pauseAnimTimer = 0.0f;
	const char* labels[] = { "Resume", "Main Menu", "Restart" };
	for (int i = 0; i < 3; i++) {
		buttons[i].x       = 0.0f;
		buttons[i].y       = 20.0f - 80.0f * i;
		buttons[i].w       = 300.0f;
		buttons[i].h       = 60.0f;
		buttons[i].label   = labels[i];
		buttons[i].hovered = false;
		buttons[i].hoverT  = 0.0f;
	}

	muteButton.x       = -700.0f;
	muteButton.y       = -380.0f;
	muteButton.w       = 160.0f;
	muteButton.h       = 50.0f;
	muteButton.label   = gAudio.IsMuted() ? "Unmute" : "Mute";
	muteButton.hovered = false;
	muteButton.hoverT  = 0.0f;
}

bool Pause_Update(bool isPlayerAlive) {
	// Toggle pause
	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		paused = !paused;
	}
	if (!isPlayerAlive) paused = true;

	// Reset entrance timer when pause is newly activated
	if (paused && !wasPaused) {
		entranceTimer = 0.0f;
		for (int i = 0; i < 3; i++)
			buttons[i].hoverT = 0.0f;
	}
	wasPaused = paused;

	if (!paused) return false;

	float dt = AEFrameRateControllerGetFrameTime();
	entranceTimer += dt;
	pauseAnimTimer += dt;

	// Mouse -> world coords (window 1600x900, origin at center)
	s32 mx, my;
	AEInputGetCursorPosition(&mx, &my);
	float worldX = (float)mx - 800.0f;
	float worldY = 450.0f - (float)my;

	for (int i = 0; i < 3; i++)
		buttons[i].hovered = IsInside(worldX, worldY, buttons[i]);
	muteButton.hovered = IsInside(worldX, worldY, muteButton);

	// Smooth hover transitions
	for (int i = 0; i < 3; i++) {
		buttons[i].hoverT += (buttons[i].hovered ? 1.0f : -1.0f) * dt * 6.0f;
		buttons[i].hoverT = Clamp01(buttons[i].hoverT);
	}
	muteButton.hoverT += (muteButton.hovered ? 1.0f : -1.0f) * dt * 6.0f;
	muteButton.hoverT = Clamp01(muteButton.hoverT);

	if (AEInputCheckTriggered(AEVK_LBUTTON)) {
		if (muteButton.hovered) {
			gAudio.ToggleMute();
			muteButton.label = gAudio.IsMuted() ? "Unmute" : "Mute";
		}
		if (buttons[0].hovered) paused = false;        // Resume
		if (buttons[1].hovered) Transition_Start(GS_MAINMENU);    // Main Menu
		if (buttons[2].hovered) { g_PlayerAttackCharges = DEFAULT_ATTACK_CHARGES; Transition_Start(static_cast<GS_STATES>(current)); }     // Restart
	}

	return true;
}

void Pause_Draw(float camX, float camY) {
	if (!paused) return;

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// Helper: draw mesh at screen-space position, offset by camera so it stays fixed
	auto drawPause = [&](float w, float h, float x, float y, float rot, float r, float g, float b, float a) {
		DrawMesh(rectMesh, w, h, x + camX, y + camY, rot, r, g, b, a);
	};

	float panelEase = Smoothstep(entranceTimer * 3.0f);

	// Dark overlay (slightly darker than before)
	drawPause(1600, 900, -800, 0, 0, 0, 0, 0, 190 * panelEase);

	// Card panel behind buttons
	float panelW = 420.0f, panelH = 380.0f;
	// Panel border
	drawPause(panelW + 4, panelH + 4, -(panelW + 4) * 0.5f, -30, 0, 80, 50, 120, 255 * panelEase);
	// Panel fill
	drawPause(panelW, panelH, -panelW * 0.5f, -30, 0, 20, 15, 35, 220 * panelEase);

	// Styled buttons with entrance stagger
	for (int i = 0; i < 3; i++) {
		float delay = i * 0.08f + 0.1f;
		float ease = Smoothstep((entranceTimer - delay) * 3.0f);
		float yOff = (1.0f - ease) * 30.0f;
		DrawStyledButton(buttons[i].x, buttons[i].y - yOff,
		                 buttons[i].w, buttons[i].h,
		                 buttons[i].hoverT, ease, camX, camY);
	}

	// Text
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// "PAUSED" title with subtle bob animation
	float titleBob = sinf(pauseAnimTimer * 1.5f) * 5.0f;
	float titleEase = Smoothstep(entranceTimer * 3.0f);
	float tw, th;
	AEGfxGetPrintSize(pauseFont, "PAUSED", 2.0f, &tw, &th);
	AEGfxPrint(pauseFont, "PAUSED", -tw * 0.5f, (200.0f + titleBob) / 450.0f, 2.0f, 1.0f, 0.85f, 0.0f, titleEase);

	// Button labels with entrance stagger
	for (int i = 0; i < 3; i++) {
		float delay = i * 0.08f + 0.1f;
		float ease = Smoothstep((entranceTimer - delay) * 3.0f);
		float yOff = (1.0f - ease) * 30.0f;
		AEGfxGetPrintSize(pauseFont, buttons[i].label, 1.0f, &tw, &th);
		float nx = buttons[i].x / 800.0f - tw * 0.5f;
		float ny = (buttons[i].y - yOff) / 450.0f - th * 0.5f;
		AEGfxPrint(pauseFont, buttons[i].label, nx, ny, 1.0f, 1.0f, 1.0f, 1.0f, ease);
	}

	// Mute button
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	DrawStyledButton(muteButton.x, muteButton.y,
	                 muteButton.w, muteButton.h,
	                 muteButton.hoverT, panelEase, camX, camY);
	{
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxGetPrintSize(pauseFont, muteButton.label, 1.0f, &tw, &th);
		AEGfxPrint(pauseFont, muteButton.label,
		           muteButton.x / 800.0f - tw * 0.5f,
		           muteButton.y / 450.0f - th * 0.5f,
		           1.0f, 1.0f, 1.0f, 1.0f, panelEase);
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
