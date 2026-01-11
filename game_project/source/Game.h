#pragma once
#include "Utils.h" // Include our new Utils file

struct Player {
    f32 pos_x, pos_y;
    f32 speed;
    f32 size;
};

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

private:
    int m_GameRunning = 1;

    // Game Objects
    Player m_Player;
    Circle m_HealCircle;
    Circle m_DmgCircle;
    RectData m_Healthbar;

    // Logic Vars
    f32 m_Barcount;
    f32 m_MinibarWidth = 100;
    u8 m_CurrentBars = 0;

    // Assets
    AEGfxVertexList* m_pCircleMesh = nullptr;
    AEGfxVertexList* m_pRectMesh = nullptr;
    AEGfxTexture* m_pTexBlock2 = nullptr;
    AEGfxTexture* m_pTexBlock = nullptr;
};