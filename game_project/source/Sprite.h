#pragma once
#include "pch.h"
#include <fstream>
#include <sstream>

class Sprite
{
	public:
		~Sprite();
		void Sprite_Init();
		void Sprite_Update(float dt);

		// Enemy Spritesheet
		// Base 1
		AEGfxTexture* GetSpriteSheet() { return s_pEnemySpriteSheet; }
		AEGfxTexture* GetEnemyWindupSpriteSheet() { return s_pEnemyWindup_SpriteSheet; }
		AEGfxTexture* GetEnemyAttackSpriteSheet() { return s_pEnemyAttack_SpriteSheet; }
		AEGfxVertexList* GetSpriteMesh() { return s_pSpriteMesh; }
		AEGfxVertexList* GetEnemyWindupSpriteMesh() { return s_pEnemyWindup_SpriteMesh; }
		AEGfxVertexList* GetEnemyAttackSpriteMesh() { return s_pEnemyAttack_SpriteMesh; }

		// Dasher
		AEGfxTexture* GetDasherSpriteSheet() { return s_pDasherSpriteSheet; }
		AEGfxTexture* GetDasherWindupSpriteSheet() { return s_pDasherWindup_SpriteSheet; }
		AEGfxTexture* GetDasherAttackSpriteSheet() { return s_pDasherAttack_SpriteSheet; }
		AEGfxVertexList* GetDasherSpriteMesh() { return s_pDasherSpriteMesh; }
		AEGfxVertexList* GetDasherWindupSpriteMesh() { return s_pDasherWindup_SpriteMesh; }
		AEGfxVertexList* GetDasherAttackSpriteMesh() { return s_pDasherAttack_SpriteMesh; }

		// Thrower
		AEGfxTexture* GetThrowerSpriteSheet() { return s_pThrowerSpriteSheet; }
		AEGfxTexture* GetThrowerWindupSpriteSheet() { return s_pThrowerWindup_SpriteSheet; }
		AEGfxTexture* GetThrowerAttackSpriteSheet() { return s_pThrowerAttack_SpriteSheet; }
		AEGfxVertexList* GetThrowerSpriteMesh() { return s_pThrowerSpriteMesh; }
		AEGfxVertexList* GetThrowerWindupSpriteMesh() { return s_pThrowerWindup_SpriteMesh; }
		AEGfxVertexList* GetThrowerAttackSpriteMesh() { return s_pThrowerAttack_SpriteMesh; }

		// Boss
		AEGfxTexture* GetBossSpriteSheet() { return s_pBossSpriteSheet; }
		AEGfxTexture* GetBossWindupSpriteSheet() { return s_pBossWindup_SpriteSheet; }
		AEGfxTexture* GetBossAttackSpriteSheet() { return s_pBossAttack_SpriteSheet; }

		AEGfxVertexList* GetBossSpriteMesh() { return s_pBossSpriteMesh; }
		AEGfxVertexList* GetBossWindupSpriteMesh() { return s_pBossWindup_SpriteMesh; }
		AEGfxVertexList* GetBossAttackSpriteMesh() { return s_pBossAttack_SpriteMesh; }

		// Player
		AEGfxTexture* GetPlayerSpriteSheet() { return s_pPlayerSpriteSheet; }
		AEGfxVertexList* GetPlayerSpriteMesh() { return s_pPlayerSpriteMesh; }

		AEGfxTexture* GetPlayerCombatSpriteSheet() { return s_pPlayerCombatSpriteSheet; }
		AEGfxVertexList* GetPlayerCombatSpriteMesh() { return s_pPlayerCombatSpriteMesh; }

		// Plus One
		AEGfxTexture* GetPlusOneSpriteSheet() { return s_pPlusOneSpriteSheet; }
		AEGfxVertexList* GetPlusOneSpriteMesh() { return s_pPlusOneSpriteMesh; }

		float GetPixelScale() const { return m_pixelScale; }

		float GetU() const { return m_u0; }
		float GetV() const { return m_v0; }

		float GetPlayerU() const { return m_p_u0; }
		float GetPlayerV() const { return m_p_v0; }

		int GetFrame() const { return m_frame; }
		int GetRow() const { return m_row; }
		int GetFrameWidth() const { return static_cast<int>(m_frameUSize); }
		int GetFrameHeight() const { return static_cast<int>(m_frameVSize); }
		int GetPlayerFrame() const { return m_pFrame; }

		void SetTextureU() { m_u0 = m_frame * m_frameUSize; }
		void SetTextureV(int animRow) { m_row = animRow; m_v0 = m_row * m_frameVSize; }

		void SetTexturePlayerU() { m_p_u0 = m_pFrame * m_pFrameUSize; }
		void SetTexturePlayerV(int animRow) { m_pRow = animRow; m_p_v0 = m_pRow * m_pFrameVSize; }

		void SetSingleFrameTexture() {
			m_frame = 0;
			m_row = 0;
			m_frameUSize = 1.0f;
			m_frameVSize = 1.0f;
			SetTextureU();
			SetTextureV(0);
		}

		void SetEnemyAttackSingleFrame(int column, int rowIndex)
		{
			m_frame = column;
			m_row = rowIndex;
			m_frameUSize = 1.0f / 8.0f;
			m_frameVSize = 1.0f / 7.0f;
			SetTextureU();
			SetTextureV(rowIndex);
		}

