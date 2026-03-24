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
		AEGfxTexture* GetSpriteSheet() { return pEnemySpriteSheet; }
		AEGfxTexture* GetEnemyWindupSpriteSheet() { return pEnemyWindup_SpriteSheet; }
		AEGfxTexture* GetEnemyAttackSpriteSheet() { return pEnemyAttack_SpriteSheet; }
		AEGfxVertexList* GetSpriteMesh() { return pSpriteMesh; }
		AEGfxVertexList* GetEnemyWindupSpriteMesh() { return pEnemyWindup_SpriteMesh; }
		AEGfxVertexList* GetEnemyAttackSpriteMesh() { return pEnemyAttack_SpriteMesh; }

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

	private:
		// Enemy Spritesheet
		AEGfxTexture* pEnemySpriteSheet{nullptr};
		AEGfxVertexList* pSpriteMesh{nullptr};

		AEGfxTexture* pEnemyWindup_SpriteSheet{ nullptr };
		AEGfxVertexList* pEnemyWindup_SpriteMesh{ nullptr };

		AEGfxTexture* pEnemyAttack_SpriteSheet{ nullptr };
		AEGfxVertexList* pEnemyAttack_SpriteMesh{ nullptr };

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