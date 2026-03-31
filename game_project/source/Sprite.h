#pragma once
#include "pch.h"
#include <fstream>
#include <sstream>

class Sprite
{
	public:
		~Sprite();
		void Sprite_Load();
		void Sprite_Init();
		void Sprite_Update(float dt);

		// Enemy Spritesheet
		// Base 1
		AEGfxTexture* GetSpriteSheet() { return pEnemySpriteSheet; }
		AEGfxTexture* GetEnemyWindupSpriteSheet() { return pEnemyWindup_SpriteSheet; }
		AEGfxTexture* GetEnemyAttackSpriteSheet() { return pEnemyAttack_SpriteSheet; }
		AEGfxVertexList* GetSpriteMesh() { return pSpriteMesh; }
		AEGfxVertexList* GetEnemyWindupSpriteMesh() { return pEnemyWindup_SpriteMesh; }
		AEGfxVertexList* GetEnemyAttackSpriteMesh() { return pEnemyAttack_SpriteMesh; }

		// Dasher
		AEGfxTexture* GetDasherSpriteSheet() { return pDasherSpriteSheet; }
		AEGfxTexture* GetDasherWindupSpriteSheet() { return pDasherWindup_SpriteSheet; }
		AEGfxTexture* GetDasherAttackSpriteSheet() { return pDasherAttack_SpriteSheet; }
		AEGfxVertexList* GetDasherSpriteMesh() { return pDasherSpriteMesh; }
		AEGfxVertexList* GetDasherWindupSpriteMesh() { return pDasherWindup_SpriteMesh; }
		AEGfxVertexList* GetDasherAttackSpriteMesh() { return pDasherAttack_SpriteMesh; }

		// Thrower
		AEGfxTexture* GetThrowerSpriteSheet() { return pThrowerSpriteSheet; }
		AEGfxTexture* GetThrowerWindupSpriteSheet() { return pThrowerWindup_SpriteSheet; }
		AEGfxTexture* GetThrowerAttackSpriteSheet() { return pThrowerAttack_SpriteSheet; }
		AEGfxVertexList* GetThrowerSpriteMesh() { return pThrowerSpriteMesh; }
		AEGfxVertexList* GetThrowerWindupSpriteMesh() { return pThrowerWindup_SpriteMesh; }
		AEGfxVertexList* GetThrowerAttackSpriteMesh() { return pThrowerAttack_SpriteMesh; }

		// Boss
		AEGfxTexture* GetBossSpriteSheet() { return pBossSpriteSheet; }
		AEGfxTexture* GetBossWindupSpriteSheet() { return pBossWindup_SpriteSheet; }
		AEGfxTexture* GetBossAttackSpriteSheet() { return pBossAttack_SpriteSheet; }

		AEGfxVertexList* GetBossSpriteMesh() { return pBossSpriteMesh; }
		AEGfxVertexList* GetBossWindupSpriteMesh() { return pBossWindup_SpriteMesh; }
		AEGfxVertexList* GetBossAttackSpriteMesh() { return pBossAttack_SpriteMesh; }

		// Player
		AEGfxTexture* GetPlayerSpriteSheet() { return pPlayerSpriteSheet; }
		AEGfxVertexList* GetPlayerSpriteMesh() { return pPlayerSpriteMesh; }

		AEGfxTexture* GetPlayerCombatSpriteSheet() { return pPlayerCombatSpriteSheet; }
		AEGfxVertexList* GetPlayerCombatSpriteMesh() { return pPlayerCombatSpriteMesh; }
		float GetPixelScale() const { return pixelScale; }

		float GetU() const { return u0; }
		float GetV() const { return v0; }

		float GetPlayerU() const { return p_u0; }
		float GetPlayerV() const { return p_v0; }

		int GetFrame() const { return frame; }
		int GetRow() const { return row; }
		int GetFrameWidth() const { return frameUSize; }
		int GetFrameHeight() const { return frameVSize; }

		void SetTextureU() { u0 = frame * frameUSize; }
		void SetTextureV(int animRow) { row = animRow; v0 = row * frameVSize; }

