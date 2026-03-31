#include "pch.h"
#include "MainMenu.h"
#include "GameStateManager.h"
#include "Utils.h"
#include "Audio.h"
#include "AugmentData.h"
#include "EventSystem.h"
#include "Player.h"
#include "Transition.h"
#include "SaveSystem.h"

// --- Enums & Structs ---
enum MenuScreen { MENU_MAIN, MENU_CONTROLS, MENU_CREDITS, MENU_TUTORIAL_PROMPT, MENU_SETTINGS, MENU_CONFIRMATION };

struct Button {
	float x, y, w, h;
	const char* label;
	bool hovered;
	float hoverT;
};

struct Slider {
	float x, y;  // Slider pos
	float w, h;  // Width and height of slider
	float min, max; // Min and max values for the slider (max 1.0)
	float value; // Current value of the slider
	bool dragging; // Whether the slider is currently being dragged
};
static Slider BGMvolumeSlider; // Declare BGM slider
static Slider GeneralSFXvolumeSlider; // Declare General SFX slider
static Slider CombatSFXvolumeSlider; // Declare Combat SFX slider
static Slider PlayerSFXvolumeSlider; // Declare Player SFX slider
static Slider EnemySFXvolumeSlider; // Declare Enemy SFX slider

// --- File-scoped statics ---
static AEGfxVertexList* rectMesh = nullptr;
static s8 fontTitle = -1;
static s8 fontBody = -1;

static MenuScreen menuScreen;
static Button mainButtons[6];
static bool hasSaveFile = false;
static int mainButtonCount = 5;
static Button backButton;
static Button yesButton;
static Button noButton;
static Button muteButton;
static float titleBob;
static float bgHue;
static float entranceTimer;
static float BGMVolume = 1.0f;
static float GeneralSFXVolume = 1.0f;
static float CombatSFXVolume = 1.0f;
static float PlayerSFXVolume = 1.0f;
static float EnemySFXVolume = 1.0f;

static Sprite logoSprite;
static AEGfxTexture* digipenLogo = nullptr;
static AEGfxVertexList* digipenLogoMesh = nullptr;

// --- Helpers ---
static float Clamp01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
static float Smoothstep(float t) { t = Clamp01(t); return t * t * (3.0f - 2.0f * t); }
static float Lerp(float a, float b, float t) { return a + (b - a) * t; }

static void HsvToRgb(float h, float s, float v, float& r, float& g, float& b) {
	float c = v * s;
	float hp = fmodf(h / 60.0f, 6.0f);
	float x = c * (1.0f - fabsf(fmodf(hp, 2.0f) - 1.0f));
	float m = v - c;
	float r1 = 0, g1 = 0, b1 = 0;
	if (hp < 1) { r1 = c; g1 = x; }
	else if (hp < 2) { r1 = x; g1 = c; }
	else if (hp < 3) { g1 = c; b1 = x; }
	else if (hp < 4) { g1 = x; b1 = c; }
	else if (hp < 5) { r1 = x; b1 = c; }
	else { r1 = c; b1 = x; }
	r = r1 + m;
	g = g1 + m;
	b = b1 + m;
}

static bool IsInside(float wx, float wy, const Button& btn) {
	return wx >= btn.x - btn.w * 0.5f && wx <= btn.x + btn.w * 0.5f &&
		wy >= btn.y - btn.h * 0.5f && wy <= btn.y + btn.h * 0.5f;
}

static bool IsInsideSlider(float wx, float wy, const Slider& s) {
	return wx >= s.x - s.w * 0.5f && wx <= s.x + s.w * 0.5f &&
		wy >= s.y - s.h * 0.5f && wy <= s.y + s.h * 0.5f;
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
	rectMesh = CreateRectMesh(0xFFFFFFFF);
	digipenLogoMesh = CreateSpriteRectMesh(0xFFFFFFFF, 1.0f, 1.0f);
	fontTitle = AEGfxCreateFont("Assets/fonts/Stick-Regular.ttf", 145);
	fontBody = AEGfxCreateFont("Assets/fonts/Stick-Regular.ttf", 36);
	digipenLogo = AEGfxTextureLoad("Assets/DigiPen_BLACK.png");
	//gAudio.Audio_Init();
}

