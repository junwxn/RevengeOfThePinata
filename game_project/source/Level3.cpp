#include "pch.h"
#include "Utils.h"
#include "Player.h"
#include "Level3.h"
#include "camera.h"
#include "Augments.h"
#include "AugmentData.h"
#include "AugmentEffects.h"
#include "EventSystem.h"
#include "GameStateManager.h"
#include "Map.h"
#include "Pause.h"
#include "Audio.h"
#include "Debug.h"

// load variables
static AEGfxTexture* TexBlock2;
static AEGfxTexture* TexBlock;
static AEGfxVertexList* CircleMesh;
static AEGfxVertexList* RectMesh;

// init variables
static Player player{};
static Camera camera{};
static MapSystem gameMap;
static Augments augments{};

// update variables
static Combat::System CombatSystem;
static std::vector<std::unique_ptr<Enemy>> Wave1{};
static std::vector<std::unique_ptr<Enemy>> Wave2{};
static std::vector<std::unique_ptr<Enemy>> Wave3{};
static bool wave1Active{};
static bool wave2Active{};
static bool wave3Active{};
static bool wave1Spawned{};
static bool wave2Spawned{};
static bool wave3Spawned{};

// healthbar
static RectData Healthbar{};
static f32 Barcount{ 0 };
static f32 MinibarWidth = 100;
static u8 CurrentBars{ 0 };

// wave state
static bool endofwave{};
static bool preventingmovement{};

static void SpawnWave1_L3() {
	Wave1.clear();
	AEVec2 playerPos = { player.GetX(), player.GetY() };

	// 5 Walkers + 3 Dashers
	for (int i = 0; i < 5; ++i) {
		AEVec2 p = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
		Wave1.push_back(std::make_unique<Walker>(p, ENEMY_SIZE, 150.0f, 240.0f));
	}
	for (int i = 0; i < 3; ++i) {
		AEVec2 p = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
		Wave1.push_back(std::make_unique<Dasher>(p, ENEMY_SIZE, 120.0f, 270.0f, 3.0f));
	}
	for (auto& enemy : Wave1) {
		enemy->Init();
		enemy->SetMap(&gameMap);
	}
}

static void SpawnWave2_L3() {
	Wave2.clear();
	AEVec2 playerPos = { player.GetX(), player.GetY() };

	// 8 Dashers
	for (int i = 0; i < 8; ++i) {
		AEVec2 p = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
		Wave2.push_back(std::make_unique<Dasher>(p, ENEMY_SIZE, 120.0f, 270.0f, 3.0f));
	}
	for (auto& enemy : Wave2) {
		enemy->Init();
		enemy->SetMap(&gameMap);
	}
}

static void SpawnWave3_L3() {
	Wave3.clear();
	AEVec2 playerPos = { player.GetX(), player.GetY() };

	// 5 Walkers + 5 Dashers
	for (int i = 0; i < 5; ++i) {
		AEVec2 p = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
		Wave3.push_back(std::make_unique<Walker>(p, ENEMY_SIZE, 150.0f, 240.0f));
	}
	for (int i = 0; i < 5; ++i) {
		AEVec2 p = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
		Wave3.push_back(std::make_unique<Dasher>(p, ENEMY_SIZE, 120.0f, 270.0f, 3.0f));
	}
	for (auto& enemy : Wave3) {
		enemy->Init();
		enemy->SetMap(&gameMap);
	}
}

void Level3_Load() {
	TexBlock2 = AEGfxTextureLoad("Assets/block2.png");
	TexBlock = AEGfxTextureLoad("Assets/block.png");
	CircleMesh = CreateCircleMesh(1.0f, 32, 0xFFFFFFFF);
	RectMesh = CreateRectMesh(0xFFFFFFFF);
	gameMap.Init("Assets/level3map.tmx", "tilesheet_complete", "Assets/tilesheet_complete.png");
	gameMap.BuildCollisionGrid("Tile Layer 2");
	Pause_Load();
	Debug_Load();
}

