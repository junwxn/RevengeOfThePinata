/*************************************************************************
@file       Projectile.cpp
@Author     Nigel Lim, nigelkaiyu.lim@digipen.edu
@Co-authors nil
@brief      This file contains the implementation of the Projectile class,
            including movement, lifetime handling, collision checks,
            rendering, and reflection behaviour.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#include "pch.h"

#include "Projectile.h"
#include "Utils.h"

// ---------------------
// | Class: Projectile |
// ---------------------

// shared mesh used by all projectiles
AEGfxVertexList* Projectile::s_projectileMesh = nullptr;

// constructor initializes projectile properties
Projectile::Projectile(AEVec2 const& startPos,
    AEVec2 const& direction,
    f32 speed,
    f32 radius,
    f32 damage,
    f32 lifetime) :
    m_pos{ startPos },
    m_dir{ direction },
    m_speed{ speed },
    m_radius{ radius },
    m_damage{ damage },
    m_lifetime{ lifetime },
    m_active{ true }
{
    // normalize direction
    f32 length = AEVec2Length(&m_dir);

    if (length > 0.001f) {
        AEVec2Normalize(&m_dir, &m_dir);
    }
    else {
        // fallback direction if invalid
        m_dir = { 1.0f, 0.0f };
    }
}

// initialize shared projectile mesh (only once)
void Projectile::Init() {
    if (!s_projectileMesh) {
        s_projectileMesh = CreateCircleMesh(1.0f, 32, 0xFFFFFF);
    }
}

// updates projectile movement, lifetime, and collision
void Projectile::Update(f32 dt, MapSystem const* pMap) {
    if (!m_active) {
        return;
    }

    // move projectile
    m_pos.x += m_dir.x * m_speed * dt;
    m_pos.y += m_dir.y * m_speed * dt;

    // reduce lifetime
    m_lifetime -= dt;

    if (m_lifetime <= 0.0f) {
        m_lifetime = 0.0f;
        m_active = false;
        return;
    }

    // check collision with map
    if (pMap && pMap->IsPositionBlocked(m_pos.x, m_pos.y, m_radius)) {
        m_active = false;
    }
}

// draw projectile if active
void Projectile::Draw() const {
    if (!m_active) {
        return;
    }

    if (!s_projectileMesh) {
        return;
    }

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    DrawMesh(s_projectileMesh,
        m_radius * 2.0f,
        m_radius * 2.0f,
        m_pos.x,
        m_pos.y,
        0.0f,
        236, 255, 155, 255);
}

// frees shared projectile mesh
void Projectile::Free() {
    if (s_projectileMesh) {
        AEGfxMeshFree(s_projectileMesh);
        s_projectileMesh = nullptr;
    }
}

// sets a new direction and normalizes it
void Projectile::SetDirection(AEVec2 const& dir) {
    m_dir = dir;

    f32 length = AEVec2Length(&m_dir);

    if (length > 0.001f) {
        AEVec2Normalize(&m_dir, &m_dir);
    }
    else {
        m_dir = { 1.0f, 0.0f };
    }
}

// reflects projectile in a new direction (used for parry mechanics)
void Projectile::Reflect(AEVec2 const& newDir) {
    m_dir = newDir;

    if (AEVec2Length(&m_dir) > 0.001f) {
        AEVec2Normalize(&m_dir, &m_dir);
    }

    // mark as reflected and give short grace period
    m_isReflected = true;
    m_reflectGrace = 0.1f;
}