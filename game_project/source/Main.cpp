#include <crtdbg.h> 
#include "AEEngine.h"
#include <cmath>

// Tile Basis Vectors (from your JS)
const float I_X = 1.0f;
const float I_Y = 1.0f;
const float J_X = -1.0f;
const float J_Y = 1.0f;

// 1. GRID MATH SIZE (Perfect 2:1 ratio)
// The diamond surface is exactly 256x128.
const float GRID_W = 111.0f;
const float GRID_H = 64.0f;

// 2. IMAGE SIZE (Actual Texture)
// The texture includes 10px of "dirt wall" at the bottom.
const float SPRITE_W = 111.0f;
const float SPRITE_H = 128.0f;

// Helper struct for coordinates
typedef struct {
	float x;
	float y;
} Vec2;

static bool AreCirclesIntersecting(float c1_x, float c1_y, float r1, float c2_x, float c2_y, float r2);
static void CreateCircleMesh(f32 radius, u8 parts, u32 color);
static void CreateRectMesh(u32 colour);
static void DrawCircle(f32 scale_x, f32 scale_y, f32 trans_x, f32 trans_y, f32 rot);
static void DrawRect(f32 height, f32 width, f32 pos_x, f32 pos_y, f32 rot);
static void SetColor(f32 r, f32 g, f32 b, f32 a);

Vec2 GridToScreen(int gridX, int gridY);

AEMtx33 scale = { 0 };
AEMtx33 rotate = { 0 };
AEMtx33 translate = { 0 };
AEMtx33 transform = { 0 };

static AEGfxVertexList* pCircleMesh = 0;
static AEGfxVertexList* pRectMesh = 0;

typedef struct player {
	f32 pos_x, pos_y;
	f32 speed;
	f32 size;
} player;

typedef struct circle {
	f32 pos_x, pos_y;
	f32 r;
}circle;

typedef struct rect {
	f32 pos_x, pos_y, w, h, max, min, current, var;
}rect;

