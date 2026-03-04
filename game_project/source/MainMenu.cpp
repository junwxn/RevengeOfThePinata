#include "pch.h"
#include "MainMenu.h"
#include "GameStateManager.h"
#include "Utils.h"
#include "Audio.h"

// --- Enums & Structs ---
enum MenuScreen { MENU_MAIN, MENU_CONTROLS, MENU_CREDITS };

struct Button {
	float x, y, w, h;
	const char* label;
	bool hovered;
};

// --- File-scoped statics ---
static AEGfxVertexList* rectMesh = nullptr;
static s8 fontTitle = -1;
static s8 fontBody  = -1;

static MenuScreen menuScreen;
static Button mainButtons[4];
static Button backButton;
static float titleBob;
static float bgHue;

// --- HSV to RGB (h: 0-360, s/v: 0-1, output r/g/b: 0-1) ---
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

// ========== LOAD ==========
void MainMenu_Load() {
	rectMesh  = CreateRectMesh(0xFFFFFFFF);
	fontTitle = AEGfxCreateFont("Assets/liberation-mono.ttf", 72);
	fontBody  = AEGfxCreateFont("Assets/liberation-mono.ttf", 36);
	gAudio.Audio_Init();
}

// ========== INIT ==========
void MainMenu_Init() {
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
	}

	backButton.x       = 0.0f;
	backButton.y       = -350.0f;
	backButton.w       = 240.0f;
	backButton.h       = 55.0f;
	backButton.label   = "Back";
	backButton.hovered = false;

	titleBob = 0.0f;
	bgHue    = 0.0f;

	//AEAudio bgm = AEAudioLoadMusic("Assets/Audio/BGM/mainMenu.wav");
	//AEAudioGroup bgmGroup = AEAudioCreateGroup();
	//AEAudioPlay(bgm, bgmGroup, 1.f, 1.f, -1);
	gAudio.PlayBGM(1);
}

// ========== UPDATE ==========
void MainMenu_Update(float dt) {
	
	// Mouse → world coords (window is 1600x900, origin at center)
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

	// Click handling
	if (AEInputCheckTriggered(AEVK_LBUTTON)) {
		if (menuScreen == MENU_MAIN) {
			if (mainButtons[0].hovered) next = GS_LEVEL1;
			if (mainButtons[1].hovered) menuScreen = MENU_CONTROLS;
			if (mainButtons[2].hovered) menuScreen = MENU_CREDITS;
			if (mainButtons[3].hovered) next = GS_QUIT;
		}
		else {
			if (backButton.hovered) menuScreen = MENU_MAIN;
		}
	}

	// ESC: quit from main, back from sub-screens
	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		if (menuScreen == MENU_MAIN)
			next = GS_QUIT;
		else
			menuScreen = MENU_MAIN;
	}

	if (!AESysDoesWindowExist())
		next = GS_QUIT;

	// Animation
	titleBob += dt;
	bgHue += dt * 20.0f;
	if (bgHue >= 360.0f) bgHue -= 360.0f;
}

// ========== DRAW ==========
void MainMenu_Draw() {
	AESysFrameStart();

	// --- Background: slow HSV color cycle ---
	float bgR, bgG, bgB;
	HsvToRgb(bgHue, 0.35f, 0.25f, bgR, bgG, bgB);
	AEGfxSetBackgroundColor(bgR, bgG, bgB);

	// --- Mesh draws (buttons / panels) ---
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	if (menuScreen == MENU_MAIN) {
		// Main buttons: magenta normal, golden hover
		for (int i = 0; i < 4; i++) {
			float w = mainButtons[i].w;
			float h = mainButtons[i].h;
			float cr = 200, cg = 50, cb = 120;
			if (mainButtons[i].hovered) {
				w *= 1.05f; h *= 1.05f;
				cr = 255; cg = 200; cb = 50;
			}
			DrawMesh(rectMesh, w, h, mainButtons[i].x - w * 0.5f, mainButtons[i].y, 0, cr, cg, cb, 255);
		}
	}
	else {
		// Dark overlay covering the full screen
		DrawMesh(rectMesh, 1600, 900, -800, 0, 0, 0, 0, 0, 180);

		// Back button: green normal, bright green hover
		float w = backButton.w;
		float h = backButton.h;
		float cr = 80, cg = 180, cb = 80;
		if (backButton.hovered) {
			w *= 1.05f; h *= 1.05f;
			cr = 120; cg = 255; cb = 120;
		}
		DrawMesh(rectMesh, w, h, backButton.x - w * 0.5f, backButton.y, 0, cr, cg, cb, 255);
	}

	// --- Text (AEGfxPrint after all mesh draws) ---
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// Title: "Revenge of the Pinata" in gold with sine bob
	{
		float bobY = 200.0f + sinf(titleBob * 2.0f) * 15.0f;
		const char* title = "Revenge of the Pinata";
		float tw, th;
		AEGfxGetPrintSize(fontTitle, title, 1.0f, &tw, &th);
		AEGfxPrint(fontTitle, title, -tw * 0.5f, bobY / 450.0f, 1.0f, 1.0f, 0.85f, 0.0f, 1.0f);
	}

	if (menuScreen == MENU_MAIN) {
		// Button labels (white, centered on each button)
		for (int i = 0; i < 4; i++) {
			float tw, th;
			AEGfxGetPrintSize(fontBody, mainButtons[i].label, 1.0f, &tw, &th);
			float nx = mainButtons[i].x / 800.0f - tw * 0.5f;
			float ny = mainButtons[i].y / 450.0f - th * 0.5f;
			AEGfxPrint(fontBody, mainButtons[i].label, nx, ny, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
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
}
