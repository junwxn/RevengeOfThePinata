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
#include "Audio.h"

// load variables
static AEGfxTexture* TexBlock2;
static AEGfxTexture* TexBlock;
static AEGfxVertexList* CircleMesh;
static AEGfxVertexList* RectMesh;

// init variables
static Player player{};
static Camera camera{};
static MapSystem gameMap;
static s8 s_bossFont = -1;
static float s_bossLagHP = 0.0f;   // lags behind real HP
static Boss* GetBoss();

// update variables
static Combat::System CombatSystem;
static std::vector<std::unique_ptr<Enemy>> Wave1{};
static bool wave1Active{};

// wave state
static bool bossDefeated{};
static bool preventingmovement{};

// phase 2
static bool bossPhase2Active = false;
static bool bossPhase2Triggered = false;
static float bossMinionSpawnTimer = 0.0f;
static float bossMinionSpawnInterval = 4.0f;
static int bossMinionAliveLimit = 4;

static int CountBossMinions()
{
	int count = 0;
	for (auto& enemy : Wave1) {
		if (dynamic_cast<Boss*>(enemy.get()) == nullptr) {
			++count;
		}
	}
	return count;
}

static void SpawnBossMinionNearBoss()
{
	Boss* boss = GetBoss();
	if (!boss) return;

	AEVec2 bossPos{ boss->GetX(), boss->GetY() };

	// closer to boss
	AEVec2 spawnPos = GetRandomSpawnPos(gameMap, bossPos, 50.0f, ENEMY_SIZE);

	int roll = rand() % 10;

	if (roll < 5) {
		Wave1.push_back(std::make_unique<Walker>(spawnPos, ENEMY_SIZE, 100.0f, 200.0f));
	}
	else if (roll < 8) {
		Wave1.push_back(std::make_unique<Dasher>(spawnPos, ENEMY_SIZE, 100.0f, 200.0f, 3.0f));
	}
	else {
		Wave1.push_back(std::make_unique<Thrower>(spawnPos, ENEMY_SIZE, 80.0f, 100.0f));
	}

	Enemy* spawned = Wave1.back().get();
	spawned->Init();
	spawned->SetMap(&gameMap);
}

static void SpawnBossWave() {
	Wave1.clear();
	AEVec2 playerPos = { player.GetX(), player.GetY() };

	// 1 Boss (spawned on a valid tile) + 3 Walkers
	AEVec2 bossPos = GetRandomSpawnPos(gameMap, playerPos, 200.0f, BOSS_SIZE);
	Wave1.push_back(std::make_unique<Boss>(bossPos, 35.0f, 500.0f, 150.0f));
	//// Walker
	//for (int i = 0; i < 0; ++i) {
	//	AEVec2 p1 = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
	//	Wave1.push_back(std::make_unique<Walker>(p1, ENEMY_SIZE, 100.0f, 200.0f));
	//}
	//// Dasher
	//for (int i = 0; i < 0; ++i) {
	//	AEVec2 p2 = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
	//	Wave1.push_back(std::make_unique<Dasher>(p2, ENEMY_SIZE, 100.0f, 200.0f, 3.0f));
	//}

	//// Thrower
	//for (int i = 0; i < 0; ++i) {
	//	AEVec2 p3 = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
	//	Wave1.push_back(std::make_unique<Thrower>(p3, ENEMY_SIZE, 80.0f, 100.0f));
	//}

	for (auto& enemy : Wave1) {
		enemy->Init();
		enemy->SetMap(&gameMap);
	}

	Boss* boss = GetBoss();
	if (boss) {
		s_bossLagHP = boss->GetCombatStats().health;
	}
}

static Boss* GetBoss()
{
	for (auto& enemy : Wave1) {
		if (Boss* boss = dynamic_cast<Boss*>(enemy.get())) {
			return boss;
		}
	}
	return nullptr;
}

