#include "pch.h"
#include "Utils.h"
#include "Player.h"
#include "Tutorial.h"
#include "camera.h"
#include "GameStateManager.h"
#include "Map.h"
#include "Pause.h"
#include "HUD.h"
#include "Audio.h"
#include "Shadow.h"
#include "Projectile.h"
#include "Combat.h"
#include "AugmentData.h"
#include "EventSystem.h"

// --- Tutorial steps ---
enum TutorialStep {
	TUT_MOVE,       // WASD to move
	TUT_DASH,       // Space to dash
	TUT_ATTACK,     // LMB to attack (introduces charges)
	TUT_CHARGES,    // Explain attack charges
	TUT_BLOCK,      // RMB to block
	TUT_PARRY,      // Parry timing to regain charges
	TUT_COMBAT,     // Fight a practice enemy
	TUT_DONE        // Tutorial complete
};

// load variables
static AEGfxTexture* TexBlock2;
static AEGfxTexture* TexBlock;
static AEGfxTexture* TexSpeechLeft;
static AEGfxTexture* TexPinata;
static AEGfxVertexList* CircleMesh;
static AEGfxVertexList* RectMesh;
static s8 tutorialFont = -1;

// ===== ADJUSTABLE TUTORIAL UI POSITIONS =====
// All offsets are in screen pixels from camera center
static const float PINATA_OFFSET_X  = -700.0f;  // pinata X (negative = left)
static const float PINATA_OFFSET_Y  = -200.0f;  // pinata Y (negative = down)
static const float PINATA_W         =  500.0f;   // pinata draw width
static const float PINATA_H         =  500.0f;   // pinata draw height

static const float SPEECH_OFFSET_X  =  380.0f;  // speech bubble X offset from pinata
static const float SPEECH_OFFSET_Y  =  140.0f;  // speech bubble Y offset from pinata
static const float SPEECH_W         =  450.0f;   // speech bubble width
static const float SPEECH_H         =  250.0f;   // speech bubble height

static const float TEXT_OFFSET_X    =  160.0f;   // text fine-tune X inside bubble (pixels)
static const float TEXT_OFFSET_Y    =   15.0f;   // text fine-tune Y inside bubble (pixels)
static const float TEXT_SCALE       =    0.72f;  // main text scale
static const float SUBTEXT_OFFSET_Y =  -25.0f;   // subtext Y offset from bubble center
static const float SUBTEXT_SCALE    =    0.65f;  // subtext scale
static const float TEXT_WRAP_WIDTH  =  200.0f;   // max text width in pixels before wrapping
static const float TEXT_LINE_SPACING=   35.0f;   // pixels between wrapped lines

// Pinata slide-in animation
static const float PINATA_SLIDE_SPEED = 900.0f;  // pixels per second
static const float PINATA_START_Y     = -700.0f;  // off-screen below (offset added to PINATA_OFFSET_Y)
static float pinataSlideY;

// Pinata bob animation (triggers on new step)
static const float PINATA_BOB_AMP      = 25.0f;   // max bob height in pixels
static const float PINATA_BOB_FREQ     = 8.0f;    // oscillations per second
static const float PINATA_BOB_DURATION = 0.8f;    // seconds until bob fades out
static float pinataBobTimer;
static float pinataBobY;
static TutorialStep prevStep;

// init variables
static Player player{};
static Camera camera{};
static MapSystem gameMap;

// combat
static Combat::System CombatSystem;
static std::vector<std::unique_ptr<Enemy>> tutorialEnemies{};

// tutorial state
static TutorialStep currentStep;
static float stepTimer;
static bool hasMoved;
static bool hasDashed;
static bool hasAttacked;
static bool hasBlocked;
static bool hasParried;
static int  chargesBeforeParry;

static const char* GetStepText() {
	switch (currentStep) {
	case TUT_MOVE:    return "Use WASD to move around";
	case TUT_DASH:    return "Press SPACE to dash";
	case TUT_ATTACK:  return "Click LMB to attack";
	case TUT_CHARGES: return "Attacks use charges!";
	case TUT_BLOCK:   return "Hold RMB to block";
	case TUT_PARRY:   return "Parry to regain charges!";
	case TUT_COMBAT:  return "Defeat the enemy!";
	case TUT_DONE:    return "Tutorial complete!";
	default:          return "";
	}
}

