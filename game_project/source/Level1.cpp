#include "pch.h"
#include "Utils.h"
#include "Player.h"
#include "Level1.h"
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
static f32 Barcount{ 0 };
static f32 MinibarWidth = 100;
static u8 CurrentBars{ 0 };
static bool pendingAugmentDrop = false;

// if wave ends
static bool endofwave{};
static bool preventingmovement{};
static Sprite m_ClearSprite;

static void SpawnWave1() {
	Wave1.clear();
	AEVec2 playerPos = { player.GetX(), player.GetY() };

	// Walker
	for (int i = 0; i < 2; ++i) {
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
		Wave1.push_back(std::make_unique<Thrower>(p3, ENEMY_SIZE, 100.0f, 100.0f));
	}



	for (auto& enemy : Wave1) {
		enemy->Init();
		enemy->SetMap(&gameMap);
	}

	gAudio.PlayGeneralSFX(GENERAL_ANNOUNCEMENT);
}

static void SpawnWave2() {
	Wave2.clear();
	AEVec2 playerPos = { player.GetX(), player.GetY() };

	// Walker
	for (int i = 0; i < 1; ++i) {
		AEVec2 p1 = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
		Wave2.push_back(std::make_unique<Walker>(p1, ENEMY_SIZE, 100.0f, 200.0f));
	}
	// Dasher
	for (int i = 0; i < 0; ++i) {
		AEVec2 p2 = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
		Wave2.push_back(std::make_unique<Dasher>(p2, ENEMY_SIZE, 100.0f, 200.0f, 3.0f));
	}

	// Thrower
	for (int i = 0; i < 2; ++i) {
		AEVec2 p3 = GetRandomSpawnPos(gameMap, playerPos, 200.0f, ENEMY_SIZE);
		Wave2.push_back(std::make_unique<Thrower>(p3, ENEMY_SIZE, 100.0f, 100.0f));
	}

	for (auto& enemy : Wave2) {
		enemy->Init();
		enemy->SetMap(&gameMap);
	}

	gAudio.PlayGeneralSFX(GENERAL_TRUMPET);

}

