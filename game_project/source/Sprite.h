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
		AEGfxTexture* GetSpriteSheet() { return m_pEnemySpriteSheet; }
		AEGfxTexture* GetEnemyWindupSpriteSheet() { return m_pEnemyWindup_SpriteSheet; }
		AEGfxTexture* GetEnemyAttackSpriteSheet() { return m_pEnemyAttack_SpriteSheet; }
		AEGfxVertexList* GetSpriteMesh() { return m_pSpriteMesh; }
		AEGfxVertexList* GetEnemyWindupSpriteMesh() { return m_pEnemyWindup_SpriteMesh; }
		AEGfxVertexList* GetEnemyAttackSpriteMesh() { return m_pEnemyAttack_SpriteMesh; }

		// Dasher
		AEGfxTexture* GetDasherSpriteSheet() { return m_pDasherSpriteSheet; }
		AEGfxTexture* GetDasherWindupSpriteSheet() { return m_pDasherWindup_SpriteSheet; }
		AEGfxTexture* GetDasherAttackSpriteSheet() { return m_pDasherAttack_SpriteSheet; }
		AEGfxVertexList* GetDasherSpriteMesh() { return m_pDasherSpriteMesh; }
		AEGfxVertexList* GetDasherWindupSpriteMesh() { return m_pDasherWindup_SpriteMesh; }
		AEGfxVertexList* GetDasherAttackSpriteMesh() { return m_pDasherAttack_SpriteMesh; }

		// Boss
		AEGfxTexture* GetBossSpriteSheet() { return m_pBossSpriteSheet; }
		AEGfxTexture* GetBossWindupSpriteSheet() { return m_pBossWindup_SpriteSheet; }
		AEGfxTexture* GetBossAttackSpriteSheet() { return m_pBossAttack_SpriteSheet; }

		AEGfxVertexList* GetBossSpriteMesh() { return m_pBossSpriteMesh; }
		AEGfxVertexList* GetBossWindupSpriteMesh() { return m_pBossWindup_SpriteMesh; }
		AEGfxVertexList* GetBossAttackSpriteMesh() { return m_pBossAttack_SpriteMesh; }

		// Player
		AEGfxTexture* GetPlayerSpriteSheet() { return m_pPlayerSpriteSheet; }
		AEGfxVertexList* GetPlayerSpriteMesh() { return m_pPlayerSpriteMesh; }

		AEGfxTexture* GetPlayerCombatSpriteSheet() { return m_pPlayerCombatSpriteSheet; }
		AEGfxVertexList* GetPlayerCombatSpriteMesh() { return m_pPlayerCombatSpriteMesh; }

		// Plus One
		AEGfxTexture* GetPlusOneSpriteSheet() { return m_pPlusOneSpriteSheet; }
		AEGfxVertexList* GetPlusOneSpriteMesh() { return m_pPlusOneSpriteMesh; }

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
		// Enemy Spritesheet
		// Base
		AEGfxTexture* m_pEnemySpriteSheet{nullptr};
		AEGfxVertexList* m_pSpriteMesh{nullptr};

		AEGfxTexture* m_pEnemyWindup_SpriteSheet{ nullptr };
		AEGfxVertexList* m_pEnemyWindup_SpriteMesh{ nullptr };

		AEGfxTexture* m_pEnemyAttack_SpriteSheet{ nullptr };
		AEGfxVertexList* m_pEnemyAttack_SpriteMesh{ nullptr };

		// Dasher
		AEGfxTexture* m_pDasherSpriteSheet{ nullptr };
		AEGfxVertexList* m_pDasherSpriteMesh{ nullptr };

		AEGfxTexture* m_pDasherWindup_SpriteSheet{ nullptr };
		AEGfxVertexList* m_pDasherWindup_SpriteMesh{ nullptr };

		AEGfxTexture* m_pDasherAttack_SpriteSheet{ nullptr };
		AEGfxVertexList* m_pDasherAttack_SpriteMesh{ nullptr };

		// Boss
		AEGfxTexture* m_pBossSpriteSheet{ nullptr };
		AEGfxVertexList* m_pBossSpriteMesh{ nullptr };

		AEGfxTexture* m_pBossWindup_SpriteSheet{ nullptr };
		AEGfxVertexList* m_pBossWindup_SpriteMesh{ nullptr };

		AEGfxTexture* m_pBossAttack_SpriteSheet{ nullptr };
		AEGfxVertexList* m_pBossAttack_SpriteMesh{ nullptr };

		// Player Spritesheet
		AEGfxTexture* m_pPlayerSpriteSheet{ nullptr };
		AEGfxVertexList* m_pPlayerSpriteMesh{ nullptr };

		AEGfxTexture* m_pPlayerCombatSpriteSheet{ nullptr };
		AEGfxVertexList* m_pPlayerCombatSpriteMesh{ nullptr };

		// Plus One
		AEGfxTexture* m_pPlusOneSpriteSheet{ nullptr };
		AEGfxVertexList* m_pPlusOneSpriteMesh{ nullptr };

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