static const char* GetStepSubtext() {
	switch (currentStep) {
	case TUT_ATTACK:  return "You have limited attack charges";
	case TUT_CHARGES: return "Check your charges in the HUD";
	case TUT_PARRY:   return "Block right as an enemy hits you";
	default:          return "";
	}
}

static void DrawWrappedText(s8 font, const char* text, float scale,
                           float centerNormX, float centerNormY,
                           float maxWidthPx, float lineSpacingPx,
                           float r, float g, float b, float a) {
	float maxNorm = maxWidthPx / 400.0f;

	// Split into words
	std::vector<std::string> words;
	std::string word;
	for (const char* p = text; *p; ++p) {
		if (*p == ' ') {
			if (!word.empty()) { words.push_back(word); word.clear(); }
		} else {
			word += *p;
		}
	}
	if (!word.empty()) words.push_back(word);

	// Build lines that fit within maxNorm width
	std::vector<std::string> lines;
	std::string currentLine;
	for (const auto& w : words) {
		std::string test = currentLine.empty() ? w : currentLine + " " + w;
		float tw, th;
		AEGfxGetPrintSize(font, test.c_str(), scale, &tw, &th);
		if (tw > maxNorm && !currentLine.empty()) {
			lines.push_back(currentLine);
			currentLine = w;
		} else {
			currentLine = test;
		}
	}
	if (!currentLine.empty()) lines.push_back(currentLine);

	// Center the block vertically around centerNormY
	float lineStep = lineSpacingPx / 450.0f;
	int numLines = (int)lines.size();
	float blockH = (numLines - 1) * lineStep;
	float startY = centerNormY + blockH * 0.5f;

	for (int i = 0; i < numLines; ++i) {
		float tw, th;
		AEGfxGetPrintSize(font, lines[i].c_str(), scale, &tw, &th);
		float y = startY - i * lineStep;
		AEGfxPrint(font, lines[i].c_str(), centerNormX - tw * 0.5f, y - th * 0.5f,
		           scale, r, g, b, a);
	}
}

static void SpawnPracticeEnemy() {
	tutorialEnemies.clear();
	AEVec2 playerPos = { player.GetX(), player.GetY() };
	AEVec2 spawnPos = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
	tutorialEnemies.push_back(std::make_unique<Walker>(spawnPos, ENEMY_SIZE, 100.0f, 150.0f));
	for (auto& enemy : tutorialEnemies) {
		enemy->Init();
		enemy->SetMap(&gameMap);
	}
}

void Tutorial_Load() {
	TexBlock2  = AEGfxTextureLoad("Assets/block2.png");
	TexBlock   = AEGfxTextureLoad("Assets/block.png");
	CircleMesh = CreateCircleMesh(1.0f, 32, 0xFFFFFFFF);
	RectMesh   = CreateRectMesh(0xFFFFFFFF);
	tutorialFont = AEGfxCreateFont("Assets/liberation-mono.ttf", 36);
	TexSpeechLeft = AEGfxTextureLoad("Assets/speech_left.png");
	TexPinata     = AEGfxTextureLoad("Assets/pinata1.png");
	gameMap.Init("Assets/tutorial.tmx", "tilesheet_complete", "Assets/tilesheet_complete.png");
	gameMap.BuildCollisionGrid("Tile Layer 2");
	Pause_Load();
	HUD_Load();
	Shadow_Init();
}

void Tutorial_Init() {
	player.Init();
	player.SetAttackCharges(g_PlayerAttackCharges);
	player.SetMap(&gameMap);
	camera.Init(player.GetX(), player.GetY());
	Pause_Init();

	currentStep = TUT_MOVE;
	stepTimer = 0.0f;
	hasMoved = false;
	hasDashed = false;
	hasAttacked = false;
	hasBlocked = false;
	hasParried = false;
	chargesBeforeParry = 0;
	pinataSlideY = PINATA_START_Y;
	pinataBobTimer = PINATA_BOB_DURATION; // start settled
	pinataBobY = 0.0f;
	prevStep = TUT_MOVE;

	gAudio.PlayBGM(BGM_WAVE);
}

