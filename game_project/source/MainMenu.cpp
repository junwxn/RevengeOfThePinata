#include "pch.h"
#include "MainMenu.h"
#include "GameStateManager.h"
#include "Utils.h"
#include "Audio.h"
#include "AugmentData.h"
#include "EventSystem.h"
#include "Player.h"

// --- Enums & Structs ---
enum MenuScreen { MENU_MAIN, MENU_CONTROLS, MENU_CREDITS };

struct Button {
	float x, y, w, h;
	const char* label;
	bool hovered;
	float hoverT;
};

// --- File-scoped statics ---
static AEGfxVertexList* rectMesh = nullptr;
static s8 fontTitle = -1;
static s8 fontBody  = -1;

static MenuScreen menuScreen;
static Button mainButtons[4];
static Button backButton;
static Button muteButton;
static float titleBob;
static float bgHue;
static float entranceTimer;

// --- Helpers ---
static float Clamp01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
static float Smoothstep(float t) { t = Clamp01(t); return t * t * (3.0f - 2.0f * t); }
static float Lerp(float a, float b, float t) { return a + (b - a) * t; }

static void HsvToRgb(float h, float s, float v, float& r, float& g, float& b) {
	float c  = v * s;
	float hp = fmodf(h / 60.0f, 6.0f);
	float x  = c * (1.0f - fabsf(fmodf(hp, 2.0f) - 1.0f));
	float m  = v - c;
	float r1 = 0, g1 = 0, b1 = 0;
	if      (hp < 1) { r1 = c; g1 = x; }
	else if (hp < 2) { r1 = x; g1 = c; }
	else if (hp < 3) { g1 = c; b1 = x; }
	else if (hp < 4) { g1 = x; b1 = c; }
	else if (hp < 5) { r1 = x; b1 = c; }
	else             { r1 = c; b1 = x; }
	r = r1 + m;
	g = g1 + m;
	b = b1 + m;
}

static bool IsInside(float wx, float wy, const Button& btn) {
	return wx >= btn.x - btn.w * 0.5f && wx <= btn.x + btn.w * 0.5f &&
	       wy >= btn.y - btn.h * 0.5f && wy <= btn.y + btn.h * 0.5f;
}

static void DrawStyledButton(float cx, float cy, float baseW, float baseH, float hT, float alpha) {
	float w = baseW * (1.0f + 0.05f * hT);
	float h = baseH * (1.0f + 0.05f * hT);
	float fr = Lerp(180, 255, hT), fg = Lerp(45, 190, hT), fb = Lerp(110, 50, hT);
	float br = Lerp(120, 200, hT), bg = Lerp(30, 150, hT), bb = Lerp(75, 30, hT);
	// Shadow
	DrawMesh(rectMesh, w, h, cx - w * 0.5f + 3, cy - 3, 0, 0, 0, 0, 60 * alpha);
	// Border
	DrawMesh(rectMesh, w + 4, h + 4, cx - (w + 4) * 0.5f, cy, 0, br, bg, bb, 255 * alpha);
	// Fill
	DrawMesh(rectMesh, w, h, cx - w * 0.5f, cy, 0, fr, fg, fb, 240 * alpha);
	// Top highlight
	DrawMesh(rectMesh, w - 8, 2, cx - (w - 8) * 0.5f, cy + h * 0.5f - 3, 0, 255, 255, 255, (30 + hT * 30) * alpha);
}

static void DrawPanel(float cx, float cy, float w, float h, float alpha) {
	// Border
	DrawMesh(rectMesh, w + 4, h + 4, cx - (w + 4) * 0.5f, cy, 0, 80, 50, 120, 255 * alpha);
	// Fill
	DrawMesh(rectMesh, w, h, cx - w * 0.5f, cy, 0, 20, 15, 35, 220 * alpha);
}

// ========== LOAD ==========
void MainMenu_Load() {
	rectMesh  = CreateRectMesh(0xFFFFFFFF);
	fontTitle = AEGfxCreateFont("Assets/liberation-mono.ttf", 72);
	fontBody  = AEGfxCreateFont("Assets/liberation-mono.ttf", 36);
	//gAudio.Audio_Init();
}

