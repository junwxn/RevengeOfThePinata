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
}

void Sprite::Sprite_Init()
{
	// Free existing mesh before creating (prevents leak on re-init)
	if (pSpriteMesh) {
		AEGfxMeshFree(pSpriteMesh);
		pSpriteMesh = nullptr;
	}

	Sprite_Load();
	pSpriteMesh = CreateSpriteRectMesh(0xAEF359);
}

void Sprite::Sprite_Update(float dt)
{
	frameTimer += dt;

	if (frameTimer > frameSpeed)
	{
		frame++;
		frame %= 8;   // 8 frames
		frameTimer = 0;
	}
}