void Tutorial_Update(float dt) {
	if (!AESysDoesWindowExist()) {
		next = GS_QUIT;
		return;
	}

	if (Pause_Update(true)) return;

	// Animate pinata sliding up from below
	if (pinataSlideY < 0.0f) {
		pinataSlideY += PINATA_SLIDE_SPEED * dt;
		if (pinataSlideY > 0.0f) pinataSlideY = 0.0f;
	}

	// Trigger bob on step change
	if (currentStep != prevStep) {
		pinataBobTimer = 0.0f;
		prevStep = currentStep;
	}

	// Animate bob (decaying sine wave)
	if (pinataBobTimer < PINATA_BOB_DURATION) {
		pinataBobTimer += dt;
		float t = pinataBobTimer / PINATA_BOB_DURATION; // 0 to 1
		float decay = 1.0f - t;
		pinataBobY = PINATA_BOB_AMP * sinf(pinataBobTimer * PINATA_BOB_FREQ * 2.0f * PI) * decay;
	} else {
		pinataBobY = 0.0f;
	}

	if (!player.GetIsAlive()) { next = GS_GAMEOVER; return; }

	bool preventMovement = false;
	player.Update(dt, CombatSystem, tutorialEnemies, camera.GetX(), camera.GetY(), preventMovement);

	// Check tutorial step progression
	switch (currentStep) {
	case TUT_MOVE:
		if (AEInputCheckCurr(AEVK_W) || AEInputCheckCurr(AEVK_A) ||
		    AEInputCheckCurr(AEVK_S) || AEInputCheckCurr(AEVK_D)) {
			hasMoved = true;
		}
		if (hasMoved) {
			stepTimer += dt;
			if (stepTimer > 1.5f) {
				currentStep = TUT_DASH;
				stepTimer = 0.0f;
			}
		}
		break;

	case TUT_DASH:
		if (AEInputCheckTriggered(AEVK_SPACE)) {
			hasDashed = true;
		}
		if (hasDashed) {
			stepTimer += dt;
			if (stepTimer > 0.2f) {
				currentStep = TUT_ATTACK;
				stepTimer = 0.0f;
			}
		}
		break;

	case TUT_ATTACK:
		if (AEInputCheckTriggered(AEVK_LBUTTON)) {
			hasAttacked = true;
		}
		if (hasAttacked) {
			stepTimer += dt;
			if (stepTimer > 0.2f) {
				currentStep = TUT_CHARGES;
				stepTimer = 0.0f;
			}
		}
		break;

	case TUT_CHARGES:
		// Let the player read the charge info, then continue
		stepTimer += dt;
		if (stepTimer > 3.0f) {
			currentStep = TUT_BLOCK;
			stepTimer = 0.0f;
		}
		break;

	case TUT_BLOCK:
		if (AEInputCheckCurr(AEVK_RBUTTON)) {
			hasBlocked = true;
		}
		if (hasBlocked) {
			stepTimer += dt;
			if (stepTimer > 0.2f) {
				currentStep = TUT_PARRY;
				stepTimer = 0.0f;
				chargesBeforeParry = player.GetAttackCharges();
				SpawnPracticeEnemy();
			}
		}
		break;

	case TUT_PARRY:
		// Update enemies for this step
		tutorialEnemies.erase(
			std::remove_if(tutorialEnemies.begin(), tutorialEnemies.end(),
				[](const std::unique_ptr<Enemy>& e) {
					return e->GetCombatStats().health <= 0.0f;
				}),
			tutorialEnemies.end()
		);
		for (auto& enemy : tutorialEnemies) {
			enemy->Update(dt, CombatSystem, player);
			CombatSystem.Update(player, *enemy, camera, dt);
		}
		// Detect if player gained a charge via parry
		if (player.GetAttackCharges() > chargesBeforeParry) {
			hasParried = true;
		}
		if (hasParried) {
			stepTimer += dt;
			if (stepTimer > 1.5f) {
				tutorialEnemies.clear();
				currentStep = TUT_COMBAT;
				stepTimer = 0.0f;
				SpawnPracticeEnemy();
			}
		}
		// If enemy dies before parry, respawn it
		if (tutorialEnemies.empty() && !hasParried) {
			SpawnPracticeEnemy();
		}
		break;

	case TUT_COMBAT:
		// Update enemies
		tutorialEnemies.erase(
			std::remove_if(tutorialEnemies.begin(), tutorialEnemies.end(),
				[](const std::unique_ptr<Enemy>& e) {
					return e->GetCombatStats().health <= 0.0f;
				}),
			tutorialEnemies.end()
		);
		for (auto& enemy : tutorialEnemies) {
			enemy->Update(dt, CombatSystem, player);
			CombatSystem.Update(player, *enemy, camera, dt);
		}
		if (tutorialEnemies.empty()) {
			currentStep = TUT_DONE;
			stepTimer = 0.0f;
		}
		break;

	case TUT_DONE:
		stepTimer += dt;
		if (stepTimer > 2.0f) {
			next = GS_LEVEL1;
		}
		break;
	}

	// Map boundary clamping (same as Level1)
	float halfW = GRID_W * 0.5f;
	float halfH = GRID_H * 0.5f;
	float invX = player.GetX() / halfW;
	float invY = player.GetY() / halfH;
	float gridX = 0.5f * (invX + invY);
	float gridY = 0.5f * (invY - invX);

	const float GRID_OFFSET = -10.0f;
	float MAP_MIN_X = 1.0f + GRID_OFFSET;
	float MAP_MIN_Y = 0.0f + GRID_OFFSET;
	float MAP_MAX_X = (float)(std::max)(1u, gameMap.GetMapWidth()) + GRID_OFFSET;
	float MAP_MAX_Y = (float)(std::max)(1u, gameMap.GetMapHeight()) - 1.0f + GRID_OFFSET;

	bool clamped = false;
	if (gridX < MAP_MIN_X) { gridX = MAP_MIN_X; clamped = true; }
	if (gridX > MAP_MAX_X) { gridX = MAP_MAX_X; clamped = true; }
	if (gridY < MAP_MIN_Y) { gridY = MAP_MIN_Y; clamped = true; }
	if (gridY > MAP_MAX_Y) { gridY = MAP_MAX_Y; clamped = true; }

	if (clamped) {
		float newScreenX = (gridX - gridY) * halfW;
		float newScreenY = (gridX + gridY) * halfH;
		player.SetPosition(newScreenX, newScreenY);
	}

	camera.Update(dt, player.GetX(), player.GetY(), preventMovement);

	// Skip tutorial shortcut
	if (AEInputCheckTriggered(AEVK_N)) {
		next = GS_LEVEL1;
	}
}

