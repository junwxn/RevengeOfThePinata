/*************************************************************************
@file		Splash.cpp
@Author		Chew Zheng Hui, Timothy Caleb z.chew@digipen.edu
@Co-authors	nil
@brief		This file contains the implementation of the Digipen Splash
			logo appearing on start-up. Home to the base functions
			init, load, draw, free, unload, update

Copyright ? 2026 DigiPen, All rights reserved.
*************************************************************************/

#include "pch.h"
#include "Splash.h"
#include "GameStateManager.h"
#include "Transition.h"
#include "Utils.h"

// --- File-scoped statics ---
static AEGfxTexture* splashTexture = nullptr;
static AEGfxVertexList* splashMesh = nullptr;
static float splashTimer = 0.0f;

// ========== LOAD ==========
void Splash_Load()
{
	if (!splashMesh)
		splashMesh = CreateSpriteRectMesh(0xFFFFFFFF, 1.0f, 1.0f);

	if (!splashTexture)
		splashTexture = AEGfxTextureLoad("Assets/DigiPen_BLACK.png");
}

// ========== INIT ==========
void Splash_Init()
{
	splashTimer = 0.0f;
	AEGfxSetCamPosition(0.0f, 0.0f);
}

// ========== UPDATE ==========
void Splash_Update(float dt)
{
	splashTimer += dt;

	if (splashTimer >= 2.0f) {
		Transition_StartImmediate(GS_MAINMENU);
	}
}

// ========== DRAW ==========
void Splash_Draw()
{
	AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

	if (splashTexture && splashMesh) {
		float alpha = 1.0f;

		// Fade in during first 0.5s
		if (splashTimer < 0.5f) {
			alpha = splashTimer / 0.5f;
		}
		// Fade out during last 0.5s
		else if (splashTimer > 1.5f) {
			alpha = (2.0f - splashTimer) / 0.5f;
		}

		if (alpha < 0.0f) alpha = 0.0f;
		if (alpha > 1.0f) alpha = 1.0f;

		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
		AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetTransparency(alpha);

		AEGfxTextureSet(splashTexture, 0.0f, 0.0f);

		AEMtx33 scale, rotate, translate, transform;
		AEMtx33Scale(&scale, 1000.0f, 360.0f);
		AEMtx33Rot(&rotate, 0.0f);
		AEMtx33Trans(&translate, 0.0f, 0.0f);

		AEMtx33Concat(&transform, &rotate, &scale);
		AEMtx33Concat(&transform, &translate, &transform);

		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(splashMesh, AE_GFX_MDM_TRIANGLES);
	}
}

// ========== FREE ==========
void Splash_Free()
{
}

// ========== UNLOAD ==========
void Splash_Unload()
{
	if (splashMesh) {
		AEGfxMeshFree(splashMesh);
		splashMesh = nullptr;
	}

	if (splashTexture) {
		AEGfxTextureUnload(splashTexture);
		splashTexture = nullptr;
	}
}