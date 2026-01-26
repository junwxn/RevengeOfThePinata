#include "Camera.h"

void Camera::Init(f32 startX, f32 startY) {
	m_X = startX;
	m_Y = startY;

	m_Speed = 2.0f;
	m_LookDist = 300.0f;
}

void Camera::Update(f32 dt, f32 playerX, f32 playerY) {
    // --- 1. Determine Look-Ahead Offset ---
    float lookOffsetX = 0.0f;
    float lookOffsetY = 0.0f;

    // Check Input (AEInput is global, so we can check it here)
    if (AEInputCheckCurr(AEVK_W)) lookOffsetY += m_LookDist;
    if (AEInputCheckCurr(AEVK_S)) lookOffsetY -= m_LookDist;
    if (AEInputCheckCurr(AEVK_D)) lookOffsetX += m_LookDist;
    if (AEInputCheckCurr(AEVK_A)) lookOffsetX -= m_LookDist;

    // --- 2. Calculate Target Position ---
    float targetX = playerX + lookOffsetX;
    float targetY = playerY + lookOffsetY;

    // --- 3. Smooth Movement (Lerp) ---
    // Formula: Current += (Target - Current) * Speed * dt
    m_X += (targetX - m_X) * m_Speed * dt;
    m_Y += (targetY - m_Y) * m_Speed * dt;

    // --- 4. Apply to Alpha Engine ---
    AEGfxSetCamPosition(m_X, m_Y);
}