		void SetTexturePlayerU() { p_u0 = pFrame * pFrameUSize; }
		void SetTexturePlayerV(int animRow) { pRow = animRow; p_v0 = pRow * pFrameVSize; }

		void SetSingleFrameTexture() {
			frame = 0;
			row = 0;
			frameUSize = 1.0f;
			frameVSize = 1.0f;
			SetTextureU();
			SetTextureV(0);
		}

		void SetEnemyAttackSingleFrame(int column, int rowIndex)
		{
			frame = column;
			row = rowIndex;
			frameUSize = 1.0f / 8.0f;
			frameVSize = 1.0f / 7.0f;
			SetTextureU();
			SetTextureV(rowIndex);
		}

		void SetDasherAttackSingleFrame(int column, int rowIndex)
		{
			frame = column;
			row = rowIndex;
			frameUSize = 1.0f / 8.0f;
			frameVSize = 1.0f / 7.0f;
			SetTextureU();
			SetTextureV(rowIndex);
		}

	private:
		// Enemy sprite sheet
		// Walker
		AEGfxTexture* pEnemySpriteSheet{nullptr};
		AEGfxVertexList* pSpriteMesh{nullptr};

		AEGfxTexture* pEnemyWindup_SpriteSheet{ nullptr };
		AEGfxVertexList* pEnemyWindup_SpriteMesh{ nullptr };

		AEGfxTexture* pEnemyAttack_SpriteSheet{ nullptr };
		AEGfxVertexList* pEnemyAttack_SpriteMesh{ nullptr };

		// Dasher
		AEGfxTexture* pDasherSpriteSheet{ nullptr };
		AEGfxVertexList* pDasherSpriteMesh{ nullptr };

		AEGfxTexture* pDasherWindup_SpriteSheet{ nullptr };
		AEGfxVertexList* pDasherWindup_SpriteMesh{ nullptr };

		AEGfxTexture* pDasherAttack_SpriteSheet{ nullptr };
		AEGfxVertexList* pDasherAttack_SpriteMesh{ nullptr };

		// Thrower
		AEGfxTexture* pThrowerSpriteSheet{ nullptr };
		AEGfxVertexList* pThrowerSpriteMesh{ nullptr };

		AEGfxTexture* pThrowerWindup_SpriteSheet{ nullptr };
		AEGfxVertexList* pThrowerWindup_SpriteMesh{ nullptr };

		AEGfxTexture* pThrowerAttack_SpriteSheet{ nullptr };
		AEGfxVertexList* pThrowerAttack_SpriteMesh{ nullptr };

		// Boss
		AEGfxTexture* pBossSpriteSheet{ nullptr };
		AEGfxVertexList* pBossSpriteMesh{ nullptr };

		AEGfxTexture* pBossWindup_SpriteSheet{ nullptr };
		AEGfxVertexList* pBossWindup_SpriteMesh{ nullptr };

		AEGfxTexture* pBossAttack_SpriteSheet{ nullptr };
		AEGfxVertexList* pBossAttack_SpriteMesh{ nullptr };

		// Player Spritesheet
		AEGfxTexture* pPlayerSpriteSheet{ nullptr };
		AEGfxVertexList* pPlayerSpriteMesh{ nullptr };

		AEGfxTexture* pPlayerCombatSpriteSheet{ nullptr };
		AEGfxVertexList* pPlayerCombatSpriteMesh{ nullptr };

		float pixelScale{ 64.0f };

		int frame{};
		int row{};
		float frameTimer{};
		float frameSpeed{ 0.1f };

		float frameUSize{ 1.0f / 8.0f };
		float frameVSize{ 1.0f / 7.0f };

		float u0{ frame * frameUSize };
		float v0{ row * frameVSize };


		int pFrame{};
		int pRow{};
		float pFrameTimer{};
		float pFrameSpeed{ 0.1f };

		float pFrameUSize{ 1.0f / 10.0f };
		float pFrameVSize{ 1.0f / 8.0f };

		float p_u0{ pFrame * pFrameUSize };
		float p_v0{ pRow * pFrameVSize };
};