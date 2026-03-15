#pragma once

#include "pch.h"
#include "Map.h"

// ---------------------
// | Class: Projectile |
// ---------------------
enum class ProjectileType {
    Normal,
    Reflect
};

class Projectile {
public:
    // Constructor
    Projectile(AEVec2 const& startPos = { 0.0f, 0.0f },
               AEVec2 const& direction = { 1.0f, 0.0f },
               f32 speed = 0.0f,
               f32 radius = .0f,
               f32 damage = 10.0f,
               f32 lifetime = 3.0f);

    // Obj functions
    void Init();
    void Update(f32 dt, MapSystem const* pMap);
    void Draw() const;

    // Class functions
    static void Free();
    void Reflect(AEVec2 const& newDir);
    bool IsInReflectGrace() const { return m_reflectGrace > 0.0f; }

    // Getters
    bool IsActive() const { return m_active; };
    AEVec2 GetPosition() const { return m_pos; };
    f32 GetRadius() const { return m_radius; };
    f32 GetDamage() const { return m_damage; };
    bool IsReflected() const { return m_isReflected; }
    ProjectileType GetType() const { return m_type; };

    // Setters
    void SetPosition(AEVec2 const& pos) { m_pos = pos; };
    void SetSpeed(f32 speed) { m_speed = speed; };
    void SetRadius(f32 radius) { m_radius = radius; };
    void SetDamage(f32 damage) { m_damage = damage; };
    void SetLifetime(f32 lifetime) { m_lifetime = lifetime; };
    void SetType(ProjectileType type) { m_type = type; };
    void SetDirection(AEVec2 const& dir);
    void Destroy() { m_active = false; }; // set as inactive/dead

private:
    AEVec2 m_pos{};
    AEVec2 m_dir{ 1.0f, 0.0f };
    f32 m_speed{ 0.0f };
    f32 m_radius{ 8.0f };
    f32 m_damage{ 10.0f };
    f32 m_lifetime{ 3.0f };
    bool m_active{ true };
    bool m_isReflected{ false };
    f32 m_reflectGrace{ 0.0f };
    ProjectileType m_type{ ProjectileType::Normal };

    static AEGfxVertexList* s_projectileMesh;
};