		void SetDasherAttackSingleFrame(int column, int rowIndex)
		{
			m_frame = column;
			m_row = rowIndex;
			m_frameUSize = 1.0f / 8.0f;
			m_frameVSize = 1.0f / 7.0f;
			SetTextureU();
			SetTextureV(rowIndex);
		}

		int GetPlusOneFrame() const { return m_plusOneFrame; }

		void SetPlusOneTextureU() { m_plusOneU0 = m_plusOneFrame * m_plusOneFrameUSize; }
		void SetPlusOneTextureV(int row) { m_plusOneRow = row; m_plusOneV0 = m_plusOneRow * m_plusOneFrameVSize; }

		float GetPlusOneU() const { return m_plusOneU0; }
		float GetPlusOneV() const { return m_plusOneV0; }

		void SetPlusOneFrame(int frame, int row = 0)
		{
			m_plusOneFrame = frame;
			m_plusOneRow = row;
			m_plusOneFrameUSize = 1.0f / 8.0f; // change if not 8
			m_plusOneFrameVSize = 1.0f / 1.0f;
			SetPlusOneTextureU();
			SetPlusOneTextureV(row);
		}

		float GetPlusOneFrameSpeed() const { return m_plusOneFrameSpeed; }
		void SetPlusOneFrameSpeed(float speed) { m_plusOneFrameSpeed = speed; }

		void ResetPlusOneAnimation()
		{
			m_plusOneFrame = 0;
			m_plusOneRow = 0;
			m_plusOneFrameTimer = 0.0f;
			m_plusOneFrameUSize = 1.0f / 8.0f;
			m_plusOneFrameVSize = 1.0f / 1.0f;
			SetPlusOneTextureU();
			SetPlusOneTextureV(0);
		}

	private:
		static void Sprite_Load();

		// Shared assets (loaded once, ref-counted across all Sprite instances)
		static int s_refCount;

		// Enemy Spritesheet
		static AEGfxTexture* s_pEnemySpriteSheet;
		static AEGfxVertexList* s_pSpriteMesh;

		static AEGfxTexture* s_pEnemyWindup_SpriteSheet;
		static AEGfxVertexList* s_pEnemyWindup_SpriteMesh;

		static AEGfxTexture* s_pEnemyAttack_SpriteSheet;
		static AEGfxVertexList* s_pEnemyAttack_SpriteMesh;

		// Dasher
		static AEGfxTexture* s_pDasherSpriteSheet;
		static AEGfxVertexList* s_pDasherSpriteMesh;

		static AEGfxTexture* s_pDasherWindup_SpriteSheet;
		static AEGfxVertexList* s_pDasherWindup_SpriteMesh;

		static AEGfxTexture* s_pDasherAttack_SpriteSheet;
		static AEGfxVertexList* s_pDasherAttack_SpriteMesh;

		// Thrower
		static AEGfxTexture* s_pThrowerSpriteSheet;
		static AEGfxVertexList* s_pThrowerSpriteMesh;

		static AEGfxTexture* s_pThrowerWindup_SpriteSheet;
		static AEGfxVertexList* s_pThrowerWindup_SpriteMesh;

		static AEGfxTexture* s_pThrowerAttack_SpriteSheet;
		static AEGfxVertexList* s_pThrowerAttack_SpriteMesh;

		// Boss
		static AEGfxTexture* s_pBossSpriteSheet;
		static AEGfxVertexList* s_pBossSpriteMesh;

		static AEGfxTexture* s_pBossWindup_SpriteSheet;
		static AEGfxVertexList* s_pBossWindup_SpriteMesh;

		static AEGfxTexture* s_pBossAttack_SpriteSheet;
		static AEGfxVertexList* s_pBossAttack_SpriteMesh;

		// Player Spritesheet
		static AEGfxTexture* s_pPlayerSpriteSheet;
		static AEGfxVertexList* s_pPlayerSpriteMesh;

		static AEGfxTexture* s_pPlayerCombatSpriteSheet;
		static AEGfxVertexList* s_pPlayerCombatSpriteMesh;

		// Plus One
		static AEGfxTexture* s_pPlusOneSpriteSheet;
		static AEGfxVertexList* s_pPlusOneSpriteMesh;

		// Per-instance animation state
		bool m_initialized{ false };
		float m_pixelScale{ 64.0f };

		int m_frame{};
		int m_row{};
		float m_frameTimer{};
		float m_frameSpeed{ 0.1f };

		float m_frameUSize{ 1.0f / 8.0f };
		float m_frameVSize{ 1.0f / 7.0f };

		float m_u0{ m_frame * m_frameUSize };
		float m_v0{ m_row * m_frameVSize };


		int m_pFrame{};
		int m_pRow{};
		float m_pFrameTimer{};
		float m_pFrameSpeed{ 0.1f };

		float m_pFrameUSize{ 1.0f / 10.0f };
		float m_pFrameVSize{ 1.0f / 8.0f };

		float m_p_u0{ m_pFrame * m_pFrameUSize };
		float m_p_v0{ m_pRow * m_pFrameVSize };

		int m_plusOneFrame{};
		int m_plusOneRow{};
		float m_plusOneFrameUSize{ 1.0f / 8.0f };
		float m_plusOneFrameVSize{ 1.0f / 1.0f };
		float m_plusOneU0{ 0.0f };
		float m_plusOneV0{ 0.0f };
		float m_plusOneFrameTimer{};
		float m_plusOneFrameSpeed{ 0.50f };
};