void Level3_Init() {
	player.Init();
	player.SetAttackCharges(g_PlayerAttackCharges);
	player.SetMap(&gameMap);

	Healthbar.w = 1200;
	Healthbar.h = 50;
	Healthbar.pos_x = -Healthbar.w / 2;
	Healthbar.pos_y = 350;
	Healthbar.max = Healthbar.pos_x + Healthbar.w;
	Healthbar.min = Healthbar.pos_x;
	Healthbar.var = 100;
	Healthbar.current = (Healthbar.var / 100) * (Healthbar.max - Healthbar.min);

	camera.Init(player.GetX(), player.GetY());
	Pause_Init();
	augments.Init();
	augments.SetAugmentSet(AugmentSet::SET_PARRY);
	AugmentEffects_Init(&player);
	AugmentEffects_Register();

	wave1Active = false;
	wave2Active = false;
	wave3Active = false;
	wave1Spawned = false;
	wave2Spawned = false;
	wave3Spawned = false;
	endofwave = false;
	preventingmovement = false;

	SpawnWave1_L3();
	wave1Active = true;
	wave1Spawned = true;

	gAudio.PlayBGM(BGM_WAVE);
	Debug_Init();
	DebugContext dbgCtx = {};
	dbgCtx.player    = &player;
	dbgCtx.camera    = &camera;
	dbgCtx.map       = &gameMap;
	dbgCtx.waves[0]  = &Wave1;
	dbgCtx.waves[1]  = &Wave2;
	dbgCtx.waves[2]  = &Wave3;
	dbgCtx.waveCount = 3;
	dbgCtx.levelName = "Level 3";
	Debug_Register(dbgCtx);
}

void Level3_Update(float dt) {
	if (!AESysDoesWindowExist()) {
		next = GS_QUIT;
		return;
	}

	if (Pause_Update(true)) return;
	Debug_Update();

	// Player death -> Game Over screen
	if (!player.GetIsAlive()) { next = GS_GAMEOVER; return; }

	// Pick the active wave for player combat
	std::vector<std::unique_ptr<Enemy>>* activeWavePtr = &Wave1;
	if (wave2Active) activeWavePtr = &Wave2;
	if (wave3Active) activeWavePtr = &Wave3;
	player.Update(dt, CombatSystem, *activeWavePtr, camera.GetX(), camera.GetY(), preventingmovement);

	// --- Wave 1 ---
	if (wave1Active) {
		Wave1.erase(
			std::remove_if(Wave1.begin(), Wave1.end(),
				[](const std::unique_ptr<Enemy>& e) { return e->GetCombatStats().health <= 0.0f; }),
			Wave1.end()
		);
		for (auto& enemy : Wave1) {
			enemy->Update(dt, CombatSystem, player);
			CombatSystem.Update(player, *enemy, camera, dt);
		}
		for (auto& enemy : Wave1) {
			if (enemy->GetCombatFlag().attackHit) {
				if (!player.GetCombatFlag().parryOn) {
					if (player.GetCombatFlag().blockOn) Healthbar.var -= (player.GetCombatStats().attack) / 2;
					else Healthbar.var -= player.GetCombatStats().attack;
				}
			}
		}
		if (Wave1.empty()) {
			wave1Active = false;
			if (!wave2Spawned) {
				SpawnWave2_L3();
				wave2Active = true;
				wave2Spawned = true;
			}
		}
	}

	// --- Wave 2 ---
	if (wave2Active) {
		Wave2.erase(
			std::remove_if(Wave2.begin(), Wave2.end(),
				[](const std::unique_ptr<Enemy>& e) { return e->GetCombatStats().health <= 0.0f; }),
			Wave2.end()
		);
		for (auto& enemy : Wave2) {
			enemy->Update(dt, CombatSystem, player);
			CombatSystem.Update(player, *enemy, camera, dt);
		}
		for (auto& enemy : Wave2) {
			if (enemy->GetCombatFlag().attackHit) {
				if (!player.GetCombatFlag().parryOn) {
					if (player.GetCombatFlag().blockOn) Healthbar.var -= (player.GetCombatStats().attack) / 2;
					else Healthbar.var -= player.GetCombatStats().attack;
				}
			}
		}
		if (Wave2.empty()) {
			wave2Active = false;
			if (!wave3Spawned) {
				SpawnWave3_L3();
				wave3Active = true;
				wave3Spawned = true;
			}
		}
	}

	// --- Wave 3 ---
	if (wave3Active) {
		Wave3.erase(
			std::remove_if(Wave3.begin(), Wave3.end(),
				[](const std::unique_ptr<Enemy>& e) { return e->GetCombatStats().health <= 0.0f; }),
			Wave3.end()
		);
		for (auto& enemy : Wave3) {
			enemy->Update(dt, CombatSystem, player);
			CombatSystem.Update(player, *enemy, camera, dt);
		}
		for (auto& enemy : Wave3) {
			if (enemy->GetCombatFlag().attackHit) {
				if (!player.GetCombatFlag().parryOn) {
					if (player.GetCombatFlag().blockOn) Healthbar.var -= (player.GetCombatStats().attack) / 2;
					else Healthbar.var -= player.GetCombatStats().attack;
				}
			}
		}
		if (Wave3.empty()) {
			wave3Active = false;
			endofwave = true;
		}
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

	// Update augment effects
	AugmentEffects_Update(dt, player, *activeWavePtr);

	if (Healthbar.var < 0) Healthbar.var = 0;
	if (Healthbar.var > 100) Healthbar.var = 100;
	Healthbar.current = (Healthbar.var / 100) * (Healthbar.max - Healthbar.min);
	Barcount = Healthbar.current / (Healthbar.w / 10);
	CurrentBars = (Barcount >= 1) ? 1 : 0;

	// Augments
	if (endofwave) {
		augments.Update(player.GetX(), player.GetY(), dt, camera.GetX(), camera.GetY());
		if (augments.GetChoose()) {
			preventingmovement = true;
		}
		if (augments.GetAugmentSelected()) {
			next = GS_BOSSLEVEL;
		}
	}
	else {
		preventingmovement = false;
	}

	if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist()) {
		next = GS_QUIT;
	}

	if (AEInputCheckTriggered(AEVK_K)) {
		Wave1.clear();
		Wave2.clear();
		Wave3.clear();
	}

	if (AEInputCheckTriggered(AEVK_N)) {
		next = GS_BOSSLEVEL;
	}
}

