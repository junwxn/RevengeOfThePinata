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

		AEGfxVertexList* GetSpriteMesh() { return pSpriteMesh; }
		AEGfxTexture* GetSpriteSheet() { return pEnemySpriteSheet; }
		float GetPixelScale() const { return pixelScale; }

		float GetU() const { return u0; }
		float GetV() const { return v0; }

		int GetFrame() const { return frame; }
		int GetRow() const { return row; }
		int GetFrameWidth() const { return frameUSize; }
		int GetFrameHeight() const { return frameVSize; }

		void SetTextureU() { u0 = frame * frameUSize; }
		void SetTextureV(int animRow) { row = animRow; v0 = row * frameVSize; }

	private:
		AEGfxTexture* pEnemySpriteSheet{nullptr};
		AEGfxVertexList* pSpriteMesh{nullptr};

		float pixelScale{ 64.0f };

		int frame{};
		int row{};
		float frameTimer{};
		float frameSpeed{ 0.1f };

		float frameUSize{ 1.0f / 8.0f };
		float frameVSize{ 1.0f / 7.0f };

		float u0{ frame * frameUSize };
		float v0{ row * frameVSize };
};