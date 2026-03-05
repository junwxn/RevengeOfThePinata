#include "pch.h"

#include "Camera.h"
#include "Utils.h"
#include <random>

void Camera::Init(f32 startX, f32 startY) {
    m_X = startX;
    m_Y = startY;

    m_Speed = 2.0f;
    m_LookDist = 350.0f;
}

//void Camera::Update(f32 dt, f32 playerX, f32 playerY, bool preventing_movement) {
void Camera::Update(f32 dt, f32 playerX, f32 playerY) {

    if (m_ScreenShake.timer > 0)
    {
        if (m_ScreenShake.timer < 0) m_ScreenShake.timer = 0;
        m_ScreenShake.on = true;
        m_ScreenShake.timer -= dt; // Decrement with time

        f32 shakeMagnitude = m_ScreenShake.magnitude * (m_ScreenShake.timer / m_ScreenShake.duration);
        m_shakeOffsetX = (rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakeMagnitude;
        m_shakeOffsetY = (rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakeMagnitude;
    }
    else
    {
        m_shakeOffsetX = 0;
        m_shakeOffsetY = 0;
        m_ScreenShake.on = false;
    }

    // --- 1. Determine Input Direction ---
    float dirX = 0.0f;
    float dirY = 0.0f;

    if (!preventing_movement) {

        if (AEInputCheckCurr(AEVK_W)) dirY += 1.0f;
        if (AEInputCheckCurr(AEVK_S)) dirY -= 1.0f;
        if (AEInputCheckCurr(AEVK_D)) dirX += 1.0f;
        if (AEInputCheckCurr(AEVK_A)) dirX -= 1.0f;

    }

    // --- 2. Calculate Isometric Offset ---
    float lookOffsetX = 0.0f;
    float lookOffsetY = 0.0f;

    if (dirX != 0.0f || dirY != 0.0f)
    {
        // CASE A: Diagonal Movement (W+D, W+A, etc.)
        // We must align with the Isometric Grid (approx 30 degrees), not Screen (45 degrees)
        if (dirX != 0.0f && dirY != 0.0f)
        {
            // Get the dimensions of one diagonal tile edge
            float halfW = GRID_W * 0.5f;
            float halfH = GRID_H * 0.5f;

            // Calculate the length of that edge (Hypotenuse)
            float length = sqrt(halfW * halfW + halfH * halfH);

            // Normalize based on Tile Shape
            float isoStepX = halfW / length; // Approx 0.86
            float isoStepY = halfH / length; // Approx 0.50

            // Apply direction signs
            lookOffsetX = (dirX > 0 ? isoStepX : -isoStepX) * m_LookDist;
            lookOffsetY = (dirY > 0 ? isoStepY : -isoStepY) * m_LookDist;
        }
        // CASE B: Orthogonal Movement (Just W, A, S, or D)
        // Move straight up/down/left/right
        else
        {
            lookOffsetX = dirX * m_LookDist;
            lookOffsetY = dirY * m_LookDist;
        }
    }

    // --- 3. Calculate Target Position ---
    float targetX = playerX + lookOffsetX;
    float targetY = playerY + lookOffsetY;

    // --- 4. Smooth Movement (Lerp) ---
    m_X += (targetX - m_X) * m_Speed * dt;
    m_Y += (targetY - m_Y) * m_Speed * dt;

    f32 renderX = m_X + m_shakeOffsetX;
    f32 renderY = m_Y + m_shakeOffsetY;

    if (m_ScreenShake.on) AEGfxSetCamPosition(renderX, renderY);

    // --- 5. Apply to Alpha Engine ---
    else AEGfxSetCamPosition(m_X, m_Y);
}