// ========== INIT ==========
void MainMenu_Init() {
	menuScreen = MENU_MAIN;
	AEGfxSetCamPosition(0.0f, 0.0f);

	hasSaveFile = SaveSystem_HasSave();

	const char* labels6[] = { "Continue", "Play", "Controls", "Credits", "Settings", "Quit" };
	const char* labels5[] = { "Play", "Controls", "Credits", "Settings", "Quit" };
	const char** labels = hasSaveFile ? labels6 : labels5;
	mainButtonCount = hasSaveFile ? 6 : 5;

	float startY = hasSaveFile ? 60.0f : -20.0f;
	for (int i = 0; i < mainButtonCount; i++) {
		mainButtons[i].x       = 0.0f;
		mainButtons[i].y       = startY - 85.0f * i;
		mainButtons[i].w       = 320.0f;
		mainButtons[i].h       = 60.0f;
		mainButtons[i].label   = labels[i];
		mainButtons[i].hovered = false;
		mainButtons[i].hoverT = 0.0f;
	}

	backButton.x = 0.0f;
	backButton.y = -350.0f;
	backButton.w = 240.0f;
	backButton.h = 55.0f;
	backButton.label = "Back";
	backButton.hovered = false;
	backButton.hoverT = 0.0f;

	yesButton.x = -110.0f;
	yesButton.y = -120.0f;
	yesButton.w = 160.0f;
	yesButton.h = 55.0f;
	yesButton.label = "Yes";
	yesButton.hovered = false;
	yesButton.hoverT = 0.0f;

	noButton.x = 110.0f;
	noButton.y = -120.0f;
	noButton.w = 160.0f;
	noButton.h = 55.0f;
	noButton.label = "No";
	noButton.hovered = false;
	noButton.hoverT = 0.0f;

	muteButton.x = -700.0f;
	muteButton.y = -380.0f;
	muteButton.w = 160.0f;
	muteButton.h = 50.0f;
	muteButton.label = gAudio.IsMuted() ? "Unmute" : "Mute";
	muteButton.hovered = false;
	muteButton.hoverT = 0.0f;

	titleBob = 0.0f;
	bgHue = 0.0f;

	//AEAudio bgm = AEAudioLoadMusic("Assets/Audio/BGM/mainMenu.wav");
	//AEAudioGroup bgmGroup = AEAudioCreateGroup();
	//AEAudioPlay(bgm, bgmGroup, 1.f, 1.f, -1);
	gAudio.PlayBGM(BGM_MAINMENU);
	entranceTimer = 0.0f;

	// Slider initialization
	BGMvolumeSlider.x = 0.0f;
	BGMvolumeSlider.y = 125.0f;
	BGMvolumeSlider.w = 400.0f;  // Width of the slider bar
	BGMvolumeSlider.h = 20.0f;   // Height of the slider bar
	BGMvolumeSlider.min = 0.0f;
	BGMvolumeSlider.max = 1.0f;
	BGMvolumeSlider.value = BGMVolume; // Initialize with current BGM volume
	BGMvolumeSlider.dragging = false;

	GeneralSFXvolumeSlider.x = 0.0f;
	GeneralSFXvolumeSlider.y = 25.0f;
	GeneralSFXvolumeSlider.w = 400.0f;
	GeneralSFXvolumeSlider.h = 20.0f;
	GeneralSFXvolumeSlider.min = 0.0f;
	GeneralSFXvolumeSlider.max = 1.0f;
	GeneralSFXvolumeSlider.value = GeneralSFXVolume;
	GeneralSFXvolumeSlider.dragging = false;

	CombatSFXvolumeSlider.x = 0.0f;
	CombatSFXvolumeSlider.y = -75.0f;
	CombatSFXvolumeSlider.w = 400.0f;
	CombatSFXvolumeSlider.h = 20.0f;
	CombatSFXvolumeSlider.min = 0.0f;
	CombatSFXvolumeSlider.max = 1.0f;
	CombatSFXvolumeSlider.value = CombatSFXVolume;
	CombatSFXvolumeSlider.dragging = false;

	PlayerSFXvolumeSlider.x = 0.0f;
	PlayerSFXvolumeSlider.y = -175.0f;
	PlayerSFXvolumeSlider.w = 400.0f;
	PlayerSFXvolumeSlider.h = 20.0f;
	PlayerSFXvolumeSlider.min = 0.0f;
	PlayerSFXvolumeSlider.max = 1.0f;
	PlayerSFXvolumeSlider.value = PlayerSFXVolume;
	PlayerSFXvolumeSlider.dragging = false;

	EnemySFXvolumeSlider.x = 0.0f;
	EnemySFXvolumeSlider.y = -275.0f;
	EnemySFXvolumeSlider.w = 400.0f;
	EnemySFXvolumeSlider.h = 20.0f;
	EnemySFXvolumeSlider.min = 0.0f;
	EnemySFXvolumeSlider.max = 1.0f;
	EnemySFXvolumeSlider.value = EnemySFXVolume;
	EnemySFXvolumeSlider.dragging = false;
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
		for (int i = 0; i < mainButtonCount; i++)
			mainButtons[i].hovered = IsInside(worldX, worldY, mainButtons[i]);
		backButton.hovered = false;
		yesButton.hovered = false;
		noButton.hovered = false;
	}
	else if (menuScreen == MENU_TUTORIAL_PROMPT) {
		for (int i = 0; i < mainButtonCount; i++)
			mainButtons[i].hovered = false;
		backButton.hovered = IsInside(worldX, worldY, backButton);
		yesButton.hovered = IsInside(worldX, worldY, yesButton);
		noButton.hovered = IsInside(worldX, worldY, noButton);
	}
	else if (menuScreen == MENU_CONFIRMATION) {
		for (int i = 0; i < mainButtonCount; i++)
			mainButtons[i].hovered = false;

		backButton.hovered = false;
		yesButton.hovered = IsInside(worldX, worldY, yesButton);
		noButton.hovered = IsInside(worldX, worldY, noButton);
	}
	else if (menuScreen == MENU_SETTINGS) {
		for (int i = 0; i < mainButtonCount; i++)
			mainButtons[i].hovered = false;

		// Start dragging when mouse clicks slider
		if (AEInputCheckTriggered(AEVK_LBUTTON)) {
			if (IsInsideSlider(worldX, worldY, BGMvolumeSlider)) {
				BGMvolumeSlider.dragging = true;
			}
			if (IsInsideSlider(worldX, worldY, GeneralSFXvolumeSlider)) {
				GeneralSFXvolumeSlider.dragging = true;
			}
			if (IsInsideSlider(worldX, worldY, CombatSFXvolumeSlider)) {
				CombatSFXvolumeSlider.dragging = true;
			}
			if (IsInsideSlider(worldX, worldY, PlayerSFXvolumeSlider)) {
				PlayerSFXvolumeSlider.dragging = true;
			}
			if (IsInsideSlider(worldX, worldY, EnemySFXvolumeSlider)) {
				EnemySFXvolumeSlider.dragging = true;
			}
		}

		// Stop dragging when mouse release slider
		if (AEInputCheckReleased(AEVK_LBUTTON)) {
			BGMvolumeSlider.dragging = false;
			GeneralSFXvolumeSlider.dragging = false;
			CombatSFXvolumeSlider.dragging = false;
			PlayerSFXvolumeSlider.dragging = false;
			EnemySFXvolumeSlider.dragging = false;
		}

		backButton.hovered = IsInside(worldX, worldY, backButton);
		yesButton.hovered = false;
		noButton.hovered = false;
	}
	else {
		for (int i = 0; i < mainButtonCount; i++)
			mainButtons[i].hovered = false;
		backButton.hovered = IsInside(worldX, worldY, backButton);
		yesButton.hovered = false;
		noButton.hovered = false;
	}

	// Mute button hover (always active regardless of screen)
	muteButton.hovered = IsInside(worldX, worldY, muteButton);

	// Smooth hover transitions
	for (int i = 0; i < mainButtonCount; i++) {
		mainButtons[i].hoverT += (mainButtons[i].hovered ? 1.0f : -1.0f) * dt * 6.0f;
		mainButtons[i].hoverT = Clamp01(mainButtons[i].hoverT);
	}
	backButton.hoverT += (backButton.hovered ? 1.0f : -1.0f) * dt * 6.0f;
	backButton.hoverT = Clamp01(backButton.hoverT);
	yesButton.hoverT += (yesButton.hovered ? 1.0f : -1.0f) * dt * 6.0f;
	yesButton.hoverT = Clamp01(yesButton.hoverT);
	noButton.hoverT += (noButton.hovered ? 1.0f : -1.0f) * dt * 6.0f;
	noButton.hoverT = Clamp01(noButton.hoverT);
	muteButton.hoverT += (muteButton.hovered ? 1.0f : -1.0f) * dt * 6.0f;
	muteButton.hoverT = Clamp01(muteButton.hoverT);

	// Click handling
	if (AEInputCheckTriggered(AEVK_LBUTTON)) {
		if (muteButton.hovered) {
			gAudio.ToggleMute();
			muteButton.label = gAudio.IsMuted() ? "Unmute" : "Mute";
		}
		if (menuScreen == MENU_MAIN) {
			int idx = 0;
			if (hasSaveFile) {
				if (mainButtons[idx].hovered) {
					int savedLevel = SaveSystem_Load();
					if (savedLevel >= GS_TUTORIAL && savedLevel <= GS_BOSSLEVEL) {
						Transition_Start(static_cast<GS_STATES>(savedLevel));
					}
				}
				idx++;
			}
			if (mainButtons[idx].hovered) {
				SaveSystem_ClearSave();
				g_Augments.Reset();
				g_Events.ClearAll();
				g_PlayerAttackCharges = DEFAULT_ATTACK_CHARGES;
				menuScreen = MENU_TUTORIAL_PROMPT;
			}
			if (mainButtons[idx + 1].hovered) menuScreen = MENU_CONTROLS;
			if (mainButtons[idx + 2].hovered) menuScreen = MENU_CREDITS;
			if (mainButtons[idx + 3].hovered) menuScreen = MENU_SETTINGS;
			if (mainButtons[idx + 4].hovered) menuScreen = MENU_CONFIRMATION;
		}
		else if (menuScreen == MENU_TUTORIAL_PROMPT) {
			if (yesButton.hovered) { /*next = GS_TUTORIAL;*/ Transition_Start(GS_TUTORIAL); }
			if (noButton.hovered) { /*next = GS_LEVEL1;*/ Transition_Start(GS_LEVEL1, TransitionSheet::LEVEL1); }
			if (backButton.hovered) menuScreen = MENU_MAIN;
		}
		else if (menuScreen == MENU_CONFIRMATION) {
			if (yesButton.hovered) {
				Transition_StartImmediate(GS_QUIT);
			}
			if (noButton.hovered) {
				menuScreen = MENU_MAIN;
			}
		}
		else {
			if (backButton.hovered) menuScreen = MENU_MAIN;
		}
	}

	// ESC: back from sub-screens
	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		if (menuScreen == MENU_CONFIRMATION) {
			menuScreen = MENU_MAIN;
		}
		else if (menuScreen != MENU_MAIN) {
			menuScreen = MENU_MAIN;
		}
	}

	// Debug: T for test level
	if (AEInputCheckTriggered(AEVK_T)) {
		g_PlayerAttackCharges = DEFAULT_ATTACK_CHARGES;
		next = GS_TEST;
	}

	if (!AESysDoesWindowExist())
		Transition_StartImmediate(GS_QUIT);

	// Animation
	titleBob += dt;
	bgHue += dt * 20.0f;
	if (bgHue >= 360.0f) bgHue -= 360.0f;
	entranceTimer += dt;

	// Volume in MENU_SETTINGS
	//if (BGMvolumeSlider.dragging) {
	//	std::cout << "Volume Slider: True" << std::endl;
	//}

	if (BGMvolumeSlider.dragging) {
		float left = BGMvolumeSlider.x - BGMvolumeSlider.w * 0.5f;
		float right = BGMvolumeSlider.x + BGMvolumeSlider.w * 0.5f;

		float t = (worldX - left) / (right - left);
		t = Clamp01(t);

		BGMvolumeSlider.value = t;
		BGMVolume = t;

		gAudio.SetBGMVolume(BGMVolume);
	}

	if (GeneralSFXvolumeSlider.dragging) {
		float left = GeneralSFXvolumeSlider.x - GeneralSFXvolumeSlider.w * 0.5f;
		float right = GeneralSFXvolumeSlider.x + GeneralSFXvolumeSlider.w * 0.5f;

		float t = (worldX - left) / (right - left);
		t = Clamp01(t);

		GeneralSFXvolumeSlider.value = t;
		GeneralSFXVolume = t;

		gAudio.SetGeneralSFXVolume(GeneralSFXVolume);
	}

	if (CombatSFXvolumeSlider.dragging) {
		float left = CombatSFXvolumeSlider.x - CombatSFXvolumeSlider.w * 0.5f;
		float right = CombatSFXvolumeSlider.x + CombatSFXvolumeSlider.w * 0.5f;

		float t = (worldX - left) / (right - left);
		t = Clamp01(t);

		CombatSFXvolumeSlider.value = t;
		CombatSFXVolume = t;

		gAudio.SetCombatSFXVolume(CombatSFXVolume);
	}

	if (PlayerSFXvolumeSlider.dragging) {
		float left = PlayerSFXvolumeSlider.x - PlayerSFXvolumeSlider.w * 0.5f;
		float right = PlayerSFXvolumeSlider.x + PlayerSFXvolumeSlider.w * 0.5f;

		float t = (worldX - left) / (right - left);
		t = Clamp01(t);

		PlayerSFXvolumeSlider.value = t;
		PlayerSFXVolume = t;

		gAudio.SetPlayerSFXVolume(PlayerSFXVolume);
	}

	if (EnemySFXvolumeSlider.dragging) {
		float left = EnemySFXvolumeSlider.x - EnemySFXvolumeSlider.w * 0.5f;
		float right = EnemySFXvolumeSlider.x + EnemySFXvolumeSlider.w * 0.5f;

		float t = (worldX - left) / (right - left);
		t = Clamp01(t);

		EnemySFXvolumeSlider.value = t;
		EnemySFXVolume = t;

		gAudio.SetEnemySFXVolume(EnemySFXVolume);
	}

}

