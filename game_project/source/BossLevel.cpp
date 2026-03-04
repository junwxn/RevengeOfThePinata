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

// healthbar
static RectData Healthbar{};
static f32 Barcount{ 0 };
static f32 MinibarWidth = 100;
static u8 CurrentBars{ 0 };

// wave state
static bool bossDefeated{};
static bool preventingmovement{};

static void SpawnBossWave() {
	Wave1.clear();
	AEVec2 playerPos = { player.GetX(), player.GetY() };

	// 1 Boss (spawned on a valid tile) + 3 Walkers
	AEVec2 bossPos = GetRandomSpawnPos(gameMap, playerPos, 200.0f, 80.0f);
	Wave1.push_back(std::make_unique<Boss>(bossPos, 80.0f, 500.0f, 150.0f));
	for (int i = 0; i < 3; ++i) {
		AEVec2 p = GetRandomSpawnPos(gameMap, playerPos, 200.0f, 40.0f);
		Wave1.push_back(std::make_unique<Walker>(p, 40.0f, 100.0f, 200.0f));
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
	gameMap.Init("Assets/untitled.tmx", "tilesheet_complete", "Assets/tilesheet_complete.png");
	gameMap.BuildCollisionGrid("Tile Layer 2");
	Pause_Load();
}

void BossLevel_Init() {
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
	AugmentEffects_Init(&player);
	AugmentEffects_Register();

	wave1Active = false;
	bossDefeated = false;
	preventingmovement = false;

	SpawnBossWave();
	wave1Active = true;
}

void BossLevel_Update(float dt) {
	if (!AESysDoesWindowExist()) {
		next = GS_QUIT;
		return;
	}

	if (Pause_Update(true)) return;

	// Player death -> Game Over screen
	if (!player.GetIsAlive()) { next = GS_GAMEOVER; return; }

	player.Update(dt, CombatSystem, Wave1, camera.GetX(), camera.GetY(), preventingmovement);

	// --- Boss wave logic ---
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
			bossDefeated = true;
		}
	}

	// Boss defeated — go straight to victory (no augments)
	if (bossDefeated) {
		next = GS_VICTORY;
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

	if (Healthbar.var < 0) Healthbar.var = 0;
	if (Healthbar.var > 100) Healthbar.var = 100;
	Healthbar.current = (Healthbar.var / 100) * (Healthbar.max - Healthbar.min);
	Barcount = Healthbar.current / (Healthbar.w / 10);
	CurrentBars = (Barcount >= 1) ? 1 : 0;

	if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist()) {
		next = GS_QUIT;
	}
	if (AEInputCheckTriggered(AEVK_K)) {
		Wave1.clear();
	}

	if (AEInputCheckTriggered(AEVK_N)) {
		next = GS_VICTORY;
	}
}

void BossLevel_Draw() {
	AESysFrameStart();
	AEGfxSetBackgroundColor(0.1f, 0.05f, 0.15f);
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

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

	gameMap.QueueLayer("Tile Layer 2", renderQueue);

	std::sort(renderQueue.begin(), renderQueue.end(), [](const RenderNode& a, const RenderNode& b) {
		return a.y > b.y;
	});

	for (auto& node : renderQueue) {
		node.drawCall();
	}

	Pause_Draw();

	AugmentEffects_Draw();

	AESysFrameEnd();
}

void BossLevel_Free() {
	g_PlayerAttackCharges = player.GetAttackCharges();
	Wave1.clear();
	player.Free();
	AugmentEffects_Free();
	g_Events.ClearAll();
}

void BossLevel_Unload() {
	if (TexBlock)  { AEGfxTextureUnload(TexBlock);  TexBlock  = nullptr; }
	if (TexBlock2) { AEGfxTextureUnload(TexBlock2); TexBlock2 = nullptr; }
	AEGfxMeshFree(CircleMesh);
	AEGfxMeshFree(RectMesh);
	gameMap.Unload();
	Pause_Unload();
}