#include "pch.h"
#include "Sprite.h"
#include "Utils.h"

Sprite::~Sprite()
{
	// Enemy
	// Base
	if (pSpriteMesh) {
		AEGfxMeshFree(pSpriteMesh);
		pSpriteMesh = nullptr;
	}
	if (pEnemySpriteSheet) {
		AEGfxTextureUnload(pEnemySpriteSheet);
		pEnemySpriteSheet = nullptr;
	}
	if (pEnemyWindup_SpriteMesh) {
		AEGfxMeshFree(pEnemyWindup_SpriteMesh);
		pEnemyWindup_SpriteMesh = nullptr;
	}
	if (pEnemyWindup_SpriteSheet) {
		AEGfxTextureUnload(pEnemyWindup_SpriteSheet);
		pEnemyWindup_SpriteSheet = nullptr;
	}
	if (pEnemyAttack_SpriteMesh) {
		AEGfxMeshFree(pEnemyAttack_SpriteMesh);
		pEnemyAttack_SpriteMesh = nullptr;
	}
	if (pEnemyAttack_SpriteSheet) {
		AEGfxTextureUnload(pEnemyAttack_SpriteSheet);
		pEnemyAttack_SpriteSheet = nullptr;
	}

	// Dasher
	if (pDasherSpriteMesh) {
		AEGfxMeshFree(pDasherSpriteMesh);
		pDasherSpriteMesh = nullptr;
	}
	if (pDasherSpriteSheet) {
		AEGfxTextureUnload(pDasherSpriteSheet);
		pDasherSpriteSheet = nullptr;
	}
	if (pDasherWindup_SpriteMesh) {
		AEGfxMeshFree(pDasherWindup_SpriteMesh);
		pDasherWindup_SpriteMesh = nullptr;
	}
	if (pDasherWindup_SpriteSheet) {
		AEGfxTextureUnload(pDasherWindup_SpriteSheet);
		pDasherWindup_SpriteSheet = nullptr;
	}
	if (pDasherAttack_SpriteMesh) {
		AEGfxMeshFree(pDasherAttack_SpriteMesh);
		pDasherAttack_SpriteMesh = nullptr;
	}
	if (pDasherAttack_SpriteSheet) {
		AEGfxTextureUnload(pDasherAttack_SpriteSheet);
		pDasherAttack_SpriteSheet = nullptr;
	}

	// Player
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

	/////////////////////
	// ENEMY SPRITE SHEET
	/////////////////////
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

	if (pEnemyWindup_SpriteSheet) {
		AEGfxTextureUnload(pEnemyWindup_SpriteSheet);
		pEnemyWindup_SpriteSheet = nullptr;
	}

	pEnemyWindup_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Enemy_Windup_SpriteSheet.png");
	if (!pEnemyWindup_SpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY WINDUP SPRITE SHEET" << std::endl;
		return;
	}

	if (pEnemyAttack_SpriteSheet) {
		AEGfxTextureUnload(pEnemyAttack_SpriteSheet);
		pEnemyAttack_SpriteSheet = nullptr;
	}

	pEnemyAttack_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Enemy_Attack_SpriteSheet.png");
	if (!pEnemyAttack_SpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY ATTACK SPRITE SHEET" << std::endl;
		return;
	}

	// Dasher
	if (pDasherSpriteSheet) {
		AEGfxTextureUnload(pDasherSpriteSheet);
		pDasherSpriteSheet = nullptr;
	}

	pDasherSpriteSheet = AEGfxTextureLoad("Assets/Sprites/BlackKid_SpriteSheet.png");
	if (!pDasherSpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY SPRITE SHEET" << std::endl;
		return;
	}

	if (pDasherWindup_SpriteSheet) {
		AEGfxTextureUnload(pDasherWindup_SpriteSheet);
		pDasherWindup_SpriteSheet = nullptr;
	}

	pDasherWindup_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/BlackKid_Windup_SpriteSheet.png");
	if (!pDasherWindup_SpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY WINDUP SPRITE SHEET" << std::endl;
		return;
	}

	if (pDasherAttack_SpriteSheet) {
		AEGfxTextureUnload(pDasherAttack_SpriteSheet);
		pDasherAttack_SpriteSheet = nullptr;
	}

	pDasherAttack_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/BlackKid_Attack_SpriteSheet.png");
	if (!pDasherAttack_SpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY ATTACK SPRITE SHEET" << std::endl;
		return;
	}

	//////////////////////
	// PLAYER SPRITE SHEET
	//////////////////////
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

	/////////////////////////////
	// PLAYER COMBAT SPRITE SHEET
	/////////////////////////////
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
	////////////////////
	// Enemy Spritesheet
	////////////////////
	if (pSpriteMesh) {
		AEGfxMeshFree(pSpriteMesh);
		pSpriteMesh = nullptr;
	}
	if (pEnemyWindup_SpriteMesh) {
		AEGfxMeshFree(pEnemyWindup_SpriteMesh);
		pEnemyWindup_SpriteMesh = nullptr;
	}
	if (pEnemyAttack_SpriteMesh) {
		AEGfxMeshFree(pEnemyAttack_SpriteMesh);
		pEnemyAttack_SpriteMesh = nullptr;
	}

	// Dasher
	if (pDasherSpriteMesh) {
		AEGfxMeshFree(pDasherSpriteMesh);
		pDasherSpriteMesh = nullptr;
	}
	if (pDasherWindup_SpriteMesh) {
		AEGfxMeshFree(pDasherWindup_SpriteMesh);
		pDasherWindup_SpriteMesh = nullptr;
	}
	if (pDasherAttack_SpriteMesh) {
		AEGfxMeshFree(pDasherAttack_SpriteMesh);
		pDasherAttack_SpriteMesh = nullptr;
	}

	/////////////////////
	// Player Spritesheet
	/////////////////////
	if (pPlayerSpriteMesh) {
		AEGfxMeshFree(pPlayerSpriteMesh);
		pPlayerSpriteMesh = nullptr;
	}

	////////////////////////////
	// Player Combat Spritesheet
	////////////////////////////
	if (pPlayerCombatSpriteMesh) {
		AEGfxMeshFree(pPlayerCombatSpriteMesh);
		pPlayerCombatSpriteMesh = nullptr;
	}

	Sprite_Load();
	// Enemy Spritesheet
	pSpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	pEnemyWindup_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	pEnemyAttack_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);

	// Dasher Spritesheet
	pDasherSpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	pDasherWindup_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	pDasherAttack_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);

	// Player Spritesheet
	pPlayerSpriteMesh = CreateSpriteRectMesh(0xAEF359, 10.0f, 8.0f);
	// Player Combat Spritesheet
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
		pFrame %= 10;   // 10 frames
		pFrameTimer = 0;
	}
}