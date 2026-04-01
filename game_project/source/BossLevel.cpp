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

// load data
static AEGfxVertexList* RectMesh = nullptr;
static s8 s_bossFont = -1;

// core objects
static Player player{};
static Camera camera{};
static Augments augments{};
static MapSystem gameMap{};

// combat / wave state
static Combat::System CombatSystem{};
static std::vector<std::unique_ptr<Enemy>> Wave1{};
static bool wave1Active{};
static bool bossDefeated{};
static bool preventingmovement{};
static float s_bossLagHP = 0.0f; // lags behind real HP

// phase 2 state
static bool bossPhase2Active = false;
static bool bossPhase2Triggered = false;
static float bossMinionSpawnTimer = 0.0f;
static float bossMinionSpawnInterval = 2.5f;
static int bossMinionAliveLimit = 10;
static bool bossPhaseHealActive = false;
static float bossPhaseHealSpeed = 120.0f;
static bool bossInitialMinionsSpawned = false;

// phase 3 state
static Thrower* bossPhase3Gun = nullptr;
static float bossPhase3SpawnTimer = 0.0f;
static float bossPhase3SpawnInterval = 1.5f;
static int bossPhase3MaxThrowers = 10;

// phase 4 state
static bool clearAddsForPhase4 = false;
static bool phase4CleanupDone = false;
static bool bossPhase4DropActive = false;
static bool bossPhase4FightActive = false;
static bool bossPhase4CanDie = false;

// helper functions
static Boss* GetBoss();
static int CountBossMinions();
static void SpawnBossMinionNearBoss();
static void SpawnPhase3Thrower();
static void SpawnBossWave();
static void DrawBossHealthBar(float camX, float camY);

static Boss* GetBoss()
{
	for (auto& enemy : Wave1) {
		if (Boss* boss = dynamic_cast<Boss*>(enemy.get())) {
			return boss;
		}
	}
	return nullptr;
}

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
	if (!boss) {
		return;
	}

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

	if (!found) {
		return;
	}

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

	float radius = 220.0f;
	float angle = AERandFloat() * 2.0f * PI;

	AEVec2 spawnPos{};
	spawnPos.x = playerPos.x + cosf(angle) * radius;
	spawnPos.y = playerPos.y + sinf(angle) * radius;

	// simple wall check
	for (int i = 0; i < 5; ++i) {
		if (!gameMap.IsPositionBlocked(spawnPos.x, spawnPos.y, ENEMY_SIZE)) {
			break;
		}

		angle = AERandFloat() * 2.0f * PI;
		spawnPos.x = playerPos.x + cosf(angle) * radius;
		spawnPos.y = playerPos.y + sinf(angle) * radius;
	}

	Wave1.push_back(std::make_unique<Thrower>(spawnPos, ENEMY_SIZE, 80.0f, 100.0f));

	Enemy* spawned = Wave1.back().get();
	spawned->Init();
	spawned->SetMap(&gameMap);
}

static void SpawnBossWave()
{
	Wave1.clear();
	AEVec2 playerPos{ player.GetX(), player.GetY() };

	// spawn boss on a valid tile
	AEVec2 bossPos = GetRandomSpawnPos(gameMap, playerPos, 200.0f, BOSS_SIZE);
	Wave1.push_back(std::make_unique<Boss>(bossPos, 20.0f, 1000.0f, 150.0f));

	for (auto& enemy : Wave1) {
		enemy->Init();
		enemy->SetMap(&gameMap);
	}

	Boss* boss = GetBoss();
	if (boss) {
		s_bossLagHP = boss->GetCombatStats().health;
	}
}