// ========== DRAW ==========
void MainMenu_Draw() {
	//AESysFrameStart();

	// Background: slow HSV color cycle
	float bgR, bgG, bgB;
	HsvToRgb(bgHue, 0.35f, 0.25f, bgR, bgG, bgB);
	AEGfxSetBackgroundColor(bgR, bgG, bgB);

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	if (menuScreen == MENU_MAIN) {
		// Panel behind buttons
		float panelEase = Smoothstep(entranceTimer * 2.5f);
		float btnStartY = hasSaveFile ? 60.0f : -20.0f;
		float btnEndY = btnStartY - 85.0f * (mainButtonCount - 1);
		float panelH = (btnStartY - btnEndY) + 120.0f;
		float panelY = (btnStartY + btnEndY) * 0.5f;
		DrawPanel(0, panelY, 400, panelH, panelEase);

		// Styled buttons with entrance stagger
		for (int i = 0; i < mainButtonCount; i++) {
			float delay = i * 0.08f;
			float ease = Smoothstep((entranceTimer - delay) * 2.5f);
			float yOff = (1.0f - ease) * 30.0f;
			DrawStyledButton(mainButtons[i].x, mainButtons[i].y - yOff,
				mainButtons[i].w, mainButtons[i].h,
				mainButtons[i].hoverT, ease);
		}
	}
	else if (menuScreen == MENU_TUTORIAL_PROMPT) {
		// Dark overlay + compact panel for the prompt
		DrawMesh(rectMesh, 1600, 900, -800, 0, 0, 0, 0, 0, 180);
		DrawPanel(0, -100, 550, 250, 1.0f);

		// Yes / No buttons
		DrawStyledButton(yesButton.x, yesButton.y, yesButton.w, yesButton.h, yesButton.hoverT, 1.0f);
		DrawStyledButton(noButton.x, noButton.y, noButton.w, noButton.h, noButton.hoverT, 1.0f);

		// Back button below the panel
		DrawStyledButton(backButton.x, backButton.y,
			backButton.w, backButton.h,
			backButton.hoverT, 1.0f);
	}
	else if (menuScreen == MENU_CONFIRMATION) {
		DrawMesh(rectMesh, 1600, 900, -800, 0, 0, 0, 0, 0, 180);
		DrawPanel(0, -100, 550, 250, 1.0f);

		DrawStyledButton(yesButton.x, yesButton.y,
			yesButton.w, yesButton.h,
			yesButton.hoverT, 1.0f);

		DrawStyledButton(noButton.x, noButton.y,
			noButton.w, noButton.h,
			noButton.hoverT, 1.0f);
	}
	else {
		// Sub-screen: dark overlay + bordered panel
		DrawMesh(rectMesh, 1600, 900, -800, 0, 0, 0, 0, 0, 180);

		if (menuScreen == MENU_CREDITS)
			DrawPanel(0, -20, 1400, 760, 1.0f);
		else
			DrawPanel(0, -50, 1200, 700, 1.0f);

		// Back button (styled)
		DrawStyledButton(backButton.x, backButton.y,
			backButton.w, backButton.h,
			backButton.hoverT, 1.0f);
	}

	// --- Text ---
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// Title: "Revenge of the Pinata" in gold with sine bob
	if (menuScreen != MENU_CREDITS && menuScreen != MENU_CONTROLS && menuScreen != MENU_SETTINGS) {
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
		for (int i = 0; i < mainButtonCount; i++) {
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
			"Mouse - Aim",
			"X - Interact" // Interact w augmentball
		};
		for (int i = 0; i < 6; i++) {
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
		float tw, th;

		// Top middle "Credits" - moved upward
		{
			const char* header = "Credits";
			AEGfxGetPrintSize(fontBody, header, 1.5f, &tw, &th);
			AEGfxPrint(fontBody, header, -tw * 0.5f, 270.0f / 450.0f,
				1.5f, 1.0f, 1.0f, 1.0f, 1.0f);
		}

		// DigiPen logo
		if (digipenLogo && digipenLogoMesh) {
			DrawTexture(logoSprite, 0,
				digipenLogoMesh,
				digipenLogo,
				320.0f, 120.0f,
				-20.0f, 170.0f,
				0.0f,
				1.0f);
		}

		// Column headers - stretched further outward
		{
			const char* leftHeader = "Team Members";
			AEGfxGetPrintSize(fontBody, leftHeader, 0.9f, &tw, &th);
			AEGfxPrint(fontBody, leftHeader, (-470.0f / 800.0f) - tw * 0.5f, 35.0f / 450.0f,
				0.9f, 1.0f, 1.0f, 1.0f, 1.0f);
		}
		{
			const char* midHeader = "President";
			AEGfxGetPrintSize(fontBody, midHeader, 0.9f, &tw, &th);
			AEGfxPrint(fontBody, midHeader, (0.0f / 800.0f) - tw * 0.5f, 15.0f / 450.0f,
				0.9f, 1.0f, 1.0f, 1.0f, 1.0f);
		}
		{
			const char* rightHeader = "Instructors";
			AEGfxGetPrintSize(fontBody, rightHeader, 0.9f, &tw, &th);
			AEGfxPrint(fontBody, rightHeader, (470.0f / 800.0f) - tw * 0.5f, 35.0f / 450.0f,
				0.9f, 1.0f, 1.0f, 1.0f, 1.0f);
		}

		// Left column - moved up and farther left
		{
			const char* leftLines[] = {
				"Chiu Jun Wen",
				"Nigel Lim Kai Yu",
				"Charles Yap",
				"Chew Zheng Hui, Timothy Caleb"
			};
			for (int i = 0; i < 4; ++i) {
				float ly = -10.0f - i * 42.0f;
				AEGfxGetPrintSize(fontBody, leftLines[i], 0.72f, &tw, &th);
				AEGfxPrint(fontBody, leftLines[i],
					(-470.0f / 800.0f) - tw * 0.5f, ly / 450.0f,
					0.72f, 1.0f, 1.0f, 1.0f, 1.0f);
			}
		}

		// Middle column
		{
			const char* midLines[] = {
				"Claude Comair"
			};
			for (int i = 0; i < 1; ++i) {
				float ly = -30.0f - i * 42.0f;
				AEGfxGetPrintSize(fontBody, midLines[i], 0.72f, &tw, &th);
				AEGfxPrint(fontBody, midLines[i],
					(0.0f / 800.0f) - tw * 0.5f, ly / 450.0f,
					0.72f, 1.0f, 1.0f, 1.0f, 1.0f);
			}
		}

		// Right column - moved up and farther right
		{
			const char* rightLines[] = {
				"Gerald Wong Han Feng",
				"Dr. Soroor Malekmohammadai",
				"Tommy Tan Chee Wei"
			};
			for (int i = 0; i < 3; ++i) {
				float ly = -10.0f - i * 42.0f;
				AEGfxGetPrintSize(fontBody, rightLines[i], 0.72f, &tw, &th);
				AEGfxPrint(fontBody, rightLines[i],
					(470.0f / 800.0f) - tw * 0.5f, ly / 450.0f,
					0.72f, 1.0f, 1.0f, 1.0f, 1.0f);
			}
		}

		// Bottom middle text - moved upward a little
		{
			const char* bottom1 = "GAM150 Team Project";
			AEGfxGetPrintSize(fontBody, bottom1, 0.82f, &tw, &th);
			AEGfxPrint(fontBody, bottom1, -tw * 0.5f, -235.0f / 450.0f,
				0.82f, 1.0f, 1.0f, 1.0f, 1.0f);
		}
		{
			const char* bottom2 = "Made with AlphaEngine";
			AEGfxGetPrintSize(fontBody, bottom2, 0.76f, &tw, &th);
			AEGfxPrint(fontBody, bottom2, -tw * 0.5f, -272.0f / 450.0f,
				0.76f, 1.0f, 1.0f, 1.0f, 1.0f);
		}

		// Back button label
		AEGfxGetPrintSize(fontBody, backButton.label, 1.0f, &tw, &th);
		AEGfxPrint(fontBody, backButton.label,
			backButton.x / 800.0f - tw * 0.5f,
			backButton.y / 450.0f - th * 0.5f,
			1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	}
	else if (menuScreen == MENU_TUTORIAL_PROMPT) {
		// Prompt text
		{
			const char* prompt = "Do you need a tutorial?";
			float tw, th;
			AEGfxGetPrintSize(fontBody, prompt, 0.8f, &tw, &th);
			AEGfxPrint(fontBody, prompt, -tw * 0.5f, -30.0f / 450.0f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f);
		}
		// Yes button label
		{
			float tw, th;
			AEGfxGetPrintSize(fontBody, yesButton.label, 1.0f, &tw, &th);
			AEGfxPrint(fontBody, yesButton.label,
				yesButton.x / 800.0f - tw * 0.5f,
				yesButton.y / 450.0f - th * 0.5f,
				1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		}
		// No button label
		{
			float tw, th;
			AEGfxGetPrintSize(fontBody, noButton.label, 1.0f, &tw, &th);
			AEGfxPrint(fontBody, noButton.label,
				noButton.x / 800.0f - tw * 0.5f,
				noButton.y / 450.0f - th * 0.5f,
				1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		}
		// Back button label
		{
			float tw, th;
			AEGfxGetPrintSize(fontBody, backButton.label, 1.0f, &tw, &th);
			AEGfxPrint(fontBody, backButton.label,
				backButton.x / 800.0f - tw * 0.5f,
				backButton.y / 450.0f - th * 0.5f,
				1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		}
	} 
	else if (menuScreen == MENU_SETTINGS) {
		// std::cout << "HERE!" << std::endl;
		// Panel behind buttons (Settings screen)
		// DrawPanel(0, -150, 400, 380, 1.0f);

		// Title Text
		float tw, th;
		{
			const char* header = "Settings";
			AEGfxGetPrintSize(fontBody, header, 1.5f, &tw, &th);
			AEGfxPrint(fontBody, header, -tw * 0.5f, 220.0f / 450.0f,
				1.5f, 1.0f, 1.0f, 1.0f, 1.0f);
		}

		// ===BGM SLIDER===
		// Draw volume slider
		DrawMesh(rectMesh, BGMvolumeSlider.w, BGMvolumeSlider.h, BGMvolumeSlider.x - 200, BGMvolumeSlider.y, 0, 100, 100, 100, 255); // Gray slider bar
		float handleX = Lerp(BGMvolumeSlider.x - BGMvolumeSlider.w * 0.5f, BGMvolumeSlider.x + BGMvolumeSlider.w * 0.5f, BGMvolumeSlider.value);
		DrawMesh(rectMesh, 20, 30, handleX - 10, BGMvolumeSlider.y, 0, 255, 0, 0, 255);

		// Draw volume text label
		char buffer[64];
		sprintf_s(buffer, "Background Music: %.0f%%", BGMvolumeSlider.value * 100.0f);
		AEGfxGetPrintSize(fontBody, buffer, 1.0f, &tw, &th);
		AEGfxPrint(fontBody, buffer, -tw * 0.5f, (BGMvolumeSlider.y + 50.0f) / 450.0f - th * 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);



		// ===GENERAL SFX SLIDER===
		// Draw volume slider
		DrawMesh(rectMesh, GeneralSFXvolumeSlider.w, GeneralSFXvolumeSlider.h, GeneralSFXvolumeSlider.x - 200, GeneralSFXvolumeSlider.y, 0, 100, 100, 100, 255);
		float handleX2 = Lerp(GeneralSFXvolumeSlider.x - GeneralSFXvolumeSlider.w * 0.5f,GeneralSFXvolumeSlider.x + GeneralSFXvolumeSlider.w * 0.5f,GeneralSFXvolumeSlider.value);
		DrawMesh(rectMesh, 20, 30, handleX2 - 10, GeneralSFXvolumeSlider.y, 0, 0, 200, 255, 255);

		// Draw volume text label
		char buffer2[64];
		sprintf_s(buffer2, "General SFX Volume: %.0f%%", GeneralSFXvolumeSlider.value * 100.0f);
		AEGfxGetPrintSize(fontBody, buffer2, 1.0f, &tw, &th);
		AEGfxPrint(fontBody, buffer2, -tw * 0.5f, (GeneralSFXvolumeSlider.y + 50.0f) / 450.0f - th * 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);



		// ===COMBAT SFX SLIDER===
		// Draw volume slider
		DrawMesh(rectMesh, CombatSFXvolumeSlider.w, CombatSFXvolumeSlider.h, CombatSFXvolumeSlider.x - 200, CombatSFXvolumeSlider.y, 0, 100, 100, 100, 255);
		float handleX3 = Lerp(CombatSFXvolumeSlider.x - CombatSFXvolumeSlider.w * 0.5f, CombatSFXvolumeSlider.x + CombatSFXvolumeSlider.w * 0.5f, CombatSFXvolumeSlider.value);
		DrawMesh(rectMesh, 20, 30, handleX3 - 10, CombatSFXvolumeSlider.y, 0, 0, 200, 255, 255);

		// Draw volume text label
		char buffer3[64];
		sprintf_s(buffer3, "Combat SFX Volume: %.0f%%", CombatSFXvolumeSlider.value * 100.0f);
		AEGfxGetPrintSize(fontBody, buffer3, 1.0f, &tw, &th);
		AEGfxPrint(fontBody, buffer3, -tw * 0.5f, (CombatSFXvolumeSlider.y + 50.0f) / 450.0f - th * 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);



		// ===PLAYER SFX SLIDER===
		// Draw volume slider
		DrawMesh(rectMesh, PlayerSFXvolumeSlider.w, PlayerSFXvolumeSlider.h, PlayerSFXvolumeSlider.x - 200, PlayerSFXvolumeSlider.y, 0, 100, 100, 100, 255);
		float handleX4 = Lerp(PlayerSFXvolumeSlider.x - PlayerSFXvolumeSlider.w * 0.5f, PlayerSFXvolumeSlider.x + PlayerSFXvolumeSlider.w * 0.5f, PlayerSFXvolumeSlider.value);
		DrawMesh(rectMesh, 20, 30, handleX4 - 10, PlayerSFXvolumeSlider.y, 0, 0, 200, 255, 255);

		// Draw volume text label
		char buffer4[64];
		sprintf_s(buffer4, "Player SFX Volume: %.0f%%", PlayerSFXvolumeSlider.value * 100.0f);
		AEGfxGetPrintSize(fontBody, buffer4, 1.0f, &tw, &th);
		AEGfxPrint(fontBody, buffer4, -tw * 0.5f, (PlayerSFXvolumeSlider.y + 50.0f) / 450.0f - th * 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);



		// ===ENEMY SFX SLIDER===
		// Draw volume slider
		DrawMesh(rectMesh, EnemySFXvolumeSlider.w, EnemySFXvolumeSlider.h, EnemySFXvolumeSlider.x - 200, EnemySFXvolumeSlider.y, 0, 100, 100, 100, 255);
		float handleX5 = Lerp(EnemySFXvolumeSlider.x - EnemySFXvolumeSlider.w * 0.5f, EnemySFXvolumeSlider.x + EnemySFXvolumeSlider.w * 0.5f, EnemySFXvolumeSlider.value);
		DrawMesh(rectMesh, 20, 30, handleX5 - 10, EnemySFXvolumeSlider.y, 0, 0, 200, 255, 255);

		// Draw volume text label
		char buffer5[64];
		sprintf_s(buffer5, "Enemy SFX Volume: %.0f%%", EnemySFXvolumeSlider.value * 100.0f);
		AEGfxGetPrintSize(fontBody, buffer5, 1.0f, &tw, &th);
		AEGfxPrint(fontBody, buffer5, -tw * 0.5f, (EnemySFXvolumeSlider.y + 50.0f) / 450.0f - th * 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

		// Back button label
		//float tw, th;
		AEGfxGetPrintSize(fontBody, backButton.label, 1.0f, &tw, &th);
		AEGfxPrint(fontBody, backButton.label,
			backButton.x / 800.0f - tw * 0.5f,
			backButton.y / 450.0f - th * 0.5f,
			1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	}
	else if (menuScreen == MENU_CONFIRMATION) {
		const char* prompt = "Are you sure?";
		float tw, th;

		AEGfxGetPrintSize(fontBody, prompt, 0.8f, &tw, &th);
		AEGfxPrint(fontBody, prompt, -tw * 0.5f, -30.0f / 450.0f,
			0.8f, 1.0f, 1.0f, 1.0f, 1.0f);

		AEGfxGetPrintSize(fontBody, yesButton.label, 1.0f, &tw, &th);
		AEGfxPrint(fontBody, yesButton.label,
			yesButton.x / 800.0f - tw * 0.5f,
			yesButton.y / 450.0f - th * 0.5f,
			1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

		AEGfxGetPrintSize(fontBody, noButton.label, 1.0f, &tw, &th);
		AEGfxPrint(fontBody, noButton.label,
			noButton.x / 800.0f - tw * 0.5f,
			noButton.y / 450.0f - th * 0.5f,
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

	//AESysFrameEnd();
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
	if (digipenLogoMesh) {
		AEGfxMeshFree(digipenLogoMesh);
		digipenLogoMesh = nullptr;
	}
	if (digipenLogo) {
		AEGfxTextureUnload(digipenLogo);
		digipenLogo = nullptr;
	}
	AEAudioStopGroup(gAudio.audioGroup.BGM);
}