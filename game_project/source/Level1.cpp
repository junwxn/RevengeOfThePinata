/*************************************************************************
@file       Level1.cpp
@Author     Nigel Lim, nigelkaiyu.lim@digipen.edu
@Co-authors nil
@brief      This file contains the implementation of Level 1, including
			wave spawning, combat updates, augment reward flow,
			rendering, cleanup, and level transition logic.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

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

// wave state
static bool endofwave{};
static bool preventingmovement{};
static Sprite m_ClearSprite;

// spawn wave 1 enemies
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

	// initialize all enemies
	for (auto& enemy : Wave1) {
		enemy->Init();
		enemy->SetMap(&gameMap);
	}

	gAudio.PlayGeneralSFX(GENERAL_ANNOUNCEMENT);
}

// spawn wave 2 enemies
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

	// initialize all enemies
	for (auto& enemy : Wave2) {
		enemy->Init();
		enemy->SetMap(&gameMap);
	}

	gAudio.PlayGeneralSFX(GENERAL_TRUMPET);
}

void Level1_Load() {
	// load textures, meshes, and map
	TexBlock2 = AEGfxTextureLoad("Assets/block2.png");
	TexBlock = AEGfxTextureLoad("Assets/block.png");
	CircleMesh = CreateCircleMesh(1.0f, 32, 0xFFFFFFFF);
	RectMesh = CreateRectMesh(0xFFFFFFFF);
	gameMap.Init("Assets/level1map.tmx", "tilesheet_complete", "Assets/tilesheet_complete.png");

	// build collision grid
	gameMap.BuildCollisionGrid("Tile Layer 2");

	Pause_Load();
	HUD_Load();
	Debug_Load();
	Shadow_Init();
}

void Level1_Init() {
	// initialize player and sprite
	m_ClearSprite.Sprite_Init();
	player.Init();
	//player.SetAttackCharges(g_PlayerAttackCharges);
	player.SetMap(&gameMap);

	// camera init
	camera.Init(player.GetX(), player.GetY());

	// initialize systems
	Pause_Init();
	augments.Init();
	augments.SetAugmentSet(AugmentSet::SET_DASH);
	AugmentEffects_Init(&player);
	AugmentEffects_Register();

	// reset level state
	wave1Active = false;
	wave2Active = false;
	wave1Spawned = false;
	wave2Spawned = false;
	endofwave = false;
	preventingmovement = false;
	pendingAugmentDrop = false;

	// auto start wave 1
	SpawnWave1();
	wave1Active = true;
	wave1Spawned = true;

	gAudio.PlayBGM(BGM_WAVE);

	// debug context
	Debug_Init();
	DebugContext dbgCtx = {};
	dbgCtx.player = &player;
	dbgCtx.camera = &camera;
	dbgCtx.map = &gameMap;
	dbgCtx.waves[0] = &Wave1;
	dbgCtx.waves[1] = &Wave2;
	dbgCtx.waveCount = 2;
	dbgCtx.levelName = "Level 1";
	Debug_Register(dbgCtx);
}

void Level1_Update(float dt) {
	// handle window close
	if (!AESysDoesWindowExist()) {
		SaveSystem_Save(GS_LEVEL1);
		Transition_StartImmediate(GS_QUIT);
		return;
	}

	// pause and debug update
	if (Pause_Update(true)) return;
	Debug_Update();

	// player death
	if (!player.GetIsAlive()) {
		Transition_StartImmediate(GS_GAMEOVER);
		return;
	}

	// active wave reference
	auto& activeWave = wave1Active ? Wave1 : Wave2;
	player.Update(dt, CombatSystem, activeWave, camera.GetX(), camera.GetY(), preventingmovement);

	// debug wave triggers
	if (AEInputCheckTriggered(AEVK_1)) {
		if (wave1Active) Wave1.clear();
		else SpawnWave1();
		wave1Active = !wave1Active;
	}
	else if (AEInputCheckTriggered(AEVK_2)) {
		if (wave2Active) Wave2.clear();
		else SpawnWave2();
		wave2Active = !wave2Active;
	}

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

	// --- Wave 2 spawn timer ---
	if (!wave1Active && !wave2Spawned) {
		waveControl.SetWaveTimer(dt);

		if (waveControl.GetWaveTimer() >= waveControl.GetWaveTrigger()) {
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
				[](const std::unique_ptr<Enemy>& e) {
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

	// camera update
	camera.Update(dt, player.GetX(), player.GetY(), preventingmovement);

	// update augment effects
	AugmentEffects_Update(dt, player, wave1Active ? Wave1 : Wave2);

	// augment selection logic
	if (endofwave) {
		augments.Update(player.GetX(), player.GetY(), dt);

		if (augments.GetChoose()) {
			preventingmovement = true;
		}

		if (augments.GetAugmentSelected()) {
			Transition_Start(GS_LEVEL2, TransitionSheet::LEVEL2);
		}
	}
	else {
		preventingmovement = false;
	}

	// debug shortcuts
	if (AEInputCheckTriggered(AEVK_K)) {
		Wave1.clear();
		Wave2.clear();
	}

	if (AEInputCheckTriggered(AEVK_N)) {
		Transition_Start(GS_LEVEL2, TransitionSheet::LEVEL2);
	}

	// update clear animation
	m_ClearSprite.Sprite_Update(dt);

	// drop augment after animation ends
	if (pendingAugmentDrop && !m_ClearSprite.IsClearAnimationActive()) {
		pendingAugmentDrop = false;
		endofwave = true;

		augments.SetPosition(player.GetX(), player.GetY());
		gAudio.PlayGeneralSFX(GENERAL_AUGMENT);
	}
}