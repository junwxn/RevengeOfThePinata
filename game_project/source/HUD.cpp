/*************************************************************************
@file		HUD.cpp
@Author		Chiu Jun Wen j.chiu@digipen.edu
@Co-authors	nil
@brief		This file contains the function definitions for managing the
			head-up display (HUD), including its initialization, updating,
			and rendering.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#include "pch.h"
#include "HUD.h"
#include "Player.h"
#include "Utils.h"
#include "AugmentData.h"
#include <cstdio>

// --- File-scoped statics ---
static AEGfxVertexList* s_rectMesh = nullptr;
static s8 s_hudFont = -1;
static s8 s_smallFont = -1;

void HUD_Load() {
	s_rectMesh  = CreateRectMesh(0xFFFFFFFF);
	s_hudFont   = AEGfxCreateFont("Assets/fonts/Stick-Regular.ttf", 40);
	s_smallFont = AEGfxCreateFont("Assets/fonts/Stick-Regular.ttf", 28);
}

void HUD_Init() {
}

void HUD_Draw(const Player* player, float camX, float camY) {
	if (!player) return;

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// Helper: draw mesh at screen-space position, offset by camera so it stays fixed
	auto drawHUD = [&](float w, float h, float x, float y, float rot, float r, float g, float b, float a) {
		DrawMesh(s_rectMesh, w, h, x + camX, y + camY, rot, r, g, b, a);
	};

	// === Background strip ===
	float stripY = -400.0f; // center of strip
	float stripH = 100.0f;
	drawHUD(1600, stripH, -800, stripY, 0, 10, 10, 15, 200);
	// Top accent border
	drawHUD(1600, 2, -800, stripY + stripH * 0.5f, 0, 180, 140, 60, 220);

	// ============================================
	// HEALTH SECTION (left region: -780 to -280)
	// ============================================
	float barX = -680.0f;     // left edge of bar
	float barY = stripY;
	float barW = 450.0f;
	float barH = 32.0f;

	Combat::CombatStats stats = player->GetCombatStats();
	float health    = stats.health;
	float maxHealth = stats.maxHealth;
	if (maxHealth <= 0.0f) maxHealth = 200.0f;
	float ratio = health / maxHealth;
	if (ratio < 0.0f) ratio = 0.0f;
	if (ratio > 1.0f) ratio = 1.0f;

	// "HP" label
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	float tw, th;
	AEGfxGetPrintSize(s_hudFont, "HP", 1.0f, &tw, &th);
	AEGfxPrint(s_hudFont, "HP", (barX - 50.0f) / 800.0f - tw * 0.5f, barY / 450.0f - th * 0.5f, 1.2f, 1.0f, 0.85f, 0.0f, 1.0f);

	// Re-set render mode after text
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// Bar border (drawn first, behind)
	drawHUD(barW + 4, barH + 4, barX - 2, barY, 0, 80, 80, 80, 180);

	// Bar background (dark)
	drawHUD(barW, barH, barX, barY, 0, 30, 30, 30, 220);

	// Bar fill (green to red gradient based on ratio)
	float fillW = barW * ratio;
	if (fillW > 0.0f) {
		float r = (1.0f - ratio) * 220.0f + 30.0f;
		float g = ratio * 200.0f + 30.0f;
		float b = 30.0f;
		drawHUD(fillW, barH, barX, barY, 0, r, g, b, 240);
	}

	// HP numeric text centered over bar
	char hpText[32];
	snprintf(hpText, sizeof(hpText), "%.0f/%.0f", health, maxHealth);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxGetPrintSize(s_smallFont, hpText, 1.1f, &tw, &th);
	float barCenterX = barX + barW * 0.5f;
	AEGfxPrint(s_smallFont, hpText, barCenterX / 800.0f - tw * 0.5f, barY / 450.0f - th * 0.5f, 1.1f, 1.0f, 1.0f, 1.0f, 1.0f);

	// ============================================
	// CHARGES SECTION (center: -200 to +200)
	// ============================================
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	int charges    = player->GetAttackCharges();
	int maxCharges = player->GetMaxAttackCharge();
	float boxSize  = 40.0f;
	float boxGap   = 10.0f;
	float totalW   = maxCharges * boxSize + (maxCharges - 1) * boxGap;
	float startX   = -totalW * 0.5f - 70.0f;

	for (int i = 0; i < maxCharges; ++i) {
		float bx = startX + i * (boxSize + boxGap);
		float by = stripY;

		// Border
		drawHUD(boxSize + 4, boxSize + 4, bx - 2, by, 0, 80, 70, 40, 200);

		if (i < charges) {
			// Filled — gold
			drawHUD(boxSize, boxSize, bx, by, 0, 255, 230, 50, 240);
		}
		else if (i == charges && charges < maxCharges) {
			// Recharging — same style as dash
			float rechargeTime = player->GetAttackChargeTime();
			float rechargeTimer = player->GetAttackChargeTimer();
			float progress = (rechargeTime > 0.0f) ? (rechargeTimer / rechargeTime) : 0.0f;
			if (progress < 0.0f) progress = 0.0f;
			if (progress > 1.0f) progress = 1.0f;

			// Dark background
			drawHUD(boxSize, boxSize, bx, by, 0, 35, 30, 25, 200);

			// Fill from bottom
			float fillH = boxSize * progress;
			if (fillH > 0.0f)
				drawHUD(boxSize, fillH, bx, by - (boxSize - fillH) * 0.5f, 0, 220, 140, 30, 220);
		}
		else {
			// Empty — dark
			drawHUD(boxSize, boxSize, bx, by, 0, 35, 30, 25, 200);
		}
	}

	// "ATTACK CHARGES" label above boxes
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxGetPrintSize(s_smallFont, "ATTACK", 1.0f, &tw, &th);
	float attackCenterX = startX + totalW * 0.5f;
	AEGfxPrint(s_smallFont, "ATTACK", attackCenterX / 800.0f - tw * 0.5f, (stripY + boxSize * 0.5f + 7.5f) / 450.0f, 1.0f, 0.8f, 0.75f, 0.5f, 1.0f);

	// ============================================
	// DASH CHARGES (between attack charges and augments)
	// ============================================
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	int dashCharges    = player->GetDashCharges();
	int maxDash        = player->GetMaxDashCharges();
	float dashBoxSize  = 40.0f;
	float dashBoxGap   = 10.0f;
	float dashTotalW   = maxDash * dashBoxSize + (maxDash - 1) * dashBoxGap;
	float dashStartX   = -780.0f + 100.0f; // left side, after HP label area
	// Position to the right of attack charges
	dashStartX = startX + totalW + 75.0f; // 40px gap after attack charges

	for (int i = 0; i < maxDash; ++i) {
		float dx = dashStartX + i * (dashBoxSize + dashBoxGap);
		float dy = stripY;

		// Border (cyan tint)
		drawHUD(dashBoxSize + 4, dashBoxSize + 4, dx - 2, dy, 0, 0, 140, 160, 200);

		if (i < dashCharges) {
			// Filled — cyan
			drawHUD(dashBoxSize, dashBoxSize, dx, dy, 0, 0, 200, 220, 240);
		}
		else if (i == dashCharges && dashCharges < maxDash) {
			// Recharging — show progress bar
			float rechargeTime  = player->GetDashRechargeTime();
			float rechargeTimer = player->GetDashRechargeTimer();
			float progress = (rechargeTime > 0.0f) ? 1.0f - (rechargeTimer / rechargeTime) : 0.0f;
			if (progress < 0.0f) progress = 0.0f;
			if (progress > 1.0f) progress = 1.0f;

			// Dark background
			drawHUD(dashBoxSize, dashBoxSize, dx, dy, 0, 15, 25, 30, 200);
			// Fill from bottom
			float fillH = dashBoxSize * progress;
			if (fillH > 0.0f)
				drawHUD(dashBoxSize, fillH, dx, dy - (dashBoxSize - fillH) * 0.5f, 0, 0, 140, 160, 200);
		}
		else {
			// Empty — dark
			drawHUD(dashBoxSize, dashBoxSize, dx, dy, 0, 15, 25, 30, 200);
		}
	}

	// "DASH" label above boxes
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	float dashLabelCenterX = dashStartX + dashTotalW * 0.5f;
	AEGfxGetPrintSize(s_smallFont, "DASH", 1.0f, &tw, &th);
	AEGfxPrint(s_smallFont, "DASH", dashLabelCenterX / 800.0f - tw * 0.5f, (stripY + dashBoxSize * 0.5f + 7.5f) / 450.0f, 1.0f, 0.0f, 0.78f, 0.86f, 1.0f);

	// ============================================
	// AUGMENTS SECTION (right: +250 to +780)
	// ============================================
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	// Colors: slot 0=Dash(cyan), slot 1=Attack(orange), slot 2=Parry(purple)
	float slotColors[3][3] = {
		{ 0, 200, 220 },    // cyan - Dash
		{ 220, 140, 30 },   // orange - Attack
		{ 160, 80, 200 }    // purple - Parry
	};
	const char* slotLabels[3] = { "DASH", "ATK", "PARRY" };

	float slotW    = 170.0f;
	float slotH    = 46.0f;
	float slotGap  = 14.0f;
	float slotTotalW = 3 * slotW + 2 * slotGap;
	float slotStartX = 780.0f - slotTotalW; // right-aligned

	for (int i = 0; i < 3; ++i) {
		float sx = slotStartX + i * (slotW + slotGap);
		float sy = stripY;

		float cr = slotColors[i][0];
		float cg = slotColors[i][1];
		float cb = slotColors[i][2];

		// Slot border (colored)
		drawHUD(slotW + 4, slotH + 4, sx - 2, sy, 0, cr, cg, cb, 200);
		// Slot fill (dark)
		drawHUD(slotW, slotH, sx, sy, 0, 20, 18, 25, 220);

		// Augment name or "---"
		AugmentID id = g_Augments.chosen[i];
		const char* name = (id != AugmentID::NONE) ? GetAugmentInfo(id).name : "---";

		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		float fontSize = 0.85f;
		AEGfxGetPrintSize(s_smallFont, name, fontSize, &tw, &th);
		// Shrink font if text overflows the slot (with small padding)
		float maxTw = (slotW - 10.0f) / 800.0f;
		if (tw > maxTw) {
			fontSize *= maxTw / tw;
			AEGfxGetPrintSize(s_smallFont, name, fontSize, &tw, &th);
		}
		float slotCenterX = sx + slotW * 0.5f;
		AEGfxPrint(s_smallFont, name, slotCenterX / 800.0f - tw * 0.5f, sy / 450.0f - th * 0.5f, fontSize,
			cr / 255.0f, cg / 255.0f, cb / 255.0f, 1.0f);

		// Slot category label above
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	}

	// Category labels above slots
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	for (int i = 0; i < 3; ++i) {
		float sx = slotStartX + i * (slotW + slotGap);
		float slotCenterX = sx + slotW * 0.5f;
		AEGfxGetPrintSize(s_smallFont, slotLabels[i], 0.75f, &tw, &th);
		AEGfxPrint(s_smallFont, slotLabels[i], slotCenterX / 800.0f - tw * 0.5f, (stripY + slotH * 0.5f + 7.5f) / 450.0f, 0.75f,
			0.6f, 0.6f, 0.5f, 1.0f);

		// Re-set after each print
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	}
}

void HUD_Free() {
}

void HUD_Unload() {
	if (s_rectMesh) {
		AEGfxMeshFree(s_rectMesh);
		s_rectMesh = nullptr;
	}
	if (s_hudFont >= 0) {
		AEGfxDestroyFont(s_hudFont);
		s_hudFont = -1;
	}
	if (s_smallFont >= 0) {
		AEGfxDestroyFont(s_smallFont);
		s_smallFont = -1;
	}
}
