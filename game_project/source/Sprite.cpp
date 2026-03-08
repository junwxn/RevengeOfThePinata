#include "pch.h"
#include "Sprite.h"
#include "Utils.h"

Sprite::~Sprite()
{
	if (pSpriteMesh) {
		AEGfxMeshFree(pSpriteMesh);
		pSpriteMesh = nullptr;
	}
	if (pEnemySpriteSheet) {
		AEGfxTextureUnload(pEnemySpriteSheet);
		pEnemySpriteSheet = nullptr;
	}

	if (pPlayerSpriteMesh) {
		AEGfxMeshFree(pPlayerSpriteMesh);
		pPlayerSpriteMesh = nullptr;
	}
	if (pPlayerSpriteSheet) {
		AEGfxTextureUnload(pPlayerSpriteSheet);
		pPlayerSpriteSheet = nullptr;
	}
}

void Sprite::Sprite_Load()
{
	// Free existing texture before loading (prevents leak on re-init)
	if (pEnemySpriteSheet) {
		AEGfxTextureUnload(pEnemySpriteSheet);
		pEnemySpriteSheet = nullptr;
	}

	pEnemySpriteSheet = AEGfxTextureLoad("Assets/Sprites/Enemy_SpriteSheet2.png");
	if (!pEnemySpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY SPRITE SHEET" << std::endl;
		return;
	}

	if (pPlayerSpriteSheet) {
		AEGfxTextureUnload(pPlayerSpriteSheet);
		pPlayerSpriteSheet = nullptr;
	}

	pPlayerSpriteSheet = AEGfxTextureLoad("Assets/Sprites/Pinata_Spritesheet.png");
	if (!pPlayerSpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY SPRITE SHEET" << std::endl;
		return;
	}
}

void Sprite::Sprite_Init()
{
	// Free existing mesh before creating (prevents leak on re-init)
	if (pSpriteMesh) {
		AEGfxMeshFree(pSpriteMesh);
		pSpriteMesh = nullptr;
	}
	if (pPlayerSpriteMesh) {
		AEGfxMeshFree(pPlayerSpriteMesh);
		pPlayerSpriteMesh = nullptr;
	}

	Sprite_Load();
	pSpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	pPlayerSpriteMesh = CreateSpriteRectMesh(0xAEF359, 10.0f, 8.0f);
}

void Sprite::Sprite_Update(float dt)
{
	frameTimer += dt;
	pFrameTimer += dt;

	if (frameTimer > frameSpeed)
	{
		frame++;
		frame %= 8;   // 8 frames
		frameTimer = 0;
	}

	if (pFrameTimer > pFrameSpeed)
	{
		pFrame++;
		pFrame %= 10;   // 8 frames
		pFrameTimer = 0;
	}
}