static void DrawBossHealthBar(float camX, float camY)
{
	Boss* boss = GetBoss();
	if (!boss) {
		return;
	}

	auto stats = boss->GetCombatStats();
	float hp = stats.health;
	float maxHp = stats.maxHealth;
	if (maxHp <= 0.0f) {
		return;
	}

	float displayHP = bossPhaseHealActive ? s_bossLagHP : hp;
	float ratio = AEClamp(displayHP / maxHp, 0.0f, 1.0f);
	float lagRatio = bossPhaseHealActive ? ratio : AEClamp(s_bossLagHP / maxHp, 0.0f, 1.0f);

	auto drawHUD = [&](float w, float h, float x, float y, float r, float g, float b, float a) {
		DrawMesh(RectMesh, w, h, x + camX, y + camY, 0.0f, r, g, b, a);
		};

	float barW = 800.0f;
	float barH = 40.0f;
	float barX = -barW * 0.5f;
	float barY = 400.0f;

	// border
	drawHUD(barW + 6, barH + 6, barX - 3, barY, 80, 80, 80, 200);

	// background
	drawHUD(barW, barH, barX, barY, 30, 30, 30, 220);

	float fillW = barW * ratio;
	float lagFillW = barW * lagRatio;

	// lag bar
	drawHUD(lagFillW, barH, barX, barY, 255, 140, 40, 220);

	// current hp bar
	drawHUD(fillW, barH, barX, barY, 200, 40, 40, 255);

	float tw = 0.0f;
	float th = 0.0f;
	AEGfxGetPrintSize(s_bossFont, "DA BOSS BABY", 0.8f, &tw, &th);

	float centerX = barX + barW * 0.5f;
	AEGfxPrint(
		s_bossFont,
		"DA BOSS BABY",
		centerX / 800.0f - tw * 0.5f,
		(barY - 60.0f) / 450.0f,
		0.8f,
		1.0f, 1.0f, 1.0f, 1.0f
	);
}

void BossLevel_Load()
{
	RectMesh = CreateRectMesh(0xFFFFFFFF);
	gameMap.Init("Assets/bossmap.tmx", "tilesheet_complete", "Assets/tilesheet_complete.png");
	gameMap.BuildCollisionGrid("Tile Layer 2");
	s_bossFont = AEGfxCreateFont("Assets/fonts/Stick-Regular.ttf", 48);

	Pause_Load();
	HUD_Load();
	Debug_Load();
	Shadow_Init();
}

void BossLevel_Init()
{
	player.Init();
	//player.SetAttackCharges(g_PlayerAttackCharges);
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
	bossPhase3SpawnTimer = 0.0f;

	clearAddsForPhase4 = false;
	phase4CleanupDone = false;
	bossPhase4DropActive = false;
	bossPhase4FightActive = false;
	bossPhase4CanDie = false;

	gAudio.PlayBGM(BGM_BOSS);

	Debug_Init();
	DebugContext dbgCtx = {};
	dbgCtx.player = &player;
	dbgCtx.camera = &camera;
	dbgCtx.map = &gameMap;
	dbgCtx.waves[0] = &Wave1;
	dbgCtx.waveCount = 1;
	dbgCtx.levelName = "Boss Level";
	Debug_Register(dbgCtx);
}