void Level3_Draw() {
	AESysFrameStart();
	AEGfxSetBackgroundColor(0.68f, 0.85f, 0.90f);

	// healthbar
	DrawMesh(RectMesh, Healthbar.w, Healthbar.h, Healthbar.pos_x, Healthbar.pos_y, 0, 255, 0, 0, 150);
	DrawMesh(RectMesh, Healthbar.current, Healthbar.h, Healthbar.pos_x, Healthbar.pos_y, 0, 255, 0, 0, 255);

	int tempBars = CurrentBars;
	while (tempBars <= Barcount && tempBars != 0) {
		float xPos = (tempBars == 1) ? Healthbar.min : Healthbar.min + (tempBars - 1) * ((Healthbar.w / 10) + (((Healthbar.w / 10.0f) - MinibarWidth) / 9.0f));
		DrawMesh(RectMesh, MinibarWidth, Healthbar.h, xPos, Healthbar.pos_y - 80, 0, 255, 0, 0, 255);
		tempBars++;
	}
	if (Healthbar.var != 0) {
		DrawMesh(RectMesh, MinibarWidth, Healthbar.h, Healthbar.min, Healthbar.pos_y - 80, 0, 255, 0, 0, 255);
	}

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
	if (wave2Active) {
		for (auto& enemy : Wave2) {
			Enemy* ePtr = enemy.get();
			renderQueue.push_back({ ePtr->GetY(), [ePtr]() { ePtr->Draw(); } });
		}
	}
	if (wave3Active) {
		for (auto& enemy : Wave3) {
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

	Pause_Draw();
	Debug_DrawHUD();

	AugmentEffects_Draw();

	if (endofwave) {
		augments.Draw(player.GetX(), player.GetY());
	}

	AESysFrameEnd();
}

void Level3_Free() {
	g_PlayerAttackCharges = player.GetAttackCharges();
	Wave1.clear();
	Wave2.clear();
	Wave3.clear();
	player.Free();
	augments.Free();
	AugmentEffects_Free();
	g_Events.ClearAll();
}

void Level3_Unload() {
	if (TexBlock)  { AEGfxTextureUnload(TexBlock);  TexBlock  = nullptr; }
	if (TexBlock2) { AEGfxTextureUnload(TexBlock2); TexBlock2 = nullptr; }
	AEGfxMeshFree(CircleMesh); CircleMesh = nullptr;
	AEGfxMeshFree(RectMesh);  RectMesh  = nullptr;
	gameMap.Unload();
	Pause_Unload();
	Debug_Unload();
}