// ========== INIT ==========
void MainMenu_Init() {
	g_Augments.Reset();
	g_Events.ClearAll();
	menuScreen = MENU_MAIN;
	AEGfxSetCamPosition(0.0f, 0.0f);

	const char* labels[] = { "Play", "Controls", "Credits", "Quit" };
	for (int i = 0; i < 4; i++) {
		mainButtons[i].x       = 0.0f;
		mainButtons[i].y       = -20.0f - 85.0f * i;
		mainButtons[i].w       = 320.0f;
		mainButtons[i].h       = 60.0f;
		mainButtons[i].label   = labels[i];
		mainButtons[i].hovered = false;
		mainButtons[i].hoverT  = 0.0f;
	}

	backButton.x       = 0.0f;
	backButton.y       = -350.0f;
	backButton.w       = 240.0f;
	backButton.h       = 55.0f;
	backButton.label   = "Back";
	backButton.hovered = false;
	backButton.hoverT  = 0.0f;

	muteButton.x       = -700.0f;
	muteButton.y       = -380.0f;
	muteButton.w       = 160.0f;
	muteButton.h       = 50.0f;
	muteButton.label   = gAudio.IsMuted() ? "Unmute" : "Mute";
	muteButton.hovered = false;
	muteButton.hoverT  = 0.0f;

	titleBob = 0.0f;
	bgHue    = 0.0f;

	//AEAudio bgm = AEAudioLoadMusic("Assets/Audio/BGM/mainMenu.wav");
	//AEAudioGroup bgmGroup = AEAudioCreateGroup();
	//AEAudioPlay(bgm, bgmGroup, 1.f, 1.f, -1);
	gAudio.PlayBGM(BGM_MAINMENU);
	entranceTimer = 0.0f;
}

// ========== UPDATE ==========
void MainMenu_Update(float dt) {
	
	// Mouse → world coords (window is 1600x900, origin at center)
	// Mouse -> world coords
	s32 mx, my;
	AEInputGetCursorPosition(&mx, &my);
	float worldX = (float)mx - 800.0f;
	float worldY = 450.0f - (float)my;

	// Hover detection
	if (menuScreen == MENU_MAIN) {
		for (int i = 0; i < 4; i++)
			mainButtons[i].hovered = IsInside(worldX, worldY, mainButtons[i]);
		backButton.hovered = false;
	}
	else {
		for (int i = 0; i < 4; i++)
			mainButtons[i].hovered = false;
		backButton.hovered = IsInside(worldX, worldY, backButton);
	}

	// Mute button hover (always active regardless of screen)
	muteButton.hovered = IsInside(worldX, worldY, muteButton);

	// Smooth hover transitions
	for (int i = 0; i < 4; i++) {
		mainButtons[i].hoverT += (mainButtons[i].hovered ? 1.0f : -1.0f) * dt * 6.0f;
		mainButtons[i].hoverT = Clamp01(mainButtons[i].hoverT);
	}
	backButton.hoverT += (backButton.hovered ? 1.0f : -1.0f) * dt * 6.0f;
	backButton.hoverT = Clamp01(backButton.hoverT);
	muteButton.hoverT += (muteButton.hovered ? 1.0f : -1.0f) * dt * 6.0f;
	muteButton.hoverT = Clamp01(muteButton.hoverT);

	// Click handling
	if (AEInputCheckTriggered(AEVK_LBUTTON)) {
		if (muteButton.hovered) {
			gAudio.ToggleMute();
			muteButton.label = gAudio.IsMuted() ? "Unmute" : "Mute";
		}
		if (menuScreen == MENU_MAIN) {
			if (mainButtons[0].hovered) { g_PlayerAttackCharges = 5; next = GS_LEVEL1; }
			if (mainButtons[1].hovered) menuScreen = MENU_CONTROLS;
			if (mainButtons[2].hovered) menuScreen = MENU_CREDITS;
			if (mainButtons[3].hovered) next = GS_QUIT;
		}
		else {
			if (backButton.hovered) menuScreen = MENU_MAIN;
		}
	}

	// ESC: back from sub-screens
	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		if (menuScreen != MENU_MAIN)
			menuScreen = MENU_MAIN;
	}

	if (!AESysDoesWindowExist())
		next = GS_QUIT;

	// Animation
	titleBob += dt;
	bgHue += dt * 20.0f;
	if (bgHue >= 360.0f) bgHue -= 360.0f;
	entranceTimer += dt;
}

