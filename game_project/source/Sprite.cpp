#include "pch.h"
#include "Sprite.h"
#include "Utils.h"

Sprite::~Sprite()
{
	// Walker
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

	// Thrower
	if (pThrowerSpriteMesh) {
		AEGfxMeshFree(pThrowerSpriteMesh);
		pThrowerSpriteMesh = nullptr;
	}
	if (pThrowerSpriteSheet) {
		AEGfxTextureUnload(pThrowerSpriteSheet);
		pThrowerSpriteSheet = nullptr;
	}
	if (pThrowerWindup_SpriteMesh) {
		AEGfxMeshFree(pThrowerWindup_SpriteMesh);
		pThrowerWindup_SpriteMesh = nullptr;
	}
	if (pThrowerWindup_SpriteSheet) {
		AEGfxTextureUnload(pThrowerWindup_SpriteSheet);
		pThrowerWindup_SpriteSheet = nullptr;
	}
	if (pThrowerAttack_SpriteMesh) {
		AEGfxMeshFree(pThrowerAttack_SpriteMesh);
		pThrowerAttack_SpriteMesh = nullptr;
	}
	if (pThrowerAttack_SpriteSheet) {
		AEGfxTextureUnload(pThrowerAttack_SpriteSheet);
		pThrowerAttack_SpriteSheet = nullptr;
	}

	// Boss
	if (pBossSpriteMesh) {
		AEGfxMeshFree(pBossSpriteMesh);
		pBossSpriteMesh = nullptr;
	}
	if (pBossSpriteSheet) {
		AEGfxTextureUnload(pBossSpriteSheet);
		pBossSpriteSheet = nullptr;
	}
	if (pBossWindup_SpriteMesh) {
		AEGfxMeshFree(pBossWindup_SpriteMesh);
		pBossWindup_SpriteMesh = nullptr;
	}
	if (pBossWindup_SpriteSheet) {
		AEGfxTextureUnload(pBossWindup_SpriteSheet);
		pBossWindup_SpriteSheet = nullptr;
	}
	if (pBossAttack_SpriteMesh) {
		AEGfxMeshFree(pBossAttack_SpriteMesh);
		pBossAttack_SpriteMesh = nullptr;
	}
	if (pBossAttack_SpriteSheet) {
		AEGfxTextureUnload(pBossAttack_SpriteSheet);
		pBossAttack_SpriteSheet = nullptr;
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
	// Walker
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

	pEnemyAttack_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Enemy_Attack_SpriteSheet2.png");
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

	pDasherAttack_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/BlackKid_Attack_SpriteSheet2.png");
	if (!pDasherAttack_SpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY ATTACK SPRITE SHEET" << std::endl;
		return;
	}

	// Thrower
	if (pThrowerSpriteSheet) {
		AEGfxTextureUnload(pThrowerSpriteSheet);
		pThrowerSpriteSheet = nullptr;
	}

	pThrowerSpriteSheet = AEGfxTextureLoad("Assets/Sprites/ChineseKid_SpriteSheet.png");
	if (!pThrowerSpriteSheet)
	{
		std::cout << "ERROR LOADING THROWER SPRITE SHEET" << std::endl;
		return;
	}

	if (pThrowerWindup_SpriteSheet) {
		AEGfxTextureUnload(pThrowerWindup_SpriteSheet);
		pThrowerWindup_SpriteSheet = nullptr;
	}

	pThrowerWindup_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/ChineseKid_Windup_SpriteSheet.png");
	if (!pThrowerWindup_SpriteSheet)
	{
		std::cout << "ERROR LOADING THROWER WINDUP SPRITE SHEET" << std::endl;
		return;
	}

	if (pThrowerAttack_SpriteSheet) {
		AEGfxTextureUnload(pThrowerAttack_SpriteSheet);
		pThrowerAttack_SpriteSheet = nullptr;
	}

	pThrowerAttack_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/ChineseKid_Attack_SpriteSheet2.png");
	if (!pThrowerAttack_SpriteSheet)
	{
		std::cout << "ERROR LOADING THROWER ATTACK SPRITE SHEET" << std::endl;
		return;
	}

	// Boss
	if (pBossSpriteSheet) {
		AEGfxTextureUnload(pBossSpriteSheet);
		pBossSpriteSheet = nullptr;
	}

	pBossSpriteSheet = AEGfxTextureLoad("Assets/Sprites/Boss_SpriteSheet.png");
	if (!pBossSpriteSheet)
	{
		std::cout << "ERROR LOADING BOSS SPRITE SHEET" << std::endl;
		return;
	}

	if (pBossWindup_SpriteSheet) {
		AEGfxTextureUnload(pBossWindup_SpriteSheet);
		pBossWindup_SpriteSheet = nullptr;
	}

	pBossWindup_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Boss_SpriteSheet.png");
	if (!pBossWindup_SpriteSheet)
	{
		std::cout << "ERROR LOADING BOSS WINDUP SPRITE SHEET" << std::endl;
		return;
	}

	if (pBossAttack_SpriteSheet) {
		AEGfxTextureUnload(pBossAttack_SpriteSheet);
		pBossAttack_SpriteSheet = nullptr;
	}

	pBossAttack_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Boss_Attack_SpriteSheet.png");
	if (!pBossAttack_SpriteSheet)
	{
		std::cout << "ERROR LOADING BOSS ATTACK SPRITE SHEET" << std::endl;
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

	// Thrower
	if (pThrowerSpriteMesh) {
		AEGfxMeshFree(pThrowerSpriteMesh);
		pThrowerSpriteMesh = nullptr;
	}
	if (pThrowerWindup_SpriteMesh) {
		AEGfxMeshFree(pThrowerWindup_SpriteMesh);
		pThrowerWindup_SpriteMesh = nullptr;
	}
	if (pThrowerAttack_SpriteMesh) {
		AEGfxMeshFree(pThrowerAttack_SpriteMesh);
		pThrowerAttack_SpriteMesh = nullptr;
	}

	// Boss
	if (pBossSpriteMesh) {
		AEGfxMeshFree(pBossSpriteMesh);
		pBossSpriteMesh = nullptr;
	}
	if (pBossWindup_SpriteMesh) {
		AEGfxMeshFree(pBossWindup_SpriteMesh);
		pBossWindup_SpriteMesh = nullptr;
	}
	if (pBossAttack_SpriteMesh) {
		AEGfxMeshFree(pBossAttack_SpriteMesh);
		pBossAttack_SpriteMesh = nullptr;
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

	// Thrower Spritesheet
	pThrowerSpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	pThrowerWindup_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	pThrowerAttack_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);

	// Boss Spritesheet
	pBossSpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	pBossWindup_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	pBossAttack_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);

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