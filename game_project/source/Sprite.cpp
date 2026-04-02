/*************************************************************************
@file		Sprite.cpp
@Author		Chew Zheng Hui, Timothy Caleb z.chew@digipen.edu
@Co-authors	nil
@brief		This file contains the implementation of the sprite system
			containing functions and pointers to the various spritesheet
			textures frequently used by the main entities (Player/Enemy). 
			Sprite function initializers, loaders and updates are here

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#include "pch.h"
#include "Sprite.h"
#include "Utils.h"

// Static member definitions
int Sprite::s_refCount = 0;

AEGfxTexture* Sprite::s_pEnemySpriteSheet = nullptr;
AEGfxVertexList* Sprite::s_pSpriteMesh = nullptr;
AEGfxTexture* Sprite::s_pEnemyWindup_SpriteSheet = nullptr;
AEGfxVertexList* Sprite::s_pEnemyWindup_SpriteMesh = nullptr;
AEGfxTexture* Sprite::s_pEnemyAttack_SpriteSheet = nullptr;
AEGfxVertexList* Sprite::s_pEnemyAttack_SpriteMesh = nullptr;

AEGfxTexture* Sprite::s_pDasherSpriteSheet = nullptr;
AEGfxVertexList* Sprite::s_pDasherSpriteMesh = nullptr;
AEGfxTexture* Sprite::s_pDasherWindup_SpriteSheet = nullptr;
AEGfxVertexList* Sprite::s_pDasherWindup_SpriteMesh = nullptr;
AEGfxTexture* Sprite::s_pDasherAttack_SpriteSheet = nullptr;
AEGfxVertexList* Sprite::s_pDasherAttack_SpriteMesh = nullptr;

AEGfxTexture* Sprite::s_pThrowerSpriteSheet = nullptr;
AEGfxVertexList* Sprite::s_pThrowerSpriteMesh = nullptr;
AEGfxTexture* Sprite::s_pThrowerWindup_SpriteSheet = nullptr;
AEGfxVertexList* Sprite::s_pThrowerWindup_SpriteMesh = nullptr;
AEGfxTexture* Sprite::s_pThrowerAttack_SpriteSheet = nullptr;
AEGfxVertexList* Sprite::s_pThrowerAttack_SpriteMesh = nullptr;

AEGfxTexture* Sprite::s_pBossSpriteSheet = nullptr;
AEGfxVertexList* Sprite::s_pBossSpriteMesh = nullptr;
AEGfxTexture* Sprite::s_pBossWindup_SpriteSheet = nullptr;
AEGfxVertexList* Sprite::s_pBossWindup_SpriteMesh = nullptr;
AEGfxTexture* Sprite::s_pBossAttack_SpriteSheet = nullptr;
AEGfxVertexList* Sprite::s_pBossAttack_SpriteMesh = nullptr;

AEGfxTexture* Sprite::s_pPlayerSpriteSheet = nullptr;
AEGfxVertexList* Sprite::s_pPlayerSpriteMesh = nullptr;
AEGfxTexture* Sprite::s_pPlayerCombatSpriteSheet = nullptr;
AEGfxVertexList* Sprite::s_pPlayerCombatSpriteMesh = nullptr;

AEGfxTexture* Sprite::s_pPlusOneSpriteSheet = nullptr;
AEGfxVertexList* Sprite::s_pPlusOneSpriteMesh = nullptr;

AEGfxTexture* Sprite::s_pClearSpriteSheet = nullptr;
AEGfxVertexList* Sprite::s_pClearSpriteMesh = nullptr;

Sprite::~Sprite()
{
	if (!m_initialized) return;
	--s_refCount;
	if (s_refCount > 0) return;
	s_refCount = 0;

	// Last instance — free all shared assets
	// Enemy
	if (s_pSpriteMesh) { AEGfxMeshFree(s_pSpriteMesh); s_pSpriteMesh = nullptr; }
	if (s_pEnemySpriteSheet) { AEGfxTextureUnload(s_pEnemySpriteSheet); s_pEnemySpriteSheet = nullptr; }
	if (s_pEnemyWindup_SpriteMesh) { AEGfxMeshFree(s_pEnemyWindup_SpriteMesh); s_pEnemyWindup_SpriteMesh = nullptr; }
	if (s_pEnemyWindup_SpriteSheet) { AEGfxTextureUnload(s_pEnemyWindup_SpriteSheet); s_pEnemyWindup_SpriteSheet = nullptr; }
	if (s_pEnemyAttack_SpriteMesh) { AEGfxMeshFree(s_pEnemyAttack_SpriteMesh); s_pEnemyAttack_SpriteMesh = nullptr; }
	if (s_pEnemyAttack_SpriteSheet) { AEGfxTextureUnload(s_pEnemyAttack_SpriteSheet); s_pEnemyAttack_SpriteSheet = nullptr; }

	// Dasher
	if (s_pDasherSpriteMesh) { AEGfxMeshFree(s_pDasherSpriteMesh); s_pDasherSpriteMesh = nullptr; }
	if (s_pDasherSpriteSheet) { AEGfxTextureUnload(s_pDasherSpriteSheet); s_pDasherSpriteSheet = nullptr; }
	if (s_pDasherWindup_SpriteMesh) { AEGfxMeshFree(s_pDasherWindup_SpriteMesh); s_pDasherWindup_SpriteMesh = nullptr; }
	if (s_pDasherWindup_SpriteSheet) { AEGfxTextureUnload(s_pDasherWindup_SpriteSheet); s_pDasherWindup_SpriteSheet = nullptr; }
	if (s_pDasherAttack_SpriteMesh) { AEGfxMeshFree(s_pDasherAttack_SpriteMesh); s_pDasherAttack_SpriteMesh = nullptr; }
	if (s_pDasherAttack_SpriteSheet) { AEGfxTextureUnload(s_pDasherAttack_SpriteSheet); s_pDasherAttack_SpriteSheet = nullptr; }

	// Thrower
	if (s_pThrowerSpriteMesh) { AEGfxMeshFree(s_pThrowerSpriteMesh); s_pThrowerSpriteMesh = nullptr; }
	if (s_pThrowerSpriteSheet) { AEGfxTextureUnload(s_pThrowerSpriteSheet); s_pThrowerSpriteSheet = nullptr; }
	if (s_pThrowerWindup_SpriteMesh) { AEGfxMeshFree(s_pThrowerWindup_SpriteMesh); s_pThrowerWindup_SpriteMesh = nullptr; }
	if (s_pThrowerWindup_SpriteSheet) { AEGfxTextureUnload(s_pThrowerWindup_SpriteSheet); s_pThrowerWindup_SpriteSheet = nullptr; }
	if (s_pThrowerAttack_SpriteMesh) { AEGfxMeshFree(s_pThrowerAttack_SpriteMesh); s_pThrowerAttack_SpriteMesh = nullptr; }
	if (s_pThrowerAttack_SpriteSheet) { AEGfxTextureUnload(s_pThrowerAttack_SpriteSheet); s_pThrowerAttack_SpriteSheet = nullptr; }

	// Boss
	if (s_pBossSpriteMesh) { AEGfxMeshFree(s_pBossSpriteMesh); s_pBossSpriteMesh = nullptr; }
	if (s_pBossSpriteSheet) { AEGfxTextureUnload(s_pBossSpriteSheet); s_pBossSpriteSheet = nullptr; }
	if (s_pBossWindup_SpriteMesh) { AEGfxMeshFree(s_pBossWindup_SpriteMesh); s_pBossWindup_SpriteMesh = nullptr; }
	if (s_pBossWindup_SpriteSheet) { AEGfxTextureUnload(s_pBossWindup_SpriteSheet); s_pBossWindup_SpriteSheet = nullptr; }
	if (s_pBossAttack_SpriteMesh) { AEGfxMeshFree(s_pBossAttack_SpriteMesh); s_pBossAttack_SpriteMesh = nullptr; }
	if (s_pBossAttack_SpriteSheet) { AEGfxTextureUnload(s_pBossAttack_SpriteSheet); s_pBossAttack_SpriteSheet = nullptr; }

	// Player
	if (s_pPlayerSpriteMesh) { AEGfxMeshFree(s_pPlayerSpriteMesh); s_pPlayerSpriteMesh = nullptr; }
	if (s_pPlayerSpriteSheet) { AEGfxTextureUnload(s_pPlayerSpriteSheet); s_pPlayerSpriteSheet = nullptr; }
	if (s_pPlayerCombatSpriteMesh) { AEGfxMeshFree(s_pPlayerCombatSpriteMesh); s_pPlayerCombatSpriteMesh = nullptr; }
	if (s_pPlayerCombatSpriteSheet) { AEGfxTextureUnload(s_pPlayerCombatSpriteSheet); s_pPlayerCombatSpriteSheet = nullptr; }

	// Plus One
	if (s_pPlusOneSpriteMesh) { AEGfxMeshFree(s_pPlusOneSpriteMesh); s_pPlusOneSpriteMesh = nullptr; }
	if (s_pPlusOneSpriteSheet) { AEGfxTextureUnload(s_pPlusOneSpriteSheet); s_pPlusOneSpriteSheet = nullptr; }

	// Clear
	if (s_pClearSpriteMesh) { AEGfxMeshFree(s_pClearSpriteMesh); s_pClearSpriteMesh = nullptr; }
	if (s_pClearSpriteSheet) { AEGfxTextureUnload(s_pClearSpriteSheet); s_pClearSpriteSheet = nullptr; }
}

void Sprite::Sprite_Load()
{
	// Already loaded — skip
	if (s_pEnemySpriteSheet) return;

	/////////////////////
	// ENEMY SPRITE SHEET
	/////////////////////
	// Walker
	s_pEnemySpriteSheet = AEGfxTextureLoad("Assets/Sprites/Enemy_SpriteSheet.png");
	if (!s_pEnemySpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY SPRITE SHEET" << std::endl;
		return;
	}

	s_pEnemyWindup_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Enemy_Windup_SpriteSheet.png");
	if (!s_pEnemyWindup_SpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY WINDUP SPRITE SHEET" << std::endl;
		return;
	}

	s_pEnemyAttack_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Enemy_Attack_SpriteSheet2.png");
	if (!s_pEnemyAttack_SpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY ATTACK SPRITE SHEET" << std::endl;
		return;
	}

	// Dasher
	s_pDasherSpriteSheet = AEGfxTextureLoad("Assets/Sprites/BlackKid_SpriteSheet.png");
	if (!s_pDasherSpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY SPRITE SHEET" << std::endl;
		return;
	}

	s_pDasherWindup_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/BlackKid_Windup_SpriteSheet.png");
	if (!s_pDasherWindup_SpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY WINDUP SPRITE SHEET" << std::endl;
		return;
	}

	s_pDasherAttack_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/BlackKid_Attack_SpriteSheet2.png");
	if (!s_pDasherAttack_SpriteSheet)
	{
		std::cout << "ERROR LOADING ENEMY ATTACK SPRITE SHEET" << std::endl;
		return;
	}

	// Thrower
	s_pThrowerSpriteSheet = AEGfxTextureLoad("Assets/Sprites/ChineseKid_SpriteSheet.png");
	if (!s_pThrowerSpriteSheet)
	{
		std::cout << "ERROR LOADING THROWER SPRITE SHEET" << std::endl;
		return;
	}

	s_pThrowerWindup_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/ChineseKid_Windup_SpriteSheet.png");
	if (!s_pThrowerWindup_SpriteSheet)
	{
		std::cout << "ERROR LOADING THROWER WINDUP SPRITE SHEET" << std::endl;
		return;
	}

	s_pThrowerAttack_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/ChineseKid_Attack_SpriteSheet2.png");
	if (!s_pThrowerAttack_SpriteSheet)
	{
		std::cout << "ERROR LOADING THROWER ATTACK SPRITE SHEET" << std::endl;
		return;
	}

	// Boss
	s_pBossSpriteSheet = AEGfxTextureLoad("Assets/Sprites/Boss_SpriteSheet.png");
	if (!s_pBossSpriteSheet)
	{
		std::cout << "ERROR LOADING BOSS SPRITE SHEET" << std::endl;
		return;
	}

	s_pBossWindup_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Boss_SpriteSheet.png");
	if (!s_pBossWindup_SpriteSheet)
	{
		std::cout << "ERROR LOADING BOSS WINDUP SPRITE SHEET" << std::endl;
		return;
	}

	s_pBossAttack_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Boss_Attack_SpriteSheet.png");
	if (!s_pBossAttack_SpriteSheet)
	{
		std::cout << "ERROR LOADING BOSS ATTACK SPRITE SHEET" << std::endl;
		return;
	}

	// Plus One
	s_pPlusOneSpriteSheet = AEGfxTextureLoad("Assets/Sprites/PlusOne.png");
	if (!s_pPlusOneSpriteSheet)
	{
		std::cout << "ERROR LOADING PLUS ONE SPRITE SHEET" << std::endl;
		return;
	}

	//////////////////////
	// PLAYER SPRITE SHEET
	//////////////////////
	s_pPlayerSpriteSheet = AEGfxTextureLoad("Assets/Sprites/Pinata_Spritesheet.png");
	if (!s_pPlayerSpriteSheet)
	{
		std::cout << "ERROR LOADING PLAYER SPRITE SHEET" << std::endl;
		return;
	}

	/////////////////////////////
	// PLAYER COMBAT SPRITE SHEET
	/////////////////////////////
	s_pPlayerCombatSpriteSheet = AEGfxTextureLoad("Assets/Sprites/Pinata_Bat_Spritesheet.png");
	if (!s_pPlayerCombatSpriteSheet)
	{
		std::cout << "ERROR LOADING PLAYER COMBAT SPRITE SHEET" << std::endl;
		return;
	}

	// Clear
	s_pClearSpriteSheet = AEGfxTextureLoad("Assets/Sprites/Level_Clear_Spritesheet.png");
	if (!s_pClearSpriteSheet)
	{
		std::cout << "ERROR LOADING CLEAR SPRITE SHEET" << std::endl;
		return;
	}
}

void Sprite::Sprite_Init()
{
	// Prevent double-init on the same instance (e.g., static Player re-initialized each level)
	if (m_initialized) return;
	m_initialized = true;
	++s_refCount;

	// Assets already loaded by a previous instance — skip
	if (s_refCount > 1 && s_pEnemySpriteSheet) return;

	Sprite_Load();

	// Enemy Spritesheet
	s_pSpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	s_pEnemyWindup_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	s_pEnemyAttack_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);

	// Dasher Spritesheet
	s_pDasherSpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	s_pDasherWindup_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	s_pDasherAttack_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);

	// Thrower Spritesheet
	s_pThrowerSpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	s_pThrowerWindup_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	s_pThrowerAttack_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);

	// Boss Spritesheet
	s_pBossSpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	s_pBossWindup_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);
	s_pBossAttack_SpriteMesh = CreateSpriteRectMesh(0xAEF359, 8.0f, 7.0f);

	// Player Spritesheet
	s_pPlayerSpriteMesh = CreateSpriteRectMesh(0xAEF359, 10.0f, 8.0f);
	// Player Combat Spritesheet
	s_pPlayerCombatSpriteMesh = CreateSpriteRectMesh(0xAEF359, 10.0f, 8.0f);

	// Plus One Sprite
	s_pPlusOneSpriteMesh = CreateSpriteRectMesh(0xFFFFFFFF, 8.0f, 1.0f);

	// Clear Sprite
	s_pClearSpriteMesh = CreateSpriteRectMesh(0xFFFFFFFF, 8.0f, 1.0f);
}

void Sprite::Sprite_Update(float dt)
{
	m_frameTimer += dt;
	m_pFrameTimer += dt;
	m_plusOneFrameTimer += dt;

	if (m_frameTimer > m_frameSpeed)
	{
		m_frame++;
		m_frame %= 8;
		m_frameTimer = 0;
	}

	if (m_pFrameTimer > m_pFrameSpeed)
	{
		m_pFrame++;
		m_pFrame %= 10;
		m_pFrameTimer = 0;
	}

	if (m_plusOneFrameTimer > m_plusOneFrameSpeed)
	{
		if (m_plusOneFrame < 7)
		{
			++m_plusOneFrame;
			SetPlusOneTextureU();
		}

		m_plusOneFrameTimer = 0.0f;
	}

	// CLEAR animation
	if (m_clearActive)
	{
		m_clearTimer -= dt;
		m_clearFrameTimer += dt;

		if (m_clearFrameTimer >= m_clearFrameSpeed)
		{
			m_clearFrameTimer = 0.0f;
			++m_clearFrame;
			m_clearFrame %= 8; // loop through all 8 frames continuously
			SetClearTextureU();
		}

		if (m_clearTimer <= 0.0f)
		{
			StopClearAnimation();
		}
	}
}