// ========== DRAW ==========
void MainMenu_Draw() {
	AESysFrameStart();

	// Background: slow HSV color cycle
	float bgR, bgG, bgB;
	HsvToRgb(bgHue, 0.35f, 0.25f, bgR, bgG, bgB);
	AEGfxSetBackgroundColor(bgR, bgG, bgB);

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	if (menuScreen == MENU_MAIN) {
		// Panel behind buttons
		float panelEase = Smoothstep(entranceTimer * 2.5f);
		DrawPanel(0, -150, 400, 380, panelEase);

		// Styled buttons with entrance stagger
		for (int i = 0; i < 4; i++) {
			float delay = i * 0.08f;
			float ease = Smoothstep((entranceTimer - delay) * 2.5f);
			float yOff = (1.0f - ease) * 30.0f;
			DrawStyledButton(mainButtons[i].x, mainButtons[i].y - yOff,
			                 mainButtons[i].w, mainButtons[i].h,
			                 mainButtons[i].hoverT, ease);
		}
	}
	else {
		// Sub-screen: dark overlay + bordered panel
		DrawMesh(rectMesh, 1600, 900, -800, 0, 0, 0, 0, 0, 180);
		DrawPanel(0, -50, 500, 450, 1.0f);

		// Back button (styled)
		DrawStyledButton(backButton.x, backButton.y,
		                 backButton.w, backButton.h,
		                 backButton.hoverT, 1.0f);
	}

	// --- Text ---
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// Title: "Revenge of the Pinata" in gold with sine bob
	{
		float bobY = 200.0f + sinf(titleBob * 2.0f) * 15.0f;
		const char* title = "Revenge of the Pinata";
		float tw, th;
		AEGfxGetPrintSize(fontTitle, title, 1.0f, &tw, &th);
		AEGfxPrint(fontTitle, title, -tw * 0.5f, bobY / 450.0f, 1.0f, 1.0f, 0.85f, 0.0f, 1.0f);
	}

	// Subtitle (main screen only)
	if (menuScreen == MENU_MAIN) {
		float subEase = Smoothstep(entranceTimer * 2.0f);
		const char* sub = "";
		float tw, th;
		AEGfxGetPrintSize(fontBody, sub, 0.7f, &tw, &th);
		AEGfxPrint(fontBody, sub, -tw * 0.5f, 155.0f / 450.0f, 0.7f, 0.6f, 0.5f, 0.7f, subEase);
	}

	if (menuScreen == MENU_MAIN) {
		// Button labels with entrance stagger
		for (int i = 0; i < 4; i++) {
			float delay = i * 0.08f;
			float ease = Smoothstep((entranceTimer - delay) * 2.5f);
			float yOff = (1.0f - ease) * 30.0f;
			float tw, th;
			AEGfxGetPrintSize(fontBody, mainButtons[i].label, 1.0f, &tw, &th);
			float nx = mainButtons[i].x / 800.0f - tw * 0.5f;
			float ny = (mainButtons[i].y - yOff) / 450.0f - th * 0.5f;
			AEGfxPrint(fontBody, mainButtons[i].label, nx, ny, 1.0f, 1.0f, 1.0f, 1.0f, ease);
		}
	}
	else if (menuScreen == MENU_CONTROLS) {
		const char* lines[] = {
			"WASD - Move",
			"Space - Dash",
			"LMB - Attack",
			"RMB - Block / Parry",
			"Mouse - Aim"
		};
		for (int i = 0; i < 5; i++) {
			float tw, th;
			float ly = 120.0f - i * 60.0f;
			AEGfxGetPrintSize(fontBody, lines[i], 1.0f, &tw, &th);
			AEGfxPrint(fontBody, lines[i], -tw * 0.5f, ly / 450.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		}
		// Back button label
		float tw, th;
		AEGfxGetPrintSize(fontBody, backButton.label, 1.0f, &tw, &th);
		AEGfxPrint(fontBody, backButton.label,
		           backButton.x / 800.0f - tw * 0.5f,
		           backButton.y / 450.0f - th * 0.5f,
		           1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	}
	else if (menuScreen == MENU_CREDITS) {
		const char* lines[] = {
			"GAM150 Team Project",
			"Team Members",
			"Made with AlphaEngine"
		};
		for (int i = 0; i < 3; i++) {
			float tw, th;
			float ly = 120.0f - i * 70.0f;
			AEGfxGetPrintSize(fontBody, lines[i], 1.0f, &tw, &th);
			AEGfxPrint(fontBody, lines[i], -tw * 0.5f, ly / 450.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		}
		// Back button label
		float tw, th;
		AEGfxGetPrintSize(fontBody, backButton.label, 1.0f, &tw, &th);
		AEGfxPrint(fontBody, backButton.label,
		           backButton.x / 800.0f - tw * 0.5f,
		           backButton.y / 450.0f - th * 0.5f,
		           1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	}

	// Mute button (always visible)
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	DrawStyledButton(muteButton.x, muteButton.y,
	                 muteButton.w, muteButton.h,
	                 muteButton.hoverT, 1.0f);
	{
		float tw, th;
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxGetPrintSize(fontBody, muteButton.label, 1.0f, &tw, &th);
		AEGfxPrint(fontBody, muteButton.label,
		           muteButton.x / 800.0f - tw * 0.5f,
		           muteButton.y / 450.0f - th * 0.5f,
		           1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	}

	AESysFrameEnd();
}

// ========== FREE ==========
void MainMenu_Free() {
}

// ========== UNLOAD ==========
void MainMenu_Unload() {
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
	AEAudioStopGroup(gAudio.audioGroup.BGM);
}
