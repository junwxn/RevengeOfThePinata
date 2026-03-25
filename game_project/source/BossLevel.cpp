#include "pch.h"
#include "Utils.h"
#include "Player.h"
#include "BossLevel.h"
#include "camera.h"
#include "AugmentEffects.h"
#include "EventSystem.h"
#include "GameStateManager.h"
#include "Map.h"
#include "Pause.h"
#include "HUD.h"
#include "Debug.h"
#include "Shadow.h"
#include "Transition.h"

// load variables
static AEGfxTexture* TexBlock2;
static AEGfxTexture* TexBlock;
static AEGfxVertexList* CircleMesh;
static AEGfxVertexList* RectMesh;

// init variables
static Player player{};
static Camera camera{};
static MapSystem gameMap;

// update variables
static Combat::System CombatSystem;
static std::vector<std::unique_ptr<Enemy>> Wave1{};
static bool wave1Active{};

// wave state
static bool bossDefeated{};
static bool preventingmovement{};

static void SpawnBossWave() {
	Wave1.clear();
	AEVec2 playerPos = { player.GetX(), player.GetY() };

	// 1 Boss (spawned on a valid tile) + 3 Walkers
	AEVec2 bossPos = GetRandomSpawnPos(gameMap, playerPos, 200.0f, BOSS_SIZE);
	Wave1.push_back(std::make_unique<Boss>(bossPos, BOSS_SIZE, 500.0f, 150.0f));
	// Walker
	for (int i = 0; i < 3; ++i) {
		AEVec2 p1 = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
		Wave1.push_back(std::make_unique<Walker>(p1, ENEMY_SIZE, 100.0f, 200.0f));
	}
	// Dasher
	for (int i = 0; i < 0; ++i) {
		AEVec2 p2 = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
		Wave1.push_back(std::make_unique<Dasher>(p2, ENEMY_SIZE, 100.0f, 200.0f, 3.0f));
	}

	// Thrower
	for (int i = 0; i < 0; ++i) {
		AEVec2 p3 = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
		Wave1.push_back(std::make_unique<Thrower>(p3, ENEMY_SIZE, 80.0f, 100.0f));
	}

	for (auto& enemy : Wave1) {
		enemy->Init();
		enemy->SetMap(&gameMap);
	}
}

void BossLevel_Load() {
	TexBlock2 = AEGfxTextureLoad("Assets/block2.png");
	TexBlock = AEGfxTextureLoad("Assets/block.png");
	CircleMesh = CreateCircleMesh(1.0f, 32, 0xFFFFFFFF);
	RectMesh = CreateRectMesh(0xFFFFFFFF);
	gameMap.Init("Assets/bossmap.tmx", "tilesheet_complete", "Assets/tilesheet_complete.png");
	gameMap.BuildCollisionGrid("Tile Layer 2");
	Pause_Load();
	HUD_Load();
	Debug_Load();
	Shadow_Init();
}

void BossLevel_Init() {
	player.Init();
	player.SetAttackCharges(g_PlayerAttackCharges);
	player.SetMap(&gameMap);

	camera.Init(player.GetX(), player.GetY());
	Pause_Init();
	AugmentEffects_Init(&player);
	AugmentEffects_Register();

	wave1Active = false;
	bossDefeated = false;
	preventingmovement = false;

	SpawnBossWave();
	wave1Active = true;

	Debug_Init();
	DebugContext dbgCtx = {};
	dbgCtx.player    = &player;
	dbgCtx.camera    = &camera;
	dbgCtx.map       = &gameMap;
	dbgCtx.waves[0]  = &Wave1;
	dbgCtx.waveCount = 1;
	dbgCtx.levelName = "Boss Level";
	Debug_Register(dbgCtx);
}

void BossLevel_Update(float dt) {
	if (!AESysDoesWindowExist()) {
		Transition_Start(GS_QUIT);
		return;
	}

	if (Pause_Update(true)) return;
	Debug_Update();

	// Player death -> Game Over screen
	if (!player.GetIsAlive()) { Transition_Start(GS_GAMEOVER); return; }

	player.Update(dt, CombatSystem, Wave1, camera.GetX(), camera.GetY(), preventingmovement);

	// --- Boss wave logic ---
	if (wave1Active) {
		Wave1.erase(
			std::remove_if(Wave1.begin(), Wave1.end(),
				[](const std::unique_ptr<Enemy>& e) { return e->GetCombatStats().health <= 0.0f; }),
			Wave1.end()
		);

		for (auto& enemy : Wave1) {
			enemy->Update(dt, CombatSystem, player, Wave1);
			CombatSystem.Update(player, *enemy, camera, dt);
		}

		if (Wave1.empty()) {
			wave1Active = false;
			bossDefeated = true;
		}
	}

	// Boss defeated — go straight to victory (no augments)
	if (bossDefeated) {
		Transition_Start(GS_VICTORY);
	}

	// map boundaries
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

	// Update augment effects (previous augments still active)
	AugmentEffects_Update(dt, player, Wave1);

	if (0 == AESysDoesWindowExist()) {
		Transition_Start(GS_QUIT);
	}
	if (AEInputCheckTriggered(AEVK_K)) {
		Wave1.clear();
	}

	if (AEInputCheckTriggered(AEVK_N)) {
		Transition_Start(GS_VICTORY);
	}
}

void BossLevel_Draw() {
	//AESysFrameStart();
	AEGfxSetBackgroundColor(0.68f, 0.85f, 0.90f);
	AEGfxSetCamPosition(camera.GetRenderX(), camera.GetRenderY());
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	// map
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	gameMap.Draw("Tile Layer 1");

	std::vector<RenderNode> renderQueue;
	renderQueue.push_back({ player.GetY(), [&]() { player.Draw(); } });

	if (wave1Active) {
		for (auto& enemy : Wave1) {
			Enemy* ePtr = enemy.get();
			renderQueue.push_back({ ePtr->GetY(), [ePtr]() { ePtr->Draw(); } });
		}
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

	AugmentEffects_Draw(camera.GetX(), camera.GetY());

	//AESysFrameEnd();
}

void BossLevel_Free() {
	g_PlayerAttackCharges = player.GetAttackCharges();
	Wave1.clear();
	player.Free();
	Projectile::Free();
	AugmentEffects_Free();
	g_Events.ClearAll();
}

void BossLevel_Unload() {
	Shadow_Free();
	if (TexBlock)  { AEGfxTextureUnload(TexBlock);  TexBlock  = nullptr; }
	if (TexBlock2) { AEGfxTextureUnload(TexBlock2); TexBlock2 = nullptr; }
	AEGfxMeshFree(CircleMesh); CircleMesh = nullptr;
	AEGfxMeshFree(RectMesh);  RectMesh  = nullptr;
	gameMap.Unload();
	Pause_Unload();
	HUD_Unload();
	Debug_Unload();
}