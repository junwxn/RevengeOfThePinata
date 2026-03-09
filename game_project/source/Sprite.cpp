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

	if (pPlayerCombatSpriteMesh) {
		AEGfxMeshFree(pPlayerCombatSpriteMesh);
		pPlayerCombatSpriteMesh = nullptr;
	}
	if (pPlayerCombatSpriteSheet) {
		AEGfxTextureUnload(pPlayerCombatSpriteSheet);
		pPlayerCombatSpriteSheet = nullptr;
	}
}

void Sprite::Sprite_Load()
{
	// Free existing texture before loading (prevents leak on re-init)

	// ENEMY SPRITE SHEET
	if (pEnemySpriteSheet) {
		AEGfxTextureUnload(pEnemySpriteSheet);
		pEnemySpriteSheet = nullptr;
	}

	pEnemySpriteSheet = AEGfxTextureLoad("Assets/Sprites/Enemy_SpriteSheet.png");
	if (!pEnemySpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY SPRITE SHEET" << std::endl;
		return;
	}

	// PLAYER SPRITE SHEET
	if (pPlayerSpriteSheet) {
		AEGfxTextureUnload(pPlayerSpriteSheet);
		pPlayerSpriteSheet = nullptr;
	}

	pPlayerSpriteSheet = AEGfxTextureLoad("Assets/Sprites/Pinata_Spritesheet.png");
	if (!pPlayerSpriteSheet)
	{
		std::cout << "ERROR LOADING PLAYER SPRITE SHEET" << std::endl;
		return;
	}

	// PLAYER COMBAT SPRITE SHEET
	if (pPlayerCombatSpriteSheet) {
		AEGfxTextureUnload(pPlayerCombatSpriteSheet);
		pPlayerCombatSpriteSheet = nullptr;
	}

	pPlayerCombatSpriteSheet = AEGfxTextureLoad("Assets/Sprites/Pinata_Bat_Spritesheet.png");
	if (!pPlayerCombatSpriteSheet)
	{
		std::cout << "ERROR LOADING PLAYER COMBAT SPRITE SHEET" << std::endl;
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

	if (pPlayerCombatSpriteMesh) {
		AEGfxMeshFree(pPlayerCombatSpriteMesh);
		pPlayerCombatSpriteMesh = nullptr;
	}

	Sprite_Load();
	pSpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	pPlayerSpriteMesh = CreateSpriteRectMesh(0xAEF359, 10.0f, 8.0f);
	pPlayerCombatSpriteMesh = CreateSpriteRectMesh(0xAEF359, 10.0f, 8.0f);
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