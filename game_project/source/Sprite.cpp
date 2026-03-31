#include "pch.h"
#include "Sprite.h"
#include "Utils.h"

Sprite::~Sprite()
{
	// Enemy
	// Base
	if (m_pSpriteMesh) {
		AEGfxMeshFree(m_pSpriteMesh);
		m_pSpriteMesh = nullptr;
	}
	if (m_pEnemySpriteSheet) {
		AEGfxTextureUnload(m_pEnemySpriteSheet);
		m_pEnemySpriteSheet = nullptr;
	}
	if (m_pEnemyWindup_SpriteMesh) {
		AEGfxMeshFree(m_pEnemyWindup_SpriteMesh);
		m_pEnemyWindup_SpriteMesh = nullptr;
	}
	if (m_pEnemyWindup_SpriteSheet) {
		AEGfxTextureUnload(m_pEnemyWindup_SpriteSheet);
		m_pEnemyWindup_SpriteSheet = nullptr;
	}
	if (m_pEnemyAttack_SpriteMesh) {
		AEGfxMeshFree(m_pEnemyAttack_SpriteMesh);
		m_pEnemyAttack_SpriteMesh = nullptr;
	}
	if (m_pEnemyAttack_SpriteSheet) {
		AEGfxTextureUnload(m_pEnemyAttack_SpriteSheet);
		m_pEnemyAttack_SpriteSheet = nullptr;
	}

	// Dasher
	if (m_pDasherSpriteMesh) {
		AEGfxMeshFree(m_pDasherSpriteMesh);
		m_pDasherSpriteMesh = nullptr;
	}
	if (m_pDasherSpriteSheet) {
		AEGfxTextureUnload(m_pDasherSpriteSheet);
		m_pDasherSpriteSheet = nullptr;
	}
	if (m_pDasherWindup_SpriteMesh) {
		AEGfxMeshFree(m_pDasherWindup_SpriteMesh);
		m_pDasherWindup_SpriteMesh = nullptr;
	}
	if (m_pDasherWindup_SpriteSheet) {
		AEGfxTextureUnload(m_pDasherWindup_SpriteSheet);
		m_pDasherWindup_SpriteSheet = nullptr;
	}
	if (m_pDasherAttack_SpriteMesh) {
		AEGfxMeshFree(m_pDasherAttack_SpriteMesh);
		m_pDasherAttack_SpriteMesh = nullptr;
	}
	if (m_pDasherAttack_SpriteSheet) {
		AEGfxTextureUnload(m_pDasherAttack_SpriteSheet);
		m_pDasherAttack_SpriteSheet = nullptr;
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
	if (m_pBossSpriteMesh) {
		AEGfxMeshFree(m_pBossSpriteMesh);
		m_pBossSpriteMesh = nullptr;
	}
	if (m_pBossSpriteSheet) {
		AEGfxTextureUnload(m_pBossSpriteSheet);
		m_pBossSpriteSheet = nullptr;
	}
	if (m_pBossWindup_SpriteMesh) {
		AEGfxMeshFree(m_pBossWindup_SpriteMesh);
		m_pBossWindup_SpriteMesh = nullptr;
	}
	if (m_pBossWindup_SpriteSheet) {
		AEGfxTextureUnload(m_pBossWindup_SpriteSheet);
		m_pBossWindup_SpriteSheet = nullptr;
	}
	if (m_pBossAttack_SpriteMesh) {
		AEGfxMeshFree(m_pBossAttack_SpriteMesh);
		m_pBossAttack_SpriteMesh = nullptr;
	}
	if (m_pBossAttack_SpriteSheet) {
		AEGfxTextureUnload(m_pBossAttack_SpriteSheet);
		m_pBossAttack_SpriteSheet = nullptr;
	}

	// Player
	if (m_pPlayerSpriteMesh) {
		AEGfxMeshFree(m_pPlayerSpriteMesh);
		m_pPlayerSpriteMesh = nullptr;
	}
	if (m_pPlayerSpriteSheet) {
		AEGfxTextureUnload(m_pPlayerSpriteSheet);
		m_pPlayerSpriteSheet = nullptr;
	}

	if (m_pPlayerCombatSpriteMesh) {
		AEGfxMeshFree(m_pPlayerCombatSpriteMesh);
		m_pPlayerCombatSpriteMesh = nullptr;
	}
	if (m_pPlayerCombatSpriteSheet) {
		AEGfxTextureUnload(m_pPlayerCombatSpriteSheet);
		m_pPlayerCombatSpriteSheet = nullptr;
	}
}

void Sprite::Sprite_Load()
{
	// Free existing texture before loading (prevents leak on re-init)

	/////////////////////
	// ENEMY SPRITE SHEET
	/////////////////////
	// Walker
	if (m_pEnemySpriteSheet) {
		AEGfxTextureUnload(m_pEnemySpriteSheet);
		m_pEnemySpriteSheet = nullptr;
	}

	m_pEnemySpriteSheet = AEGfxTextureLoad("Assets/Sprites/Enemy_SpriteSheet.png");
	if (!m_pEnemySpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY SPRITE SHEET" << std::endl;
		return;
	}

	if (m_pEnemyWindup_SpriteSheet) {
		AEGfxTextureUnload(m_pEnemyWindup_SpriteSheet);
		m_pEnemyWindup_SpriteSheet = nullptr;
	}

	m_pEnemyWindup_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Enemy_Windup_SpriteSheet.png");
	if (!m_pEnemyWindup_SpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY WINDUP SPRITE SHEET" << std::endl;
		return;
	}

	if (m_pEnemyAttack_SpriteSheet) {
		AEGfxTextureUnload(m_pEnemyAttack_SpriteSheet);
		m_pEnemyAttack_SpriteSheet = nullptr;
	}

	m_pEnemyAttack_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Enemy_Attack_SpriteSheet2.png");
	if (!m_pEnemyAttack_SpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY ATTACK SPRITE SHEET" << std::endl;
		return;
	}

	// Dasher
	if (m_pDasherSpriteSheet) {
		AEGfxTextureUnload(m_pDasherSpriteSheet);
		m_pDasherSpriteSheet = nullptr;
	}

	m_pDasherSpriteSheet = AEGfxTextureLoad("Assets/Sprites/BlackKid_SpriteSheet.png");
	if (!m_pDasherSpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY SPRITE SHEET" << std::endl;
		return;
	}

	if (m_pDasherWindup_SpriteSheet) {
		AEGfxTextureUnload(m_pDasherWindup_SpriteSheet);
		m_pDasherWindup_SpriteSheet = nullptr;
	}

	m_pDasherWindup_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/BlackKid_Windup_SpriteSheet.png");
	if (!m_pDasherWindup_SpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY WINDUP SPRITE SHEET" << std::endl;
		return;
	}

	if (m_pDasherAttack_SpriteSheet) {
		AEGfxTextureUnload(m_pDasherAttack_SpriteSheet);
		m_pDasherAttack_SpriteSheet = nullptr;
	}

	m_pDasherAttack_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/BlackKid_Attack_SpriteSheet2.png");
	if (!m_pDasherAttack_SpriteSheet)
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
	if (m_pBossSpriteSheet) {
		AEGfxTextureUnload(m_pBossSpriteSheet);
		m_pBossSpriteSheet = nullptr;
	}

	m_pBossSpriteSheet = AEGfxTextureLoad("Assets/Sprites/Boss_SpriteSheet.png");
	if (!m_pBossSpriteSheet)
	{
		std::cout << "ERROR LOADING BOSS SPRITE SHEET" << std::endl;
		return;
	}

	if (m_pBossWindup_SpriteSheet) {
		AEGfxTextureUnload(m_pBossWindup_SpriteSheet);
		m_pBossWindup_SpriteSheet = nullptr;
	}

	m_pBossWindup_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Boss_SpriteSheet.png");
	if (!m_pBossWindup_SpriteSheet)
	{
		std::cout << "ERROR LOADING BOSS WINDUP SPRITE SHEET" << std::endl;
		return;
	}

	if (m_pBossAttack_SpriteSheet) {
		AEGfxTextureUnload(m_pBossAttack_SpriteSheet);
		m_pBossAttack_SpriteSheet = nullptr;
	}

	m_pBossAttack_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Boss_Attack_SpriteSheet.png");
	if (!m_pBossAttack_SpriteSheet)
	{
		std::cout << "ERROR LOADING BOSS ATTACK SPRITE SHEET" << std::endl;
		return;
	}

	//////////////////////
	// PLAYER SPRITE SHEET
	//////////////////////
	if (m_pPlayerSpriteSheet) {
		AEGfxTextureUnload(m_pPlayerSpriteSheet);
		m_pPlayerSpriteSheet = nullptr;
	}

	m_pPlayerSpriteSheet = AEGfxTextureLoad("Assets/Sprites/Pinata_Spritesheet.png");
	if (!m_pPlayerSpriteSheet)
	{
		std::cout << "ERROR LOADING PLAYER SPRITE SHEET" << std::endl;
		return;
	}

	/////////////////////////////
	// PLAYER COMBAT SPRITE SHEET
	/////////////////////////////
	if (m_pPlayerCombatSpriteSheet) {
		AEGfxTextureUnload(m_pPlayerCombatSpriteSheet);
		m_pPlayerCombatSpriteSheet = nullptr;
	}

	m_pPlayerCombatSpriteSheet = AEGfxTextureLoad("Assets/Sprites/Pinata_Bat_Spritesheet.png");
	if (!m_pPlayerCombatSpriteSheet)
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
	if (m_pSpriteMesh) {
		AEGfxMeshFree(m_pSpriteMesh);
		m_pSpriteMesh = nullptr;
	}
	if (m_pEnemyWindup_SpriteMesh) {
		AEGfxMeshFree(m_pEnemyWindup_SpriteMesh);
		m_pEnemyWindup_SpriteMesh = nullptr;
	}
	if (m_pEnemyAttack_SpriteMesh) {
		AEGfxMeshFree(m_pEnemyAttack_SpriteMesh);
		m_pEnemyAttack_SpriteMesh = nullptr;
	}

	// Dasher
	if (m_pDasherSpriteMesh) {
		AEGfxMeshFree(m_pDasherSpriteMesh);
		m_pDasherSpriteMesh = nullptr;
	}
	if (m_pDasherWindup_SpriteMesh) {
		AEGfxMeshFree(m_pDasherWindup_SpriteMesh);
		m_pDasherWindup_SpriteMesh = nullptr;
	}
	if (m_pDasherAttack_SpriteMesh) {
		AEGfxMeshFree(m_pDasherAttack_SpriteMesh);
		m_pDasherAttack_SpriteMesh = nullptr;
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
	if (m_pBossSpriteMesh) {
		AEGfxMeshFree(m_pBossSpriteMesh);
		m_pBossSpriteMesh = nullptr;
	}
	if (m_pBossWindup_SpriteMesh) {
		AEGfxMeshFree(m_pBossWindup_SpriteMesh);
		m_pBossWindup_SpriteMesh = nullptr;
	}
	if (m_pBossAttack_SpriteMesh) {
		AEGfxMeshFree(m_pBossAttack_SpriteMesh);
		m_pBossAttack_SpriteMesh = nullptr;
	}

	/////////////////////
	// Player Spritesheet
	/////////////////////
	if (m_pPlayerSpriteMesh) {
		AEGfxMeshFree(m_pPlayerSpriteMesh);
		m_pPlayerSpriteMesh = nullptr;
	}

	////////////////////////////
	// Player Combat Spritesheet
	////////////////////////////
	if (m_pPlayerCombatSpriteMesh) {
		AEGfxMeshFree(m_pPlayerCombatSpriteMesh);
		m_pPlayerCombatSpriteMesh = nullptr;
	}

	Sprite_Load();
	// Enemy Spritesheet
	m_pSpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	m_pEnemyWindup_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	m_pEnemyAttack_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);

	// Dasher Spritesheet
	m_pDasherSpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	m_pDasherWindup_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	m_pDasherAttack_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);

	// Thrower Spritesheet
	pThrowerSpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	pThrowerWindup_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	pThrowerAttack_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);

	// Boss Spritesheet
	m_pBossSpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	m_pBossWindup_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	m_pBossAttack_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);

	// Player Spritesheet
	m_pPlayerSpriteMesh = CreateSpriteRectMesh(0xAEF359, 10.0f, 8.0f);
	// Player Combat Spritesheet
	m_pPlayerCombatSpriteMesh = CreateSpriteRectMesh(0xAEF359, 10.0f, 8.0f);
}

void Sprite::Sprite_Update(float dt)
{
	m_frameTimer += dt;
	m_pFrameTimer += dt;

	if (m_frameTimer > m_frameSpeed)
	{
		m_frame++;
		m_frame %= 8;   // 8 frames
		m_frameTimer = 0;
	}

	if (m_pFrameTimer > m_pFrameSpeed)
	{
		m_pFrame++;
		m_pFrame %= 10;   // 10 frames
		m_pFrameTimer = 0;
	}
}