#pragma once
#include "Utils.h" // Access to AE system and Grid constants

class Augments
{
public:
    void Init();
    void Interact(f32 playerX, f32 playerY);
    void Choosing();
    void Draw(f32 playerX, f32 playerY, f32 dt);
    void DrawShadow(f32 dt);
    void Free();

    //// Getters allowing Game.cpp to access position for Camera/Collisions
    //float GetX() const { return m_PosX; }
    //float GetY() const { return m_PosY; }
    //float GetSize() const { return m_Size; }

    //// Setters if you need to teleport the player (e.g. respawning)
    //void SetPosition(float x, float y) { m_PosX = x; m_PosY = y; }

private:
    // Position & Stats
    float augPosX, augPosY;
    float augSize;
    float interactRange;
    float playerX, playerY;

    float hoverPosY, hoverTime, hoverPower, hoverSpeed;

    bool choose = false;

    // Visual Assets
    AEGfxVertexList* augmentMesh = nullptr;
    AEGfxVertexList* cardMesh = nullptr;
};