void Level1_Load() {
	TexBlock2	= AEGfxTextureLoad("Assets/block2.png");
	TexBlock	= AEGfxTextureLoad("Assets/block.png");
	CircleMesh	= CreateCircleMesh(1.0f, 32, 0xFFFFFFFF);
	RectMesh	= CreateRectMesh(0xFFFFFFFF);
	gameMap.Init("Assets/level1map.tmx", "tilesheet_complete", "Assets/tilesheet_complete.png");

	// Build the binary collision grid from the wall layer.
	gameMap.BuildCollisionGrid("Tile Layer 2");

	Pause_Load();
	HUD_Load();
	Debug_Load();
	Shadow_Init();
}
void Level1_Init() {
	m_ClearSprite.Sprite_Init();
	player.Init();
	//player.SetAttackCharges(g_PlayerAttackCharges);
	player.SetMap(&gameMap); // Enable player-map collision

	// camera init
	camera.Init(player.GetX(), player.GetY());

	Pause_Init();
	augments.Init();
	augments.SetAugmentSet(AugmentSet::SET_DASH);
	AugmentEffects_Init(&player);
	AugmentEffects_Register();

	// Auto-spawn wave 1
	wave1Active = false;
	wave2Active = false;
	wave1Spawned = false;
	wave2Spawned = false;
	endofwave = false;
	preventingmovement = false;
	pendingAugmentDrop = false;

	SpawnWave1();
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
	dbgCtx.levelName = "Level 1";
	Debug_Register(dbgCtx);
}
void Level1_Update(float dt) {
	if (!AESysDoesWindowExist()) {
		SaveSystem_Save(GS_LEVEL1);
		Transition_StartImmediate(GS_QUIT);
		return;
	}

	// Pause handles ESC toggle + menu input; returns true if paused
	if (Pause_Update(true)) return;
	Debug_Update();

	// Player death -> Game Over screen
	if (!player.GetIsAlive()) { Transition_StartImmediate(GS_GAMEOVER); return; }

	auto& activeWave = wave1Active ? Wave1 : Wave2;
	player.Update(dt, CombatSystem, activeWave, camera.GetX(), camera.GetY(), preventingmovement);

	// Debug wave triggers (keys 1/2 still work)
	if (AEInputCheckTriggered(AEVK_1)) {
		if (wave1Active) {
			Wave1.clear();
		}
		else {
			SpawnWave1();
		}
		wave1Active = !wave1Active;
	}
	else if (AEInputCheckTriggered(AEVK_2)) {
		if (wave2Active) {
			Wave2.clear();
		}
		else {
			SpawnWave2();
		}
		wave2Active = !wave2Active;
	}

	// --- Wave 1 logic ---
	if (wave1Active) {
		Wave1.erase(
			std::remove_if(Wave1.begin(), Wave1.end(),
				[](const std::unique_ptr<Enemy>& e)
				{
					return e->GetCombatStats().health <= 0.0f;
				}),
			Wave1.end()
		);

		for (auto& enemy : Wave1) {
			enemy->Update(dt, CombatSystem, player, Wave1);
			CombatSystem.Update(player, *enemy, camera, dt);
		}

		std::cout << "Wave1 size: " << Wave1.size()
			<< ", wave2Spawned: " << wave2Spawned << std::endl;

		if (Wave1.empty()) {
			wave1Active = false;
		}
	}

	// --- Wave 2 spawn timer logic ---
	if (!wave1Active && !wave2Spawned)
	{
		waveControl.SetWaveTimer(dt);

		std::cout << "Timer: " << waveControl.GetWaveTimer()
			<< " / " << waveControl.GetWaveTrigger() << std::endl;

		if (waveControl.GetWaveTimer() >= waveControl.GetWaveTrigger())
		{
			std::cout << "SPAWNING WAVE 2" << std::endl;
			SpawnWave2();
			wave2Active = true;
			wave2Spawned = true;
			waveControl.ResetWaveTimer();
		}
	}

	// --- Wave 2 logic ---
	if (wave2Active) {
		Wave2.erase(
			std::remove_if(Wave2.begin(), Wave2.end(),
				[](const std::unique_ptr<Enemy>& e)
				{
					return e->GetCombatStats().health <= 0.0f;
				}),
			Wave2.end()
		);

		for (auto& enemy : Wave2) {
			enemy->Update(dt, CombatSystem, player, Wave2);
			CombatSystem.Update(player, *enemy, camera, dt);
		}


		if (Wave2.empty()) {
			wave2Active = false;
			pendingAugmentDrop = true;
			gAudio.PlayFireworksSFX();
			m_ClearSprite.StartClearAnimation(3.0f, 0.12f);
		}
	}

	// map boundaries
	float halfW = GRID_W * 0.5f;
	float halfH = GRID_H * 0.5f;

	// Convert Screen -> Grid
	float invX = player.GetX() / halfW;
	float invY = player.GetY() / halfH;
	float gridX = 0.5f * (invX + invY);
	float gridY = 0.5f * (invY - invX);

	// --- Map Limits ---
		// We apply the same -10 offset here that is used in MapSystem::Draw
	const float GRID_OFFSET = -10.0f;

	// Min limits are the starting grid index (0) plus the offset
	float MAP_MIN_X = 1.0f + GRID_OFFSET;
	float MAP_MIN_Y = 0.0f + GRID_OFFSET;

	// Max limits are the map width/height minus 1 (to stay on the tile), plus the offset
	// We use max(1, width) to prevent crashes if the map fails to load
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

	// camera
	camera.Update(dt, player.GetX(), player.GetY(), preventingmovement);

	// Update augment effects
	AugmentEffects_Update(dt, player, wave1Active ? Wave1 : Wave2);

	// Debug augment trigger (O key still works)
	if (AEInputCheckTriggered(AEVK_O)) {
		std::cout << "AUGMENTS TRIGGERED" << std::endl;
		endofwave = true;
		augments.SetPosition(player.GetX(), player.GetY());
	}

	if (endofwave) {
		
		augments.Update(player.GetX(), player.GetY(), dt);
		if (augments.GetChoose()) {
			preventingmovement = true;
		}
		if (augments.GetAugmentSelected()) {
			Transition_Start(GS_LEVEL2, TransitionSheet::LEVEL2);
		}
		if (AEInputCheckTriggered(AEVK_P)) {
			std::cout << "AUGMENTS TRIGGERED AGAIN" << std::endl;
			endofwave = false;
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
		Transition_Start(GS_LEVEL2, TransitionSheet::LEVEL2);
	}

	m_ClearSprite.Sprite_Update(dt);

	if (pendingAugmentDrop && !m_ClearSprite.IsClearAnimationActive()) {
		pendingAugmentDrop = false;
		endofwave = true;

		augments.SetPosition(player.GetX(), player.GetY());
		gAudio.PlayGeneralSFX(GENERAL_AUGMENT);
	}
}

void Level1_Draw() {
	//AESysFrameStart();
	AEGfxSetBackgroundColor(0.68f, 0.85f, 0.90f);
	AEGfxSetCamPosition(camera.GetRenderX(), camera.GetRenderY());

	//draw map
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	// 1. Draw the floor/background layer FIRST
	gameMap.Draw("Tile Layer 1");

	// 2. Draw Layer 2 blocks that are BEHIND the player
	std::vector<RenderNode> renderQueue;

	// Add the Player
	renderQueue.push_back({ player.GetY(), [&]() { player.Draw(); } });

	// Add Wave 1 Enemies
	if (wave1Active) {
		for (auto& enemy : Wave1) {
			Enemy* ePtr = enemy.get();
			renderQueue.push_back({ ePtr->GetY(), [ePtr]() { ePtr->Draw(); } });
		}
	}

	// Add Wave 2 Enemies
	if (wave2Active) {
		for (auto& enemy : Wave2) {
			Enemy* ePtr = enemy.get();
			renderQueue.push_back({ ePtr->GetY(), [ePtr]() { ePtr->Draw(); } });
		}
	}

	// Ask the Map to push every block in Layer 2 into the queue!
	gameMap.QueueLayer("Tile Layer 2", renderQueue);

	// --- 3. SORT THE ENTIRE WORLD ---
	// Sorts from Highest Y (Furthest Back) to Lowest Y (Closest to Front)
	std::sort(renderQueue.begin(), renderQueue.end(), [](const RenderNode& a, const RenderNode& b) {
		return a.y > b.y;
		});

	// --- 4. EXECUTE ALL DRAW CALLS ---
	for (auto& node : renderQueue) {
		node.drawCall();
	}

	Debug_DrawWorld(camera.GetX(), camera.GetY());

	HUD_Draw(&player, camera.GetX(), camera.GetY());
	Debug_DrawHUD();

	// Clear Animation
	AugmentEffects_Draw(camera.GetX(), camera.GetY());
	if (endofwave) {
		augments.Draw(camera.GetX(), camera.GetY());
	}

	DrawClearOverlay(m_ClearSprite);
	Pause_Draw(camera.GetX(), camera.GetY());

}

void Level1_Free() {
	std::cout << "FREEING LEVEL 1" << std::endl;
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
void Level1_Unload() {
	Shadow_Free();
	if (TexBlock)  { AEGfxTextureUnload(TexBlock);  TexBlock  = nullptr; }
	if (TexBlock2) { AEGfxTextureUnload(TexBlock2); TexBlock2 = nullptr; }
	AEGfxMeshFree(CircleMesh); CircleMesh = nullptr;
	AEGfxMeshFree(RectMesh);  RectMesh = nullptr;
	gameMap.Unload();
	Pause_Unload();
	HUD_Unload();
	AEAudioStopGroup(gAudio.m_audioGroup.BGM);
	Debug_Unload();
}