void Tutorial_Draw() {
	AESysFrameStart();
	AEGfxSetBackgroundColor(0.68f, 0.85f, 0.90f);
	AEGfxSetCamPosition(camera.GetX(), camera.GetY());

	// Draw map
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	gameMap.Draw("Tile Layer 1");

	// Render queue for depth sorting
	std::vector<RenderNode> renderQueue;
	renderQueue.push_back({ player.GetY(), [&]() { player.Draw(); } });

	for (auto& enemy : tutorialEnemies) {
		Enemy* ePtr = enemy.get();
		renderQueue.push_back({ ePtr->GetY(), [ePtr]() { ePtr->Draw(); } });
	}

	gameMap.QueueLayer("Tile Layer 2", renderQueue);

	std::sort(renderQueue.begin(), renderQueue.end(), [](const RenderNode& a, const RenderNode& b) {
		return a.y > b.y;
	});

	for (auto& node : renderQueue) {
		node.drawCall();
	}


	// --- Pinata + Speech Bubble tutorial overlay ---
	float pinataX = camera.GetX() + PINATA_OFFSET_X;
	float pinataY = camera.GetY() + PINATA_OFFSET_Y + pinataSlideY;
	float speechX = pinataX + SPEECH_OFFSET_X;
	float speechY = pinataY + SPEECH_OFFSET_Y;

	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	// Draw speech bubble (centered at speechX, speechY)
	AEGfxTextureSet(TexSpeechLeft, 0.0f, 0.0f);
	{
		AEMtx33 scale, translate, transform;
		AEMtx33Scale(&scale, SPEECH_W, SPEECH_H);
		AEMtx33Trans(&translate, speechX - SPEECH_W * 0.5f, speechY);
		AEMtx33Concat(&transform, &translate, &scale);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(RectMesh, AE_GFX_MDM_TRIANGLES);
	}

	// Draw pinata (centered at pinataX, pinataY)
	AEGfxTextureSet(TexPinata, 0.0f, 0.0f);
	{
		AEMtx33 scale, translate, transform;
		AEMtx33Scale(&scale, PINATA_W, PINATA_H);
		AEMtx33Trans(&translate, pinataX - PINATA_W * 0.5f, pinataY + pinataBobY);
		AEMtx33Concat(&transform, &translate, &scale);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(RectMesh, AE_GFX_MDM_TRIANGLES);
	}

	// Draw tutorial text inside speech bubble
	const char* text = GetStepText();
	const char* subtext = GetStepSubtext();
	bool hasSubtext = subtext[0] != '\0';
	float tw, th;

	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	float textScreenX = PINATA_OFFSET_X + SPEECH_OFFSET_X + TEXT_OFFSET_X;
	float textScreenY = PINATA_OFFSET_Y + pinataSlideY + SPEECH_OFFSET_Y + (hasSubtext ? TEXT_OFFSET_Y + 10.0f : TEXT_OFFSET_Y);
	DrawWrappedText(tutorialFont, text, TEXT_SCALE,
	                textScreenX / 400.0f, textScreenY / 450.0f,
	                TEXT_WRAP_WIDTH, TEXT_LINE_SPACING,
	                0.1f, 0.1f, 0.1f, 1.0f);

	if (hasSubtext) {
		float subScreenY = PINATA_OFFSET_Y + pinataSlideY + SPEECH_OFFSET_Y + SUBTEXT_OFFSET_Y;
		DrawWrappedText(tutorialFont, subtext, SUBTEXT_SCALE,
		                textScreenX / 400.0f, subScreenY / 450.0f,
		                TEXT_WRAP_WIDTH, TEXT_LINE_SPACING,
		                0.3f, 0.3f, 0.3f, 1.0f);
	}

	HUD_Draw(&player, camera.GetX(), camera.GetY());
	Pause_Draw(camera.GetX(), camera.GetY());

	// "Press N to skip" hint at bottom
	const char* skip = "Press N to skip tutorial";
	AEGfxGetPrintSize(tutorialFont, skip, 0.7f, &tw, &th);
	AEGfxPrint(tutorialFont, skip, 0.4f, 0.8f, 0.7f, 0.0f, 0.0f, 0.0f, 1.0f);

	AESysFrameEnd();
}

void Tutorial_Free() {
	tutorialEnemies.clear();
	player.Free();
	Projectile::Free();
	g_Events.ClearAll();
}

void Tutorial_Unload() {
	Shadow_Free();
	if (TexBlock)      { AEGfxTextureUnload(TexBlock);      TexBlock      = nullptr; }
	if (TexBlock2)     { AEGfxTextureUnload(TexBlock2);     TexBlock2     = nullptr; }
	if (TexSpeechLeft) { AEGfxTextureUnload(TexSpeechLeft); TexSpeechLeft = nullptr; }
	if (TexPinata)     { AEGfxTextureUnload(TexPinata);     TexPinata     = nullptr; }
	AEGfxMeshFree(CircleMesh); CircleMesh = nullptr;
	AEGfxMeshFree(RectMesh);   RectMesh   = nullptr;
	if (tutorialFont >= 0) { AEGfxDestroyFont(tutorialFont); tutorialFont = -1; }
	gameMap.Unload();
	Pause_Unload();
	HUD_Unload();
	AEAudioStopGroup(gAudio.audioGroup.BGM);
}
