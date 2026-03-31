/*************************************************************************
@file		Camera.cpp
@Author		Chiu Jun Wen j.chiu@digipen.edu
@Co-authors	nil
@brief		This file contains the function definitions for managing the camera,
            including its initialization, updating, and screen shake effects.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#include "pch.h"

#include "Camera.h"
#include "Utils.h"
#include <random>

void Camera::Init(f32 startX, f32 startY) {
    m_X = startX;
    m_Y = startY;

    m_Speed = 1.5f;
    m_LookDist = 350.0f;
}

void Camera::Update(f32 dt, f32 playerX, f32 playerY, bool preventing_movement)
{
    if (m_ScreenShake.timer > 0.0f)
    {
        m_ScreenShake.timer -= dt;
        if (m_ScreenShake.timer < 0.0f)
            m_ScreenShake.timer = 0.0f;

        m_ScreenShake.on = true;

        f32 shakeMagnitude = m_ScreenShake.magnitude * (m_ScreenShake.timer / m_ScreenShake.duration);
        m_shakeOffsetX = (rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakeMagnitude;
        m_shakeOffsetY = (rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakeMagnitude;
    }
    else
    {
        m_shakeOffsetX = 0.0f;
        m_shakeOffsetY = 0.0f;
        m_ScreenShake.on = false;
    }

    float dirX = 0.0f;
    float dirY = 0.0f;

    if (!preventing_movement)
    {
        if (AEInputCheckCurr(AEVK_W)) dirY += 1.0f;
        if (AEInputCheckCurr(AEVK_S)) dirY -= 1.0f;
        if (AEInputCheckCurr(AEVK_D)) dirX += 1.0f;
        if (AEInputCheckCurr(AEVK_A)) dirX -= 1.0f;
    }

    float lookOffsetX = 0.0f;
    float lookOffsetY = 0.0f;

    if (dirX != 0.0f || dirY != 0.0f)
    {
        if (dirX != 0.0f && dirY != 0.0f)
        {
            float halfW = GRID_W * 0.5f;
            float halfH = GRID_H * 0.5f;
            float length = sqrt(halfW * halfW + halfH * halfH);

            float isoStepX = halfW / length;
            float isoStepY = halfH / length;

            lookOffsetX = (dirX > 0 ? isoStepX : -isoStepX) * m_LookDist;
            lookOffsetY = (dirY > 0 ? isoStepY : -isoStepY) * m_LookDist;
        }
        else
        {
            lookOffsetX = dirX * m_LookDist;
            lookOffsetY = dirY * m_LookDist;
        }
    }

    float targetX = playerX + lookOffsetX;
    float targetY = playerY + lookOffsetY;

    m_X += (targetX - m_X) * m_Speed * dt;
    m_Y += (targetY - m_Y) * m_Speed * dt;
}