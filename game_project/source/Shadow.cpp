/*************************************************************************
@file		Shadow.cpp
@Author		Chiu Jun Wen j.chiu@digipen.edu
@Co-authors	nil
@brief		This file contains the function definitions for managing shadows,
			including their initialization, rendering, and cleanup.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/
#include "pch.h"
#include "Shadow.h"
#include "Utils.h"

static AEGfxVertexList* s_shadowMesh = nullptr;

static const float SHADOW_WIDTH_SCALE = 1.2f;
static const float SHADOW_HEIGHT_RATIO = 0.4f;
static const float SHADOW_Y_OFFSET = -2.0f;
static const float SHADOW_ALPHA = 30.0f;

void Shadow_Init() {
	if (s_shadowMesh) {
		AEGfxMeshFree(s_shadowMesh);
		s_shadowMesh = nullptr;
	}
	s_shadowMesh = CreateCircleMesh(1.0f, 32, 0x000000);
}

void Shadow_Free() {
	if (s_shadowMesh) {
		AEGfxMeshFree(s_shadowMesh);
		s_shadowMesh = nullptr;
	}
}

void Shadow_Draw(float x, float y, float entitySize) {
	if (!s_shadowMesh) return;

	float shadowW = entitySize * SHADOW_WIDTH_SCALE;
	float shadowH = shadowW * SHADOW_HEIGHT_RATIO;
	float shadowY = y + entitySize * SHADOW_Y_OFFSET;

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(SHADOW_ALPHA / 255.0f);
	AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, SHADOW_ALPHA / 255.0f);

	AEMtx33 scale, translate, transform;
	AEMtx33Scale(&scale, shadowW, shadowH);
	AEMtx33Trans(&translate, x, shadowY);
	AEMtx33Concat(&transform, &translate, &scale);

	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(s_shadowMesh, AE_GFX_MDM_TRIANGLES);

	AEGfxSetTransparency(1.0f);
}
