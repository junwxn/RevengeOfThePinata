#pragma once
#include "Utils.h" // Access to AE system and Grid constants

class Player
{
public:
    void Init();
    void Update(float dt);
    void Draw();
    void Free();

    // Getters allowing Game.cpp to access position for Camera/Collisions
    float GetX() const { return m_PosX; }
    float GetY() const { return m_PosY; }
    float GetSize() const { return m_Size; }

    // Setters if you need to teleport the player (e.g. respawning)
    void SetPosition(float x, float y) { m_PosX = x; m_PosY = y; }

private:
    // Position & Stats
    float m_PosX, m_PosY;
    float m_Speed;
    float m_Size;

    // Dash Logic
    float m_DashCooldown;
    float m_DashCooldown_Default;

    // Visual Assets
    AEGfxVertexList* m_pMesh = nullptr;
};