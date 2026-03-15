#include "pch.h"

#include "Projectile.h"
#include "Utils.h"

// ---------------------
// | Class: Projectile |
// ---------------------
AEGfxVertexList* Projectile::s_projectileMesh = nullptr;

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
    f32 length = AEVec2Length(&m_dir);

    if (length > 0.001f) {
        AEVec2Normalize(&m_dir, &m_dir);
    }
    else {
        m_dir = { 1.0f, 0.0f };
    }
}

void Projectile::Init(){
    if (!s_projectileMesh) {
        s_projectileMesh = CreateCircleMesh(1.0f, 32, 0xFFFFFF);
    }
}

void Projectile::Update(f32 dt, MapSystem const* pMap) {
    if (!m_active) {
        return;
    }

    // Move projectile
    m_pos.x += m_dir.x * m_speed * dt;
    m_pos.y += m_dir.y * m_speed * dt;

    // Projectile lifetime
    m_lifetime -= dt;

    if (m_lifetime <= 0.0f) {
        m_lifetime = 0.0f;
        m_active = false;
        return;
    }

    // Wall collision
    if (pMap && pMap->IsPositionBlocked(m_pos.x, m_pos.y, m_radius)) {
        m_active = false;
    }
}

void Projectile::Draw() const {
    if (!m_active) {
        return;
    }

    if (!s_projectileMesh) {
        return;
    }

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    DrawMesh(s_projectileMesh, m_radius*2.0f, m_radius*2.0f, m_pos.x, m_pos.y, 0.0f,
        236, 255, 155, 255);
}

// Uses one mesh for all projectiles
void Projectile::Free() {
    if (s_projectileMesh) {
        AEGfxMeshFree(s_projectileMesh);
        s_projectileMesh = nullptr;
    }
}

void Projectile::SetDirection(AEVec2 const& dir){
    m_dir = dir;

    f32 length = AEVec2Length(&m_dir);

    if (length > 0.001f) {
        AEVec2Normalize(&m_dir, &m_dir);
    }
    else {
        m_dir = { 1.0f, 0.0f };
    }
}

void Projectile::Reflect(AEVec2 const& newDir) {
    m_dir = newDir;

    if (AEVec2Length(&m_dir) > 0.001f) {
        AEVec2Normalize(&m_dir, &m_dir);
    }

    m_isReflected = true;
    m_reflectGrace = 0.1f;
}