f32 barcount;
f32 minibar_width = 100;
u8 current_bars{ 0 };


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	int gGameRunning = 1;

	// Using custom window procedure
	AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, true, NULL);

	// Changing the window title
	AESysSetWindowTitle("Solo Project!!!");

	//create mesh
	CreateCircleMesh(1.0f, 32, 0xFFFFFFFF);
	CreateRectMesh(0xFFFFFFFF);

	player player{};
	player.pos_x = 0;
	player.pos_y = 0;
	player.speed = 200;
	player.size = 40.0f;

	circle heal_circle{};
	heal_circle.pos_x = -400;
	heal_circle.pos_y = 0;
	heal_circle.r = 150;

	circle dmg_circle{};
	dmg_circle.pos_x = 400;
	dmg_circle.pos_y = 0;
	dmg_circle.r = 150;

	rect healthbar{};
	healthbar.w = 1200;
	healthbar.h = 50;
	healthbar.pos_x = -healthbar.w / 2;
	healthbar.pos_y = 350;
	healthbar.max = healthbar.pos_x + healthbar.w;
	healthbar.min = healthbar.pos_x;
	healthbar.var = 100;
	healthbar.current = (healthbar.var / 100) * (healthbar.max - healthbar.min);

	AEGfxTexture* pTex = AEGfxTextureLoad("Assets/block.png");

	// Game Loop
	while (gGameRunning)
	{
		//start of frame
		AESysFrameStart();
		//background colour white
		AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

		AEGfxSetRenderMode(AE_GFX_RM_COLOR);

		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetTransparency(1.0f);

		f32 dt = (f32)AEFrameRateControllerGetFrameTime();

		

		//check player within window boundary
		if (player.pos_x - player.size < -AEGfxGetWindowWidth() / 2.0f) {
			player.pos_x = -AEGfxGetWindowWidth() / 2.0f + player.size;
		}
		if (player.pos_x + player.size > AEGfxGetWindowWidth() / 2.0f) {
			player.pos_x = AEGfxGetWindowWidth() / 2.0f - player.size;
		}
		if (player.pos_y - player.size < -AEGfxGetWindowHeight() / 2.0f) {
			player.pos_y = -AEGfxGetWindowHeight() / 2.0f + player.size;
		}
		if (player.pos_y + player.size > AEGfxGetWindowHeight() / 2.0f) {
			player.pos_y = AEGfxGetWindowHeight() / 2.0f - player.size;
		}

		if (AEInputCheckCurr(AEVK_W)) { // input: W
			player.pos_y += player.speed * dt;
		}
		if (AEInputCheckCurr(AEVK_A)) { // input: A
			player.pos_x -= player.speed * dt;
		}
		if (AEInputCheckCurr(AEVK_S)) { // input: S
			player.pos_y -= player.speed * dt;
		}
		if (AEInputCheckCurr(AEVK_D)) { // input: D
			player.pos_x += player.speed * dt;
		}

		//check if player in healing circle
		if (AreCirclesIntersecting(heal_circle.pos_x, heal_circle.pos_y, heal_circle.r, player.pos_x, player.pos_y, player.size)) {
			healthbar.var += 15 * dt;
		}

		//check if player in dmg circle
		if (AreCirclesIntersecting(dmg_circle.pos_x, dmg_circle.pos_y, dmg_circle.r, player.pos_x, player.pos_y, player.size)) {
			healthbar.var -= 15 * dt;
		}
		//make sure healthbar.var doesnt go into the negatives
		if (healthbar.var < 0) {
			healthbar.var = 0;
		}
		//make sure healtbar.var doesnt go above 100
		if (healthbar.var > 100) {
			healthbar.var = 100;
		}
		//current width of healthbar based on health
		healthbar.current = (healthbar.var / 100) * (healthbar.max - healthbar.min);
		barcount = healthbar.current / (healthbar.w / 10);

		if (barcount >= 1) {
			current_bars = 1;
		}
		else current_bars = 0;

		//draw healing circle
		SetColor(0, 255, 0, 255);
		DrawCircle(heal_circle.r, heal_circle.r, heal_circle.pos_x, heal_circle.pos_y, 0);

		//draw dmg circle
		SetColor(255, 0, 0, 255);
		DrawCircle(dmg_circle.r, dmg_circle.r, dmg_circle.pos_x, dmg_circle.pos_y, 0);

		

		//draw healtbar background
		SetColor(255, 0, 0, 150);
		DrawRect(healthbar.h, healthbar.w, healthbar.pos_x, healthbar.pos_y, 0);

		//draw actual healthbar
		SetColor(255, 0, 0, 255);
		DrawRect(healthbar.h, healthbar.current, healthbar.pos_x, healthbar.pos_y, 0);

		//draw minibar
		while (current_bars <= barcount && current_bars != 0) {
			if (current_bars == 1) {
				SetColor(255, 0, 0, 255);
				DrawRect(healthbar.h, minibar_width, healthbar.min, healthbar.pos_y - 80, 0);
				current_bars++;
			}
			else if (current_bars > 1) {
				SetColor(255, 0, 0, 255);
				DrawRect(healthbar.h, minibar_width, healthbar.min + (current_bars - 1) * ((healthbar.w / 10) + (((healthbar.w / 10.0f) - minibar_width) / 9.0f)), healthbar.pos_y - 80, 0);
				current_bars++;
			}
		}
		if (healthbar.var != 0) {
			DrawRect(healthbar.h, minibar_width, healthbar.min, healthbar.pos_y - 80, 0);
		}

		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
		AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetTransparency(1.0f);
		AEGfxTextureSet(pTex, 0, 0);
		// Example: Drawing a 10x10 map
		for (int x = 5; x > 0; --x) {
			for (int y = 6; y > 0; --y) {

				// 1. Calculate Position
				Vec2 pos = GridToScreen(x, y);

				// 2. Setup Matrices
				AEMtx33 scale, trans, transform;

				// Scale the mesh to the sprite size (32x32)
				AEMtx33Scale(&scale, SPRITE_W, SPRITE_H);

				// Move it to the calculated isometric position
				AEMtx33Trans(&trans, pos.x, pos.y);

				// 3. Combine (Order: Scale first, then Translate)
				// Mathematically: Trans * Scale * Vertex
				AEMtx33Concat(&transform, &trans, &scale);

				// 4. Send to Graphics Card
				AEGfxSetTransform(transform.m);
				AEGfxMeshDraw(pRectMesh, AE_GFX_MDM_TRIANGLES);
			}
		}

		Vec2 pos = GridToScreen(2, 4);

		// 2. Setup Matrices
		AEMtx33 scale, trans, transform;

		// Scale the mesh to the sprite size (32x32)
		AEMtx33Scale(&scale, SPRITE_W, SPRITE_H);

		// Move it to the calculated isometric position
		AEMtx33Trans(&trans, pos.x, pos.y);

		// 3. Combine (Order: Scale first, then Translate)
		// Mathematically: Trans * Scale * Vertex
		AEMtx33Concat(&transform, &trans, &scale);

		// 4. Send to Graphics Card
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(pRectMesh, AE_GFX_MDM_TRIANGLES);

		//draw player
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		SetColor(0, 0, 255, 255);
		DrawCircle(player.size, player.size, player.pos_x, player.pos_y, 0);
		// Informing the system about the loop's end
		AESysFrameEnd();

		// check if forcing the application to quit
		if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
			gGameRunning = 0;
	}
	AEGfxMeshFree(pCircleMesh);
	AEGfxMeshFree(pRectMesh);
	// free the system
	AEGfxTextureUnload(pTex);
	AESysExit();
}