void BossLevel_Update(float dt)
{
	// window exit
	if (!AESysDoesWindowExist()) {
		SaveSystem_Save(GS_BOSSLEVEL);
		Transition_StartImmediate(GS_QUIT);
		return;
	}

	// pause menu
	if (Pause_Update(true)) {
		return;
	}

	// debug keys
	Debug_Update();

	if (AEInputCheckTriggered(AEVK_K)) {
		Boss* boss = GetBoss();

		if (boss) {
			// Do not allow debug skipping while the boss is already transitioning
			bool bossBusy =
				boss->IsPhaseTransitioning() ||
				boss->IsPhaseBlinking() ||
				boss->IsPhase3Transitioning() ||
				boss->IsPhase3Blinking() ||
				boss->IsPhase4Transitioning() ||
				boss->IsPhase4Blinking() ||
				boss->IsPhase4BallVisible();

			if (bossBusy) {
				return;
			}

			// Phase 1 -> Phase 2
			if (!bossPhase2Triggered) {
				boss->SetHealth(0.0f);
			}
			// Phase 2 -> Phase 3
			else if (!boss->IsPhase3Triggered()) {
				float maxHP = boss->GetCombatStats().maxHealth;
				boss->SetHealth(maxHP * 0.20f);
			}
			// Phase 3 -> Phase 4
			else if (!boss->IsPhase4Triggered()) {
				bossPhase4DropActive = false;
				bossPhase4FightActive = false;
				bossPhase4CanDie = false;
				augments.SetSpawnAnim(false);

				boss->SetHealth(0.0f);
			}
			// Phase 4 orb pickup shortcut
			else if (!bossPhase4FightActive) {
				bossPhase4DropActive = false;
				bossPhase4FightActive = true;
				bossPhase4CanDie = false;
				augments.SetSpawnAnim(false);

				bossDefeated = false;
				wave1Active = true;

				boss->ConsumePhase4Pickup();
			}
			// Final kill
			else {
				bossPhase4CanDie = true;
				boss->SetHealth(0.0f);
			}
		}
	}
	if (AEInputCheckTriggered(AEVK_N)) {
		bossDefeated = true;
	}

	// player death
	if (!player.GetIsAlive()) {
		Transition_StartImmediate(GS_GAMEOVER);
		return;
	}

	player.Update(dt, CombatSystem, Wave1, camera.GetX(), camera.GetY(), preventingmovement);

	if (wave1Active) {
		// generic cleanup for non-boss enemies
		Wave1.erase(
			std::remove_if(Wave1.begin(), Wave1.end(),
				[](const std::unique_ptr<Enemy>& e)
				{
					if (Boss* boss = dynamic_cast<Boss*>(e.get())) {
						// Let the boss class handle all pre-phase-4 death protection.
						// Do not erase the boss before its own phase logic runs.
						if (!boss->IsPhase4Triggered()) {
							return false;
						}

						// Fake death drop is visible: keep boss alive
						if (boss->IsPhase4BallVisible()) {
							return false;
						}

						// Final phase started after pickup, but boss has not properly revived yet
						if (bossPhase4FightActive && !bossPhase4CanDie) {
							return false;
						}

						// Real final death is allowed here
						return boss->GetCombatStats().health <= 0.0f;
					}

					return e->GetCombatStats().health <= 0.0f;
				}),
			Wave1.end()
		);

		// main boss / enemy update
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

		Boss* boss = GetBoss();


		// mark phase 4 cleanup when final phase is triggered
		if (boss && boss->IsPhase4Triggered() && !phase4CleanupDone) {
			clearAddsForPhase4 = true;
		}

		// spawn phase 4 drop once
		if (boss && boss->IsPhase4Triggered() && boss->IsPhase4BallVisible()) {
			if (!bossPhase4DropActive) {
				AEVec2 ballPos = boss->GetPhase4BallPos();
				augments.SetPosition(ballPos.x, ballPos.y);
				augments.SetSpawnAnim(true);
				bossPhase4DropActive = true;

				gAudio.PlayGeneralSFX(GENERAL_AUGMENT);
			}
		}

		// remove adds before final phase
		if (clearAddsForPhase4) {
			Wave1.erase(
				std::remove_if(Wave1.begin(), Wave1.end(),
					[](const std::unique_ptr<Enemy>& e) {
						return dynamic_cast<Boss*>(e.get()) == nullptr;
					}),
				Wave1.end()
			);

			bossPhase3Gun = nullptr;
			clearAddsForPhase4 = false;
			phase4CleanupDone = true;
		}

		boss = GetBoss();

		// once the boss has real hp again, final death is allowed later
		if (bossPhase4FightActive && boss && boss->GetCombatStats().health > 0.0f) {
			bossPhase4CanDie = true;
		}

		// phase 3 thrower spawning
		if (boss && boss->IsThrowerPhase() && !boss->IsPhase4Triggered()) {
			bossPhase3SpawnTimer += dt;

			if (bossPhase3SpawnTimer >= bossPhase3SpawnInterval) {
				bossPhase3SpawnTimer = 0.0f;

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

		// phase 3 hidden gun follows boss
		if (boss && bossPhase3Gun) {
			bossPhase3Gun->SetPosition(boss->GetX(), boss->GetY());
		}

		// phase 2 trigger
		if (boss && !bossPhase2Triggered) {
			float bossHP = boss->GetCombatStats().health;
			float bossMaxHP = boss->GetCombatStats().maxHealth;

			bool growthTrigger = (boss->GetGrowthHits() >= 5);
			bool hpTrigger = (bossHP <= bossMaxHP * 0.50f);

			// If player avoids the growth-hit mechanic, force boss into the 5-hit growth state first
			if (hpTrigger && !growthTrigger) {
				boss->ForceGrowthHits(5);
			}

			// Only start phase 2 after boss is already in the 5-hit grown state
			if (boss->GetGrowthHits() >= 5) {
				bossPhase2Triggered = true;
				bossPhase2Active = true;
				bossMinionSpawnTimer = bossMinionSpawnInterval;

				// lock UI to current displayed value, then animate upward
				bossPhaseHealActive = true;
			}
		}

		// boss hp lag / heal display
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

		// phase 2 minion spawning
		if (bossPhase2Active && boss && !boss->IsPhaseBlinking() && !boss->IsPhaseTransitioning() && !bossInitialMinionsSpawned) {
			for (int i = 0; i < 3; ++i) {
				if (CountBossMinions() < bossMinionAliveLimit) {
					SpawnBossMinionNearBoss();
				}
			}

			bossInitialMinionsSpawned = true;
			bossMinionSpawnTimer = bossMinionSpawnInterval;
		}

		if (bossPhase2Active && boss) {
			bossMinionSpawnTimer -= dt;

			if (bossMinionSpawnTimer <= 0.0f) {
				bossMinionSpawnTimer = bossMinionSpawnInterval;
				if (CountBossMinions() < bossMinionAliveLimit) {
					SpawnBossMinionNearBoss();
				}
			}
		}

		// phase 3 trigger
		if (boss &&
			bossPhase2Triggered &&
			!boss->IsPhase3Triggered() &&
			boss->GetCombatStats().health <= boss->GetCombatStats().maxHealth * 0.2f) {

			boss->TriggerPhaseThree();

			// keep boss only
			Wave1.erase(
				std::remove_if(Wave1.begin(), Wave1.end(),
					[](const std::unique_ptr<Enemy>& e) {
						return dynamic_cast<Boss*>(e.get()) == nullptr;
					}),
				Wave1.end()
			);

			// hidden helper thrower
			if (!bossPhase3Gun) {
				AEVec2 bossPos{ boss->GetX(), boss->GetY() };

				Wave1.push_back(std::make_unique<Thrower>(bossPos, ENEMY_SIZE, 99999.0f, 100.0f));
				bossPhase3Gun = static_cast<Thrower*>(Wave1.back().get());
				bossPhase3Gun->Init();
				bossPhase3Gun->SetMap(&gameMap);
				bossPhase3Gun->SetHideBody(true);
				bossPhase3Gun->SetProjectileStats(1200.0f, 20.0f, 150.0f, 10000.0f);
			}

			bossPhase2Active = false;
			bossInitialMinionsSpawned = false;
			bossPhaseHealActive = true;
		}

		// boss truly defeated
		if (!boss) {
			Wave1.clear();
			bossPhase3Gun = nullptr;
			bossPhase4FightActive = false;
			bossPhase4CanDie = false;
			wave1Active = false;
			bossDefeated = true;
		}
	}

	// boss defeated
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
	float MAP_MAX_X = static_cast<float>((std::max)(1u, gameMap.GetMapWidth())) + GRID_OFFSET;
	float MAP_MAX_Y = static_cast<float>((std::max)(1u, gameMap.GetMapHeight())) - 1.0f + GRID_OFFSET;

	bool clamped = false;
	if (gridX < MAP_MIN_X) {
		gridX = MAP_MIN_X;
		clamped = true;
	}
	if (gridX > MAP_MAX_X) {
		gridX = MAP_MAX_X;
		clamped = true;
	}
	if (gridY < MAP_MIN_Y) {
		gridY = MAP_MIN_Y;
		clamped = true;
	}
	if (gridY > MAP_MAX_Y) {
		gridY = MAP_MAX_Y;
		clamped = true;
	}

	if (clamped) {
		float newScreenX = (gridX - gridY) * halfW;
		float newScreenY = (gridX + gridY) * halfH;
		player.SetPosition(newScreenX, newScreenY);
	}

	camera.Update(dt, player.GetX(), player.GetY(), preventingmovement);

	// previous augments still active
	AugmentEffects_Update(dt, player, Wave1);

	// phase 4 orb update / pickup
	if (bossPhase4DropActive) {
		Boss* boss = GetBoss();

		if (boss && boss->IsPhase4Triggered() && boss->IsPhase4BallVisible()) {
			AEVec2 ballPos = boss->GetPhase4BallPos();

			augments.SetPosition(ballPos.x, ballPos.y);
			augments.Update(player.GetX(), player.GetY(), dt);

			preventingmovement = false;

			if (augments.GetChoose()) {
				bossPhase4DropActive = false;
				bossPhase4FightActive = true;
				bossPhase4CanDie = false;
				augments.SetSpawnAnim(false);

				bossDefeated = false;
				wave1Active = true;

				boss->ConsumePhase4Pickup();
			}
		}
		else {
			bossPhase4DropActive = false;
			augments.SetSpawnAnim(false);
		}
	}
}

void BossLevel_Draw()
{
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

			// hide boss body during fake death drop
			if (Boss* boss = dynamic_cast<Boss*>(ePtr)) {
				if (boss->IsPhase4Triggered() && boss->IsPhase4BallVisible()) {
					continue;
				}
			}

			renderQueue.push_back({ ePtr->GetY(), [ePtr]() { ePtr->Draw(); } });
		}
	}

	// map walls
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
	AugmentEffects_Draw(camera.GetX(), camera.GetY());

	if (bossPhase4DropActive) {
		augments.Draw(camera.GetX(), camera.GetY());
	}

	Debug_DrawHUD();
	Pause_Draw(camera.GetX(), camera.GetY());

}

void BossLevel_Free()
{
	if (Transition_GetState() != current) {
		g_PlayerAttackCharges = player.GetAttackCharges();
	}

	int nextState = Transition_GetState();
	if (nextState >= GS_TUTORIAL && nextState <= GS_BOSSLEVEL) {
		SaveSystem_Save(nextState);
	}

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

void BossLevel_Unload()
{
	Shadow_Free();
	AEGfxMeshFree(RectMesh);
	RectMesh = nullptr;
	gameMap.Unload();
	Pause_Unload();
	HUD_Unload();
	Debug_Unload();
	AEAudioStopGroup(gAudio.m_audioGroup.BGM);
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

	float overshoot = 180.0f;
	m_Phase4DashTarget.x = player.GetX() + m_Phase4LockedDashDir.x * overshoot;
	m_Phase4DashTarget.y = player.GetY() + m_Phase4LockedDashDir.y * overshoot;

	m_AimAngle = atan2(-m_Phase4LockedDashDir.y, -m_Phase4LockedDashDir.x);
	m_Phase4DashTravelled = 0.0f;
	m_Phase4CurrentDashSpeed = 0.0f;
	m_Phase4DashHitPlayer = false;

	gAudio.PlayGeneralSFX(GENERAL_BOSS_DASH);
}