static void DrawBossHealthBar(float camX, float camY)
{
	Boss* boss = GetBoss();
	if (!boss) return;

	auto stats = boss->GetCombatStats();
	float hp = stats.health;
	float maxHp = stats.maxHealth;
	if (maxHp <= 0.0f) return;

	float ratio = AEClamp(hp / maxHp, 0.0f, 1.0f);
	float lagRatio = AEClamp(s_bossLagHP / maxHp, 0.0f, 1.0f);

	// Match HUD style (screen-space via cam offset)
	auto drawHUD = [&](float w, float h, float x, float y, float r, float g, float b, float a)
		{
			DrawMesh(RectMesh, w, h, x + camX, y + camY, 0.0f, r, g, b, a);
		};

	float barW = 800.0f;
	float barH = 40.0f;
	float barX = -barW * 0.5f;
	float barY = 400.0f; // top of screen

	// Border
	drawHUD(barW + 6, barH + 6, barX - 3, barY, 80, 80, 80, 200);

	// Background
	drawHUD(barW, barH, barX, barY, 30, 30, 30, 220);

	float fillW = barW * ratio;
	float lagFillW = barW * lagRatio;

	// ORANGE lag bar (behind)
	drawHUD(lagFillW, barH, barX, barY, 255, 140, 40, 220);

	// RED current HP (front)
	drawHUD(fillW, barH, barX, barY, 200, 40, 40, 255);

	// Text under bar (same system as HUD)
	float tw, th;
	AEGfxGetPrintSize(s_bossFont, "BOSS BABY", 0.8f, &tw, &th);

	float centerX = barX + barW * 0.5f;

	AEGfxPrint(
		s_bossFont,
		"BOSS BABY",
		centerX / 800.0f - tw * 0.5f,
		(barY - 60.0f) / 450.0f,
		0.8f,
		1.0f, 1.0f, 1.0f, 1.0f
	);
}

void BossLevel_Load() {
	TexBlock2 = AEGfxTextureLoad("Assets/block2.png");
	TexBlock = AEGfxTextureLoad("Assets/block.png");
	CircleMesh = CreateCircleMesh(1.0f, 32, 0xFFFFFFFF);
	RectMesh = CreateRectMesh(0xFFFFFFFF);
	gameMap.Init("Assets/bossmap.tmx", "tilesheet_complete", "Assets/tilesheet_complete.png");
	gameMap.BuildCollisionGrid("Tile Layer 2");
	s_bossFont = AEGfxCreateFont("Assets/fonts/Stick-Regular.ttf", 48);
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

	bossPhase2Active = false;
	bossPhase2Triggered = false;
	bossMinionSpawnTimer = 0.0f;

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
		Transition_StartImmediate(GS_QUIT);
		return;
	}

	if (Pause_Update(true)) return;
	Debug_Update();

	// Player death -> Game Over screen
	if (!player.GetIsAlive()) { Transition_StartImmediate(GS_GAMEOVER); return; }

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

		Boss* boss = GetBoss();
		if (boss) {
			auto stats = boss->GetCombatStats();
			float realHP = stats.health;

			// If boss heals → snap up instantly
			if (s_bossLagHP < realHP) {
				s_bossLagHP = realHP;
			}

			// Smoothly decrease when taking damage
			float diff = s_bossLagHP - realHP;
			if (diff > 0.0f) {
				float decaySpeed = 120.0f; // tweak this
				float step = decaySpeed * dt;

				if (step > diff) step = diff;

				s_bossLagHP -= step;
			}
		}

		// =========================
		// BOSS PHASE 2 TRIGGER
		// =========================
		if (boss && !bossPhase2Triggered && boss->GetGrowthHits() >= 5) {
			bossPhase2Triggered = true;
			bossPhase2Active = true;
			bossMinionSpawnTimer = bossMinionSpawnInterval;
		}

		// =========================
		// PHASE 2 MINION SPAWNING
		// =========================
		if (bossPhase2Active && boss) {
			bossMinionSpawnTimer -= dt;

			if (bossMinionSpawnTimer <= 0.0f) {
				bossMinionSpawnTimer = bossMinionSpawnInterval;

				if (CountBossMinions() < bossMinionAliveLimit) {
					SpawnBossMinionNearBoss();
				}
			}
		}

		if (!boss) {
			Wave1.clear(); // remove all minions instantly
			wave1Active = false;
			bossDefeated = true;
		}
	}

	// Boss defeated — go straight to victory (no augments)
	if (bossDefeated) {
		Transition_StartImmediate(GS_VICTORY);
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
		Transition_StartImmediate(GS_QUIT);
	}
	if (AEInputCheckTriggered(AEVK_K)) {
		Wave1.clear();
	}

	if (AEInputCheckTriggered(AEVK_N)) {
		Transition_StartImmediate(GS_VICTORY);
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
	DrawBossHealthBar(camera.GetX(), camera.GetY());
	Pause_Draw(camera.GetX(), camera.GetY());
	Debug_DrawHUD();
	AugmentEffects_Draw(camera.GetX(), camera.GetY());
}

void BossLevel_Free() {
	if (Transition_GetState() != current)
		g_PlayerAttackCharges = player.GetAttackCharges();
	Wave1.clear();
	player.Free();
	Projectile::Free();
	AugmentEffects_Free();
	g_Events.ClearAll();

	if (s_bossFont >= 0) {
		AEGfxDestroyFont(s_bossFont);
		s_bossFont = -1;
	}
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
	AEAudioStopGroup(gAudio.audioGroup.BGM);
}