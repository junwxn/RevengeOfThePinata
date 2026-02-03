#pragma once
#include<vector>

#include "Utils.h"
#include "Player.h" 
#include "Camera.h"
#include "Enemy.h"

struct Circle {
    f32 pos_x, pos_y;
    f32 r;
};

struct RectData {
    f32 pos_x, pos_y, w, h;
    f32 max, min, current, var;
};

class Game {
public:
    void Init();
    void Update();
    void Draw();
    void Free();
    bool IsRunning() const { return m_GameRunning; }

    void DealDamage(f32 damageAmount);

private:
    int m_GameRunning = 1;

    // --- Game Objects ---

    // NEW: Use the Class, not the struct
    Combat::System m_CombatSystem;
    Player m_Player;
    std::vector<std::unique_ptr<Enemy>> m_Wave1;
    std::vector<std::unique_ptr<Enemy>> m_Wave2;


    Circle m_HealCircle{ 0 };
    Circle m_DmgCircle{ 0 };
    RectData m_Healthbar{ 0 };

    // Logic Vars
    f32 m_Barcount{ 0 };
    f32 m_MinibarWidth = 100;
    u8 m_CurrentBars{ 0 };

    // Assets
    AEGfxVertexList* m_pCircleMesh = nullptr;
    AEGfxVertexList* m_pRectMesh = nullptr;
    AEGfxTexture* m_pTexBlock2 = nullptr;
    AEGfxTexture* m_pTexBlock = nullptr;

    // Camera
    Camera m_Camera;
};