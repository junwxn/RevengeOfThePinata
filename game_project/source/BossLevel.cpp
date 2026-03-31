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
#include "SaveSystem.h"

// load variables
static AEGfxTexture* TexBlock2;
static AEGfxTexture* TexBlock;
static AEGfxVertexList* CircleMesh;
static AEGfxVertexList* RectMesh;

// init variables
static Player player{};
static Camera camera{};
static Augments augments{};
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
static float bossMinionSpawnInterval = 2.5f;
static int bossMinionAliveLimit = 10;
static bool bossPhaseHealActive = false;
static float bossPhaseHealSpeed = 120.0f;
static bool bossInitialMinionsSpawned = false;

// phase 3
static bool bossPhase3AddsSpawned = false;
static Thrower* bossPhase3Gun = nullptr;
static float bossPhase3SpawnTimer = 0.0f;
static float bossPhase3SpawnInterval = 1.5f;
static int bossPhase3MaxThrowers = 10;

// phase 4
static bool clearAddsForPhase4 = false;
static bool phase4CleanupDone = false;
static bool bossPhase4DropActive = false;

static int CountBossMinions()
{
	int count = 0;
	for (auto& enemy : Wave1) {
		Enemy* ePtr = enemy.get();

		if (dynamic_cast<Boss*>(ePtr) == nullptr) {
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

	const float minSpawnDist = boss->GetSize() + ENEMY_SIZE + 10.0f;
	const float maxSpawnDist = minSpawnDist + 25.0f;

	AEVec2 spawnPos{};
	bool found = false;

	for (int i = 0; i < 16; ++i) {
		float angle = AERandFloat() * 2.0f * PI;
		float dist = minSpawnDist + AERandFloat() * (maxSpawnDist - minSpawnDist);

		spawnPos.x = bossPos.x + cosf(angle) * dist;
		spawnPos.y = bossPos.y + sinf(angle) * dist;

		if (!gameMap.IsPositionBlocked(spawnPos.x, spawnPos.y, ENEMY_SIZE)) {
			found = true;
			break;
		}
	}

	if (!found) return;

	int roll = rand() % 10;

	if (roll < 4) {
		Wave1.push_back(std::make_unique<Walker>(spawnPos, ENEMY_SIZE, 100.0f, 200.0f));
	}
	else if (roll < 6) {
		Wave1.push_back(std::make_unique<Dasher>(spawnPos, ENEMY_SIZE, 100.0f, 200.0f, 3.0f));
	}
	else {
		Wave1.push_back(std::make_unique<Thrower>(spawnPos, ENEMY_SIZE, 80.0f, 100.0f));
	}

	Enemy* spawned = Wave1.back().get();
	spawned->Init();
	spawned->SetMap(&gameMap);
}

static void SpawnPhase3Thrower()
{
	AEVec2 playerPos{ player.GetX(), player.GetY() };

	float radius = 220.0f; // distance from player
	float angle = AERandFloat() * 2.0f * PI;

	AEVec2 spawnPos;
	spawnPos.x = playerPos.x + cosf(angle) * radius;
	spawnPos.y = playerPos.y + sinf(angle) * radius;

	// simple wall check (retry a few times)
	for (int i = 0; i < 5; ++i) {
		if (!gameMap.IsPositionBlocked(spawnPos.x, spawnPos.y, ENEMY_SIZE))
			break;

		angle = AERandFloat() * 2.0f * PI;
		spawnPos.x = playerPos.x + cosf(angle) * radius;
		spawnPos.y = playerPos.y + sinf(angle) * radius;
	}

	Wave1.push_back(std::make_unique<Thrower>(spawnPos, ENEMY_SIZE, 80.0f, 100.0f));

	Enemy* spawned = Wave1.back().get();
	spawned->Init();
	spawned->SetMap(&gameMap);
}

static void SpawnBossWave() {
	Wave1.clear();
	AEVec2 playerPos = { player.GetX(), player.GetY() };

	// 1 Boss (spawned on a valid tile) + 3 Walkers
	AEVec2 bossPos = GetRandomSpawnPos(gameMap, playerPos, 200.0f, BOSS_SIZE);
	Wave1.push_back(std::make_unique<Boss>(
		bossPos, // Position
		20.0f, // Size
		800.0f, // HP
		150.0f // Speed
	));

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

	float displayHP = bossPhaseHealActive ? s_bossLagHP : hp;

	float ratio = AEClamp(displayHP / maxHp, 0.0f, 1.0f);
	float lagRatio = bossPhaseHealActive ? ratio : AEClamp(s_bossLagHP / maxHp, 0.0f, 1.0f);

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
	AEGfxGetPrintSize(s_bossFont, "DE BOSS BABY", 0.8f, &tw, &th);

	float centerX = barX + barW * 0.5f;

	AEGfxPrint(
		s_bossFont,
		"DE BOSS BABY",
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
	augments.Init();
	augments.SetAugmentSet(AugmentSet::SET_PARRY);
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
	bossInitialMinionsSpawned = false;

	bossPhase3Gun = nullptr;

	clearAddsForPhase4 = false;
	phase4CleanupDone = false;
	bossPhase4DropActive = false;

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
		SaveSystem_Save(GS_BOSSLEVEL);
		Transition_StartImmediate(GS_QUIT);
		return;
	}

	if (Pause_Update(true)) return;
	Debug_Update();

	// Player death -> Game Over screen
	if (!player.GetIsAlive()) { Transition_StartImmediate(GS_GAMEOVER); return; }

	player.Update(dt, CombatSystem, Wave1, camera.GetX(), camera.GetY(), preventingmovement);


	if (wave1Active) {
		Wave1.erase(
			std::remove_if(Wave1.begin(), Wave1.end(),
				[](const std::unique_ptr<Enemy>& e)
				{
					if (Boss* boss = dynamic_cast<Boss*>(e.get())) {
						// Boss cannot truly die while the fake-drop / ball state is active
						if (boss->IsPhase4Triggered() && !boss->IsPhase4BallVisible()) {
							return boss->GetCombatStats().health <= 0.0f;
						}

						return false;
					}

					return e->GetCombatStats().health <= 0.0f;
				}),
			Wave1.end()
		);


		// =========================
		// BOSS PHASE 1
		// =========================
		for (auto& enemy : Wave1) {
			enemy->Update(dt, CombatSystem, player, Wave1);

			if (Boss* bossEnemy = dynamic_cast<Boss*>(enemy.get())) {
				if (bossEnemy->IsPhaseTransitioning() ||
					bossEnemy->IsPhaseBlinking() ||
					bossEnemy->IsPhase3Transitioning() ||
					bossEnemy->IsPhase3Blinking() ||
					bossEnemy->IsPhase4BallVisible() ||
					bossEnemy->IsPhase4Blinking()) {
					continue;
				}
			}

			if (enemy.get() != bossPhase3Gun) {
				CombatSystem.Update(player, *enemy, camera, dt);
			}
		}

		if (Boss* phase4Boss = GetBoss()) {
			if (phase4Boss->IsPhase4Triggered() && !phase4CleanupDone) {
				clearAddsForPhase4 = true;
			}
		}

		if (Boss* phase4Boss = GetBoss()) {
			if (phase4Boss->IsPhase4Triggered() && phase4Boss->IsPhase4BallVisible()) {
				if (!bossPhase4DropActive) {
					AEVec2 ballPos = phase4Boss->GetPhase4BallPos();
					augments.SetPosition(ballPos.x, ballPos.y);
					bossPhase4DropActive = true;
				}
			}
			else if (!phase4Boss->IsPhase4BallVisible()) {
				bossPhase4DropActive = false;
			}
		}
		else {
			bossPhase4DropActive = false;
		}

		if (clearAddsForPhase4) {
			Wave1.erase(
				std::remove_if(Wave1.begin(), Wave1.end(),
					[](const std::unique_ptr<Enemy>& e) {
						return dynamic_cast<Boss*>(e.get()) == nullptr;
					}),
				Wave1.end()
			);

			bossPhase3Gun = nullptr;
			bossPhase3AddsSpawned = false;
			clearAddsForPhase4 = false;
			phase4CleanupDone = true;
		}

		if (Boss* phase4Boss = GetBoss()) {
			if (phase4Boss->IsPhase4Triggered() && !phase4CleanupDone) {
				clearAddsForPhase4 = true;
			}
		}

		Boss* boss = GetBoss();

		// =========================
		// BOSS PHASE 3 Minion spawn
		// =========================
		if (boss && boss->IsThrowerPhase() && !boss->IsPhase4Triggered()) {

			bossPhase3SpawnTimer += dt;

			if (bossPhase3SpawnTimer >= bossPhase3SpawnInterval) {
				bossPhase3SpawnTimer = 0.0f;

				// limit number of throwers
				int throwerCount = 0;
				for (auto& enemy : Wave1) {
					Enemy* ePtr = enemy.get();

					if (dynamic_cast<Thrower*>(ePtr) && ePtr != bossPhase3Gun) {
						++throwerCount;
					}
				}

				if (throwerCount < bossPhase3MaxThrowers) {
					SpawnPhase3Thrower();
				}
			}
		}

		// =========================
		// BOSS PHASE 3 GUN
		// =========================
		if (boss && bossPhase3Gun) {
			bossPhase3Gun->SetPosition(boss->GetX(), boss->GetY());
		}

		// =========================
		// BOSS PHASE 2 TRIGGER
		// =========================
		if (boss && !bossPhase2Triggered && boss->GetGrowthHits() >= 5) {
			bossPhase2Triggered = true;
			bossPhase2Active = true;
			bossMinionSpawnTimer = bossMinionSpawnInterval;

			// lock UI to current displayed value, then animate upward
			bossPhaseHealActive = true;
		}

		// =========================
		// BOSS PHASE 2/3 HEAL
		// =========================
		if (boss) {
			float realHP = boss->GetCombatStats().health;
			float maxHP = boss->GetCombatStats().maxHealth;

			if (bossPhaseHealActive) {
				float healTarget = maxHP;
				bool phase3Healing = false;
				bool phase4Healing = false;

				if (boss->IsPhase3Triggered() &&
					(boss->IsPhase3Transitioning() || boss->IsPhase3Blinking() || boss->IsThrowerPhase())) {
					healTarget = maxHP * 0.8f;
					phase3Healing = true;
				}

				if (boss->IsPhase4Triggered() &&
					(boss->IsPhase4Transitioning() || boss->IsPhase4Blinking())) {
					healTarget = maxHP;
					phase4Healing = true;
				}

				s_bossLagHP += bossPhaseHealSpeed * dt;

				if (s_bossLagHP >= healTarget) {
					s_bossLagHP = healTarget;

					if (phase4Healing) {
						if (!boss->IsPhase4Transitioning() && !boss->IsPhase4Blinking()) {
							bossPhaseHealActive = false;
						}
					}
					else if (phase3Healing) {
						if (!boss->IsPhase3Transitioning() && !boss->IsPhase3Blinking()) {
							bossPhaseHealActive = false;
						}
					}
					else {
						bossPhaseHealActive = false;
					}
				}
			}
			else {
				if (s_bossLagHP > realHP) {
					s_bossLagHP -= 120.0f * dt;
					if (s_bossLagHP < realHP) {
						s_bossLagHP = realHP;
					}
				}
				else if (s_bossLagHP < realHP) {
					s_bossLagHP = realHP;
				}
			}
		}

		// =========================
		// PHASE 2 MINION SPAWNING
		// =========================
		// Initial
		if (bossPhase2Active && boss && !boss->IsPhaseBlinking() && !boss->IsPhaseTransitioning() && !bossInitialMinionsSpawned) {
			for (int i = 0; i < 3; ++i) {
				if (CountBossMinions() < bossMinionAliveLimit) {
					SpawnBossMinionNearBoss();
				}
			}

			bossInitialMinionsSpawned = true;
			bossMinionSpawnTimer = bossMinionSpawnInterval;
		}

		// Periodic
		if (bossPhase2Active && boss) {
			bossMinionSpawnTimer -= dt;

			if (bossMinionSpawnTimer <= 0.0f) {
				bossMinionSpawnTimer = bossMinionSpawnInterval;

				if (CountBossMinions() < bossMinionAliveLimit) {
					SpawnBossMinionNearBoss();
				}
			}
		}

		// =========================
		// PHASE 3 TRIGGER
		// =========================
		if (boss &&
			bossPhase2Triggered &&
			!boss->IsPhase3Triggered() &&
			boss->GetCombatStats().health <= boss->GetCombatStats().maxHealth * 0.2f) {

			boss->TriggerPhaseThree();

			// despawn every existing minion, keep only boss
			Wave1.erase(
				std::remove_if(Wave1.begin(), Wave1.end(),
					[](const std::unique_ptr<Enemy>& e) {
						return dynamic_cast<Boss*>(e.get()) == nullptr;
					}),
				Wave1.end()
			);

			// now spawn hidden helper thrower
			if (!bossPhase3Gun) {
				AEVec2 bossPos{ boss->GetX(), boss->GetY() };

				Wave1.push_back(std::make_unique<Thrower>(bossPos, ENEMY_SIZE, 99999.0f, 100.0f));
				bossPhase3Gun = static_cast<Thrower*>(Wave1.back().get());
				bossPhase3Gun->Init();
				bossPhase3Gun->SetMap(&gameMap);
				bossPhase3Gun->SetHideBody(true);
				bossPhase3Gun->SetProjectileStats(1000.0f, 20.0f, 30.0f);
			}

			bossPhase2Active = false;
			bossInitialMinionsSpawned = false;
			bossPhase3AddsSpawned = false;
			bossPhaseHealActive = true; // animate big bar to phase 3 heal target
		}


		if (!boss) {
			Wave1.clear(); // remove all minions instantly
			bossPhase3Gun = nullptr;
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

	if (bossPhase4DropActive) {
		Boss* boss = GetBoss();

		if (boss && boss->IsPhase4Triggered() && boss->IsPhase4BallVisible()) {
			AEVec2 playerPos{ player.GetX(), player.GetY() };
			AEVec2 ballPos = boss->GetPhase4BallPos();

			AEVec2 diff;
			AEVec2Sub(&diff, &playerPos, &ballPos);

			float interactRange = 100.0f + player.GetSize();

			// Do NOT freeze movement just for being near the drop
			if (AEVec2Length(&diff) <= interactRange) {
				if (AEInputCheckTriggered(AEVK_X)) {
					bossPhase4DropActive = false;
					boss->ConsumePhase4Pickup();
				}
			}
		}
		else {
			bossPhase4DropActive = false;
		}
	}

	if (0 == AESysDoesWindowExist()) {
		Transition_StartImmediate(GS_QUIT);
	}
	if (AEInputCheckTriggered(AEVK_K)) {
		//Wave1.clear();
	}

	if (AEInputCheckTriggered(AEVK_N)) {
		Transition_StartImmediate(GS_VICTORY);
	}

}

void Boss::PreparePhase4DashTarget(Player& player)
{
	AEVec2 toPlayer{ player.GetX() - m_pos.x, player.GetY() - m_pos.y };
	float len = AEVec2Length(&toPlayer);

	if (len > 0.001f) {
		AEVec2Scale(&m_Phase4LockedDashDir, &toPlayer, 1.0f / len);
	}
	else {
		m_Phase4LockedDashDir = { 1.0f, 0.0f };
	}

	// Dash past the player a little bit
	float overshoot = 180.0f;

	m_Phase4DashTarget.x = player.GetX() + m_Phase4LockedDashDir.x * overshoot;
	m_Phase4DashTarget.y = player.GetY() + m_Phase4LockedDashDir.y * overshoot;

	m_AimAngle = atan2(-m_Phase4LockedDashDir.y, -m_Phase4LockedDashDir.x);
	m_Phase4DashTravelled = 0.0f;
	m_Phase4CurrentDashSpeed = 0.0f;
	m_Phase4DashHitPlayer = false;
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

	HUD_Draw(&player, camera.GetX(), camera.GetY());
	DrawBossHealthBar(camera.GetX(), camera.GetY());
	Pause_Draw(camera.GetX(), camera.GetY());
	Debug_DrawHUD();
	AugmentEffects_Draw(camera.GetX(), camera.GetY());

	if (bossPhase4DropActive) {
		augments.Draw(camera.GetX(), camera.GetY());
	}
}

void BossLevel_Free() {
	if (Transition_GetState() != current)
		g_PlayerAttackCharges = player.GetAttackCharges();
	int nextState = Transition_GetState();
	if (nextState >= GS_TUTORIAL && nextState <= GS_BOSSLEVEL)
		SaveSystem_Save(nextState);
	Wave1.clear();
	bossPhase3Gun = nullptr;
	player.Free();
	Projectile::Free();
	augments.Free();
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