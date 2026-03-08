#include "pch.h"
#include "Sprite.h"
#include "Utils.h"

void Sprite::Sprite_Load()
{
	//pEnemySpriteSheet = AEGfxTextureLoad("Assets/Sprites/Enemy_SpriteSheet.png");
	pEnemySpriteSheet = AEGfxTextureLoad("Assets/Sprites/Enemy_SpriteSheet2.png");
	if (!pEnemySpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY SPRITE SHEET" << std::endl;
		return;
	}
}

void Sprite::Sprite_Init()
{
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