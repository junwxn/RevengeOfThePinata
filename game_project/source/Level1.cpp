#include "pch.h"
#include "Utils.h"
#include "Player.h"
#include "Level1.h"
#include "camera.h"
#include "Augments.h"
#include "GameStateManager.h"
#include "Map.h"
#include "Pause.h"

// load variables
static AEGfxTexture* TexBlock2;
static AEGfxTexture* TexBlock;
static AEGfxVertexList* CircleMesh;
static AEGfxVertexList* RectMesh;

// init variables
Player player{};
Circle HealCircle{};
Circle DMGCircle{};
RectData Healthbar{};
Camera camera{};
Augments augments{};
MapSystem gameMap;

// update variables
Combat::System CombatSystem;
std::vector<std::vector<std::unique_ptr<Enemy>>> Waves;
std::vector<std::unique_ptr<Enemy>> Wave1{};
std::vector<std::unique_ptr<Enemy>> Wave2{};
bool wave1Active{};
bool wave2Active{};
f32 Barcount{ 0 };
f32 MinibarWidth = 100;
u8 CurrentBars{ 0 };

// if wave ends
bool endofwave{};
bool preventingmovement{};

void Level1_Load() {
	TexBlock2	= AEGfxTextureLoad("Assets/block2.png");
	TexBlock	= AEGfxTextureLoad("Assets/block.png");
	CircleMesh	= CreateCircleMesh(1.0f, 32, 0xFFFFFFFF);
	RectMesh	= CreateRectMesh(0xFFFFFFFF);
	gameMap.Init("Assets/untitled.tmx", "tilesheet_complete", "Assets/tilesheet_complete.png");

	// Build the binary collision grid from the wall layer.
	// Change "Tile Layer 2" to whatever your collision/wall layer is named in Tiled.
	gameMap.BuildCollisionGrid("Tile Layer 2");

	Pause_Load();
}
void Level1_Init() {

	player.Init();
	player.SetMap(&gameMap); // Enable player↔map collision

	// logic objects
	HealCircle = { -400.0f, 0.0f, 150.0f };
	DMGCircle = { 400.0f, 0.0f, 150.0f };

	// healthbar Init
	Healthbar.w = 1200;
	Healthbar.h = 50;
	Healthbar.pos_x = -Healthbar.w / 2;
	Healthbar.pos_y = 350;
	Healthbar.max = Healthbar.pos_x + Healthbar.w;
	Healthbar.min = Healthbar.pos_x;
	Healthbar.var = 100;
	Healthbar.current = (Healthbar.var / 100) * (Healthbar.max - Healthbar.min);
	
	// camera init
	camera.Init(player.GetX(), player.GetY());

	Pause_Init();

	// Augments Init
	augments.Init();
}
void Level1_Update(float dt) {
	if (!AESysDoesWindowExist()) {
		next = GS_QUIT;
		return;
	}

	// Pause handles ESC toggle + menu input; returns true if paused
	if (Pause_Update()) return;

	player.Update(dt, CombatSystem, Wave1, camera.GetX(), camera.GetY(), preventingmovement);

	if (AEInputCheckTriggered(AEVK_1)) {
		if (wave1Active) {
			Wave1.clear();
		}
		else {
			SpawnWave1();
		}
		wave1Active = wave1Active ? false : true;
	}
	else if (AEInputCheckTriggered(AEVK_2)) {
		if (wave2Active) {
			Wave2.clear();
		}
		else {
			SpawnWave2();
		}
		wave2Active = wave2Active ? false : true;
	}

	//update enemy
	if (wave1Active) {
		if (Wave1.empty()) { wave1Active = false; };
		Wave1.erase(
			std::remove_if(Wave1.begin(), Wave1.end(),
				[](const std::unique_ptr<Enemy>& e)
				{
					return e->GetCombatStats().health  <= 0.0f;
				}),
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
	}

	if (wave2Active) {
		if (Wave2.empty()) { wave2Active = false; };

		Wave2.erase(
			std::remove_if(Wave2.begin(), Wave2.end(),
				[](const std::unique_ptr<Enemy>& e)
				{
					return e->GetCombatStats().health <= 0.0f;
				}),
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

	if (AreCirclesIntersecting(HealCircle.pos_x, HealCircle.pos_y, HealCircle.r, player.GetX(), player.GetY(), player.GetSize())) {
		Healthbar.var += 15 * dt;
	}
	if (AreCirclesIntersecting(DMGCircle.pos_x, DMGCircle.pos_y, DMGCircle.r, player.GetX(), player.GetY(), player.GetSize())) {
		Healthbar.var -= 15 * dt;
	}

	if (player.GetCombatFlag().attackHit) {
		Healthbar.var -= player.GetCombatStats().attack;
	}

	if (Healthbar.var < 0) Healthbar.var = 0;
	if (Healthbar.var > 100) Healthbar.var = 100;

	Healthbar.current = (Healthbar.var / 100) * (Healthbar.max - Healthbar.min);
	Barcount = Healthbar.current / (Healthbar.w / 10);
	CurrentBars = (Barcount >= 1) ? 1 : 0;

	// Augments, only activates if the wave ends
	if (AEInputCheckTriggered(AEVK_O)) {
		std::cout << "AUGMENTS TRIGGERED" << std::endl;
		endofwave = true;
	}

	if (endofwave) {
		augments.Update(player.GetX(), player.GetY(), dt, camera.GetX(), camera.GetY());
		if (augments.GetChoose()) {
			preventingmovement = true;
			//std::cout << "THIS WORKS NOW" << std::endl;
		}
		if (AEInputCheckTriggered(AEVK_P)) {
			std::cout << "AUGMENTS TRIGGERED AGAIN" << std::endl;
			endofwave = false;
		}
	}
	else {
		preventingmovement = false;
	}

	if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist()) {
		next = GS_QUIT;
	}
}
void Level1_Draw() {
	AESysFrameStart();
	AEGfxSetBackgroundColor(0.0f, 0.23f, 0.34f);
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	//draw heal/dmg circles
	DrawMesh(CircleMesh, HealCircle.r, HealCircle.r, HealCircle.pos_x, HealCircle.pos_y, 0, 0, 255, 0, 255);
	DrawMesh(CircleMesh, DMGCircle.r, DMGCircle.r, DMGCircle.pos_x, DMGCircle.pos_y, 0, 255, 0, 0, 255);

	//draw healthbar
	DrawMesh(RectMesh, Healthbar.w, Healthbar.h, Healthbar.pos_x, Healthbar.pos_y, 0, 255, 0, 0, 150);
	DrawMesh(RectMesh, Healthbar.current, Healthbar.h, Healthbar.pos_x, Healthbar.pos_y, 0, 255, 0, 0, 255);

	//draw minibar
	int tempBars = CurrentBars;
	while (tempBars <= Barcount && tempBars != 0) {
		float xPos = (tempBars == 1) ? Healthbar.min : Healthbar.min + (tempBars - 1) * ((Healthbar.w / 10) + (((Healthbar.w / 10.0f) - MinibarWidth) / 9.0f));
		DrawMesh(RectMesh, MinibarWidth, Healthbar.h, xPos, Healthbar.pos_y - 80, 0, 255, 0, 0, 255);
		tempBars++;
	}
	if (Healthbar.var != 0) {
		DrawMesh(RectMesh, MinibarWidth, Healthbar.h, Healthbar.min, Healthbar.pos_y - 80, 0, 255, 0, 0, 255);
	}

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

	// If end of wave spawn augment ball (AT THE VERY FRONT)
	if (endofwave) {
		augments.Draw();
	}

	Pause_Draw();

	AESysFrameEnd();
}
void Level1_Free() {
	Wave1.clear();
	Wave2.clear();
	player.Free();
}
void Level1_Unload() {
	if (TexBlock)  { AEGfxTextureUnload(TexBlock);  TexBlock  = nullptr; }
	if (TexBlock2) { AEGfxTextureUnload(TexBlock2); TexBlock2 = nullptr; }
	AEGfxMeshFree(CircleMesh);
	AEGfxMeshFree(RectMesh);
	gameMap.Unload();
	Pause_Unload();
}

void SpawnWave1() {
	Wave1.clear();
	Wave1.push_back(std::make_unique<Walker>(
		AEVec2{ 150.0f, 100.0f }, 40.0f, 100.0f, 200.0f)
	);
	Wave1.push_back(std::make_unique<Dasher>(
		AEVec2{ 150.0f, -100.0f }, 40.0f, 100.0f, 200.0f, 0.1f)
	);

	for (auto& enemy : Wave1) {
		enemy->Init();
		enemy->SetMap(&gameMap); // Enable enemy↔map collision
	}

};
void SpawnWave2() {
	Wave2.clear();

	f32 centerX = 0.0f;
	f32 centerY = 0.0f;
	f32 widthSpacing = 80.0f;
	f32 heightSpacing = 2.0f;
	for (int i{}; i < 15; ++i) {
		f32 theta = i * heightSpacing;
		f32 r = widthSpacing * theta;
		f32 posX = centerX + r * cosf(theta);
		f32 posY = centerY + r * sinf(theta);

		if (i < 5) continue;
		Wave2.push_back(std::make_unique<Walker>(
			AEVec2{ posX, posY }, 40.0f, 100.0f, 200.0f)
		);
	}

	for (auto& enemy : Wave2) {
		enemy->Init();
		enemy->SetMap(&gameMap); // Enable enemy↔map collision
	}

};