static bool AreCirclesIntersecting(float c1_x, float c1_y, float r1, float c2_x, float c2_y, float r2)
{

	if (sqrt(pow((c1_x - c2_x), 2) + pow((c1_y - c2_y), 2)) < (r1 + r2)) {
		return true;
	}
	return false;
}

static void CreateCircleMesh(f32 radius, u8 parts, u32 color)
{
	AEGfxMeshStart();

	f32 angleStep = (2.0f * PI) / parts;
	f32 currentAngle = 0.0f;

	for (int i = 0; i < parts; ++i)
	{
		f32 x1 = radius * cosf(currentAngle);
		f32 y1 = radius * sinf(currentAngle);

		f32 x2 = radius * cosf(currentAngle + angleStep);
		f32 y2 = radius * sinf(currentAngle + angleStep);

		AEGfxTriAdd(
			0.0f, 0.0f, color, 0.0f, 0.0f,        // Center
			x1, y1, color, 0.0f, 0.0f,                 // Point 1
			x2, y2, color, 0.0f, 0.0f                  // Point 2
		);
		currentAngle += angleStep;
	}
	pCircleMesh = AEGfxMeshEnd();
}

static void CreateRectMesh(u32 colour) // left anchored
{
	AEGfxMeshStart();

	AEGfxTriAdd(
		0.0f, 0.5f, colour, 0.0f, 0.0f, // Top-Left (Now at X=0)
		0.0f, -0.5f, colour, 0.0f, 1.0f, // Bottom-Left (Now at X=0)
		1.0f, -0.5f, colour, 1.0f, 1.0f  // Bottom-Right (At X=1)
	);

	AEGfxTriAdd(
		0.0f, 0.5f, colour, 0.0f, 0.0f, // Top-Left
		1.0f, -0.5f, colour, 1.0f, 1.0f, // Bottom-Right
		1.0f, 0.5f, colour, 1.0f, 0.0f  // Top-Right (At X=1)
	);

	pRectMesh = AEGfxMeshEnd();
}

static void DrawCircle(f32 scale_x, f32 scale_y, f32 trans_x, f32 trans_y, f32 rot) {
	//transform 
	AEMtx33Scale(&scale, scale_x, scale_y);
	AEMtx33Rot(&rotate, rot);
	AEMtx33Trans(&translate, trans_x, trans_y);
	AEMtx33Concat(&transform, &rotate, &scale);
	AEMtx33Concat(&transform, &translate, &transform);
	AEGfxSetTransform(transform.m);
	//draw mesh 
	AEGfxMeshDraw(pCircleMesh, AE_GFX_MDM_TRIANGLES);
}

static void DrawRect(f32 height, f32 width, f32 pos_x, f32 pos_y, f32 rot) {
	//transform
	AEMtx33Scale(&scale, width, height);
	AEMtx33Rot(&rotate, rot);
	AEMtx33Trans(&translate, pos_x, pos_y);
	AEMtx33Concat(&transform, &rotate, &scale);
	AEMtx33Concat(&transform, &translate, &transform);
	AEGfxSetTransform(transform.m);
	//draw mesh
	AEGfxMeshDraw(pRectMesh, AE_GFX_MDM_TRIANGLES);
}

static void SetColor(f32 r, f32 g, f32 b, f32 a)
{
	AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetColorToAdd(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}


Vec2 GridToScreen(int gridX, int gridY) {
	Vec2 result;

	// Use the GRID dimensions (128), NOT the sprite dimensions (138)
	float halfW = GRID_W * 0.5f;
	float halfH = GRID_H * 0.5f;

	// Standard Isometric logic
	result.x = (gridX * I_X * halfW) + (gridY * J_X * halfW);
	result.y = (gridX * I_Y * halfH) + (gridY * J_Y * halfH);

	return result;
}