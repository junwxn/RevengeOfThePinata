#include "pch.h"
#include "Utils.h"
#include "Player.h"
#include "TestLevel.h"
#include "camera.h"
#include "GameStateManager.h"
#include "Map.h"
#include "Pause.h"
#include "HUD.h"
#include "Audio.h"
#include "Debug.h"
#include "Shadow.h"
#include "Projectile.h"
#include "EventSystem.h"

// load variables
static AEGfxVertexList* CircleMesh;
static AEGfxVertexList* RectMesh;

// init variables
static Player player{};
static Camera camera{};
static MapSystem gameMap;

// enemies (for testing combat on new map features)
static Combat::System CombatSystem;
static std::vector<std::unique_ptr<Enemy>> Wave{};

void TestLevel_Load() {
	CircleMesh = CreateCircleMesh(1.0f, 32, 0xFFFFFFFF);
	RectMesh = CreateRectMesh(0xFFFFFFFF);
	gameMap.Init("Assets/level1map.tmx", "tilesheet_complete", "Assets/tilesheet_complete.png");
	gameMap.BuildCollisionGrid("Tile Layer 2");
	Pause_Load();
	HUD_Load();
	Debug_Load();
	Shadow_Init();
}

void TestLevel_Init() {
	player.Init();
	player.SetAttackCharges(DEFAULT_ATTACK_CHARGES);
	player.SetMap(&gameMap);

	camera.Init(player.GetX(), player.GetY());
	Pause_Init();

	Wave.clear();

	gAudio.PlayBGM(BGM_WAVE);
	Debug_Init();
	DebugContext dbgCtx = {};
	dbgCtx.player    = &player;
	dbgCtx.camera    = &camera;
	dbgCtx.map       = &gameMap;
	dbgCtx.waves[0]  = &Wave;
	dbgCtx.waveCount = 1;
	dbgCtx.levelName = "Test Level";
	Debug_Register(dbgCtx);
}

void TestLevel_Update(float dt) {
	if (!AESysDoesWindowExist()) {
		next = GS_QUIT;
		return;
	}

	if (Pause_Update(true)) return;
	Debug_Update();

	bool preventingmovement = false;
	player.Update(dt, CombatSystem, Wave, camera.GetX(), camera.GetY(), preventingmovement);

	// Remove dead enemies
	Wave.erase(
		std::remove_if(Wave.begin(), Wave.end(),
			[](const std::unique_ptr<Enemy>& e) { return e->GetCombatStats().health <= 0.0f; }),
		Wave.end()
	);

	for (auto& enemy : Wave) {
		enemy->Update(dt, CombatSystem, player, Wave);
		CombatSystem.Update(player, *enemy, camera, dt);
	}

	// Map boundary clamping
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

	camera.Update(dt, player.GetX(), player.GetY(), preventingmovement);

	// Helper: mouse cursor to world position
	auto MouseToWorld = [&]() -> AEVec2 {
		s32 mx, my;
		AEInputGetCursorPosition(&mx, &my);
		float halfW2 = (float)AEGfxGetWindowWidth() / 2.0f;
		float halfH2 = (float)AEGfxGetWindowHeight() / 2.0f;
		float worldX = ((float)mx - halfW2) + camera.GetX();
		float worldY = (halfH2 - (float)my) + camera.GetY();
		return { worldX, worldY };
	};

	// Debug: spawn a Walker at mouse position
	if (AEInputCheckTriggered(AEVK_1)) {
		AEVec2 pos = MouseToWorld();
		Wave.push_back(std::make_unique<Walker>(pos, ENEMY_SIZE, 120.0f, 220.0f));
		Wave.back()->Init();
		Wave.back()->SetMap(&gameMap);
	}

	// Debug: spawn a Dasher at mouse position
	if (AEInputCheckTriggered(AEVK_2)) {
		AEVec2 pos = MouseToWorld();
		Wave.push_back(std::make_unique<Dasher>(pos, ENEMY_SIZE, 100.0f, 250.0f, 3.0f));
		Wave.back()->Init();
		Wave.back()->SetMap(&gameMap);
	}

	// Debug: spawn a Thrower at mouse position
	if (AEInputCheckTriggered(AEVK_3)) {
		AEVec2 pos = MouseToWorld();
		Wave.push_back(std::make_unique<Thrower>(pos, ENEMY_SIZE, 50.0f, 100.0f));
		Wave.back()->Init();
		Wave.back()->SetMap(&gameMap);
	}

	// Debug: clear all enemies
	if (AEInputCheckTriggered(AEVK_K)) {
		Wave.clear();
	}

	// Back to main menu
	if (AEInputCheckTriggered(AEVK_BACK)) {
		next = GS_MAINMENU;
	}
}

void TestLevel_Draw() {
	AESysFrameStart();
	AEGfxSetBackgroundColor(0.68f, 0.85f, 0.90f);
	AEGfxSetCamPosition(camera.GetX(), camera.GetY());

	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	gameMap.Draw("Tile Layer 1");

	std::vector<RenderNode> renderQueue;
	renderQueue.push_back({ player.GetY(), [&]() { player.Draw(); } });

	for (auto& enemy : Wave) {
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

	Debug_DrawWorld(camera.GetX(), camera.GetY());

	HUD_Draw(&player, camera.GetX(), camera.GetY());
	Pause_Draw(camera.GetX(), camera.GetY());
	Debug_DrawHUD();

	AESysFrameEnd();
}

void TestLevel_Free() {
	Wave.clear();
	player.Free();
	Projectile::Free();
	g_Events.ClearAll();
}

void TestLevel_Unload() {
	Shadow_Free();
	AEGfxMeshFree(CircleMesh); CircleMesh = nullptr;
	AEGfxMeshFree(RectMesh);   RectMesh = nullptr;
	gameMap.Unload();
	Pause_Unload();
	HUD_Unload();
	AEAudioStopGroup(gAudio.audioGroup.BGM);
	Debug_Unload();
}
