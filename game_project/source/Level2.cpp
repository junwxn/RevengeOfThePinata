#include "pch.h"
#include "Utils.h"
#include "Player.h"
#include "Level2.h"
#include "camera.h"
#include "Augments.h"
#include "AugmentData.h"
#include "AugmentEffects.h"
#include "EventSystem.h"
#include "GameStateManager.h"
#include "Map.h"
#include "Pause.h"
#include "HUD.h"
#include "Audio.h"
#include "Debug.h"
#include "SaveSystem.h"
#include "Shadow.h"
#include "Projectile.h"
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
static Augments augments{};

// update variables
static WaveTimers waveControl;
static Combat::System CombatSystem;
static std::vector<std::unique_ptr<Enemy>> Wave1{};
static std::vector<std::unique_ptr<Enemy>> Wave2{};
static bool wave1Active{};
static bool wave2Active{};
static bool wave1Spawned{};
static bool wave2Spawned{};


// wave state
static bool endofwave{};
static bool preventingmovement{};

static void SpawnWave1_L2() {
	Wave1.clear();
	AEVec2 playerPos = { player.GetX(), player.GetY() };

	// Walker
	for (int i = 0; i < 3; ++i) {
		AEVec2 p1 = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
		Wave1.push_back(std::make_unique<Walker>(p1, ENEMY_SIZE, 100.0f, 200.0f));
	}
	// Dasher
	for (int i = 0; i < 1; ++i) {
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

static void SpawnWave2_L2() {
	Wave2.clear();
	AEVec2 playerPos = { player.GetX(), player.GetY() };

	// Walker
	for (int i = 0; i < 1; ++i) {
		AEVec2 p1 = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
		Wave2.push_back(std::make_unique<Walker>(p1, ENEMY_SIZE, 100.0f, 200.0f));
	}
	// Dasher
	for (int i = 0; i < 3; ++i) {
		AEVec2 p2 = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
		Wave2.push_back(std::make_unique<Dasher>(p2, ENEMY_SIZE, 100.0f, 200.0f, 3.0f));
	}

	// Thrower
	for (int i = 0; i < 1; ++i) {
		AEVec2 p3 = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
		Wave2.push_back(std::make_unique<Thrower>(p3, ENEMY_SIZE, 80.0f, 100.0f));
	}

	for (auto& enemy : Wave2) {
		enemy->Init();
		enemy->SetMap(&gameMap);
	}
}

void Level2_Load() {
	TexBlock2 = AEGfxTextureLoad("Assets/block2.png");
	TexBlock = AEGfxTextureLoad("Assets/block.png");
	CircleMesh = CreateCircleMesh(1.0f, 32, 0xFFFFFFFF);
	RectMesh = CreateRectMesh(0xFFFFFFFF);
	gameMap.Init("Assets/level2map.tmx", "tilesheet_complete", "Assets/tilesheet_complete.png");
	gameMap.BuildCollisionGrid("Tile Layer 2");
	Pause_Load();
	HUD_Load();
	Debug_Load();
	Shadow_Init();
}

void Level2_Init() {
	player.Init();
	player.SetAttackCharges(g_PlayerAttackCharges);
	player.SetMap(&gameMap);

	camera.Init(player.GetX(), player.GetY());
	Pause_Init();
	augments.Init();
	augments.SetAugmentSet(AugmentSet::SET_ATTACK);
	AugmentEffects_Init(&player);
	AugmentEffects_Register();

	// Auto-spawn wave 1
	wave1Active = false;
	wave2Active = false;
	wave1Spawned = false;
	wave2Spawned = false;
	endofwave = false;
	preventingmovement = false;

	SpawnWave1_L2();
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
	dbgCtx.waveCount = 2;
	dbgCtx.levelName = "Level 2";
	Debug_Register(dbgCtx);
}

void Level2_Update(float dt) {
	if (!AESysDoesWindowExist()) {
		SaveSystem_Save(GS_LEVEL2);
		Transition_StartImmediate(GS_QUIT);
		return;
	}

	if (Pause_Update(true)) return;
	Debug_Update();

	// Player death -> Game Over screen
	if (!player.GetIsAlive()) { Transition_StartImmediate(GS_GAMEOVER); return; }

	// Use Wave1 for player combat reference
	auto& activeWave = wave1Active ? Wave1 : Wave2;
	player.Update(dt, CombatSystem, activeWave, camera.GetX(), camera.GetY(), preventingmovement);

	// --- Wave 1 logic ---
// --- Wave 1 logic ---
	if (wave1Active) {
		Wave1.erase(
			std::remove_if(Wave1.begin(), Wave1.end(),
				[](const std::unique_ptr<Enemy>& e) {
					return e->GetCombatStats().health <= 0.0f;
				}),
			Wave1.end()
		);

		for (auto& enemy : Wave1) {
			enemy->Update(dt, CombatSystem, player, Wave1);
			CombatSystem.Update(player, *enemy, camera, dt);
		}

		if (Wave1.empty()) {
			wave1Active = false;
		}
	}

	// --- Wave 2 delayed spawn logic ---
	if (!wave1Active && !wave2Spawned)
	{
		waveControl.SetWaveTimer(dt);

		std::cout << "Timer: " << waveControl.GetWaveTimer()
			<< " / " << waveControl.GetWaveTrigger() << std::endl;

		if (waveControl.GetWaveTimer() >= waveControl.GetWaveTrigger())
		{
			std::cout << "SPAWNING WAVE 2" << std::endl;
			SpawnWave2_L2();
			wave2Active = true;
			wave2Spawned = true;
			waveControl.ResetWaveTimer();
		}
	}

	// --- Wave 2 logic ---
	if (wave2Active) {
		Wave2.erase(
			std::remove_if(Wave2.begin(), Wave2.end(),
				[](const std::unique_ptr<Enemy>& e) { return e->GetCombatStats().health <= 0.0f; }),
			Wave2.end()
		);

		for (auto& enemy : Wave2) {
			enemy->Update(dt, CombatSystem, player, Wave2);
			CombatSystem.Update(player, *enemy, camera, dt);
		}

		if (Wave2.empty()) {
			wave2Active = false;
			endofwave = true;
			augments.SetPosition(player.GetX(), player.GetY());
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
	AugmentEffects_Update(dt, player, wave1Active ? Wave1 : Wave2);


	// Augments — triggers after all waves cleared
	if (endofwave) {
		augments.Update(player.GetX(), player.GetY(), dt);
		if (augments.GetChoose()) {
			preventingmovement = true;
		}
		if (augments.GetAugmentSelected()) {
			Transition_Start(GS_LEVEL3, TransitionSheet::LEVEL3);
		}
	}
	else {
		preventingmovement = false;
	}

	if (0 == AESysDoesWindowExist()) {
		Transition_StartImmediate(GS_QUIT);
	}

	if (AEInputCheckTriggered(AEVK_K)) {
		Wave1.clear();
		Wave2.clear();
	}

	if (AEInputCheckTriggered(AEVK_N)) {
		Transition_Start(GS_LEVEL3);
	}
}

void Level2_Draw() {
	//AESysFrameStart();
	AEGfxSetBackgroundColor(0.68f, 0.85f, 0.90f);
	AEGfxSetCamPosition(camera.GetRenderX(), camera.GetRenderY());

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
	if (endofwave) {
		augments.Draw(camera.GetX(), camera.GetY());
	}

	//AESysFrameEnd();
}

void Level2_Free() {
	if (Transition_GetState() != current)
		g_PlayerAttackCharges = player.GetAttackCharges();
	int nextState = Transition_GetState();
	if (nextState >= GS_TUTORIAL && nextState <= GS_BOSSLEVEL)
		SaveSystem_Save(nextState);
	Wave1.clear();
	Wave2.clear();
	player.Free();
	augments.Free();
	AugmentEffects_Free();
	Projectile::Free();
	g_Events.ClearAll();
}

void Level2_Unload() {
	Shadow_Free();
	if (TexBlock)  { AEGfxTextureUnload(TexBlock);  TexBlock  = nullptr; }
	if (TexBlock2) { AEGfxTextureUnload(TexBlock2); TexBlock2 = nullptr; }
	AEGfxMeshFree(CircleMesh); CircleMesh = nullptr;
	AEGfxMeshFree(RectMesh);  RectMesh  = nullptr;
	gameMap.Unload();
	Pause_Unload();
	HUD_Unload();
	Debug_Unload();
	AEAudioStopGroup(gAudio.m_audioGroup.BGM);
}