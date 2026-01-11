#include "Game.h"

void Game::Init() {
    // Use Utils to create meshes
    m_pCircleMesh = CreateCircleMesh(1.0f, 32, 0xFFFFFFFF);
    m_pRectMesh = CreateRectMesh(0xFFFFFFFF);

    m_pTexBlock2 = AEGfxTextureLoad("Assets/block2.png");
    m_pTexBlock = AEGfxTextureLoad("Assets/block.png");

    m_Player = { 0, 0, 200.0f, 40.0f };         // x, y, speed, size
    m_HealCircle = { -400.0f, 0.0f, 150.0f };   // x, y, radius
    m_DmgCircle = { 400.0f, 0.0f, 150.0f };     // x, y, radius

    m_Healthbar.w = 1200;
    m_Healthbar.h = 50;
    m_Healthbar.pos_x = -m_Healthbar.w / 2;
    m_Healthbar.pos_y = 350;
    m_Healthbar.max = m_Healthbar.pos_x + m_Healthbar.w;
    m_Healthbar.min = m_Healthbar.pos_x;
    m_Healthbar.var = 100;
    m_Healthbar.current = (m_Healthbar.var / 100) * (m_Healthbar.max - m_Healthbar.min);
}

void Game::Update() {
    f32 dt = (f32)AEFrameRateControllerGetFrameTime();

    // Input
    if (AEInputCheckCurr(AEVK_W)) m_Player.pos_y += m_Player.speed * dt;
    if (AEInputCheckCurr(AEVK_A)) m_Player.pos_x -= m_Player.speed * dt;
    if (AEInputCheckCurr(AEVK_S)) m_Player.pos_y -= m_Player.speed * dt;
    if (AEInputCheckCurr(AEVK_D)) m_Player.pos_x += m_Player.speed * dt;

    // Bounds
    float winW = (float)AEGfxGetWindowWidth();
    float winH = (float)AEGfxGetWindowHeight();
    if (m_Player.pos_x - m_Player.size < -winW / 2) m_Player.pos_x = -winW / 2 + m_Player.size;
    if (m_Player.pos_x + m_Player.size > winW / 2)  m_Player.pos_x = winW / 2 - m_Player.size;
    if (m_Player.pos_y - m_Player.size < -winH / 2) m_Player.pos_y = -winH / 2 + m_Player.size;
    if (m_Player.pos_y + m_Player.size > winH / 2)  m_Player.pos_y = winH / 2 - m_Player.size;

    // Logic using Utils function
    if (AreCirclesIntersecting(m_HealCircle.pos_x, m_HealCircle.pos_y, m_HealCircle.r, m_Player.pos_x, m_Player.pos_y, m_Player.size)) {
        m_Healthbar.var += 15 * dt;
    }
    if (AreCirclesIntersecting(m_DmgCircle.pos_x, m_DmgCircle.pos_y, m_DmgCircle.r, m_Player.pos_x, m_Player.pos_y, m_Player.size)) {
        m_Healthbar.var -= 15 * dt;
    }

    if (m_Healthbar.var < 0) m_Healthbar.var = 0;
    if (m_Healthbar.var > 100) m_Healthbar.var = 100;

    m_Healthbar.current = (m_Healthbar.var / 100) * (m_Healthbar.max - m_Healthbar.min);
    m_Barcount = m_Healthbar.current / (m_Healthbar.w / 10);
    m_CurrentBars = (m_Barcount >= 1) ? 1 : 0;

    if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
        m_GameRunning = 0;
}

void Game::Draw() {
    AESysFrameStart();
    AEGfxSetBackgroundColor(0.85f, 0.9f, 0.86f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    // Using generic DrawMesh from Utils
    // Heal Circle
    DrawMesh(m_pCircleMesh, m_HealCircle.r, m_HealCircle.r, m_HealCircle.pos_x, m_HealCircle.pos_y, 0, 0, 255, 0, 255);
    // Dmg Circle
    DrawMesh(m_pCircleMesh, m_DmgCircle.r, m_DmgCircle.r, m_DmgCircle.pos_x, m_DmgCircle.pos_y, 0, 255, 0, 0, 255);

    // Healthbar BG
    DrawMesh(m_pRectMesh, m_Healthbar.w, m_Healthbar.h, m_Healthbar.pos_x, m_Healthbar.pos_y, 0, 255, 0, 0, 150);
    // Healthbar Fill
    DrawMesh(m_pRectMesh, m_Healthbar.current, m_Healthbar.h, m_Healthbar.pos_x, m_Healthbar.pos_y, 0, 255, 0, 0, 255);

    // Minibars
    int tempBars = m_CurrentBars;
    while (tempBars <= m_Barcount && tempBars != 0) {
        float xPos = (tempBars == 1) ? m_Healthbar.min : m_Healthbar.min + (tempBars - 1) * ((m_Healthbar.w / 10) + (((m_Healthbar.w / 10.0f) - m_MinibarWidth) / 9.0f));
        DrawMesh(m_pRectMesh, m_MinibarWidth, m_Healthbar.h, xPos, m_Healthbar.pos_y - 80, 0, 255, 0, 0, 255);
        tempBars++;
    }
    if (m_Healthbar.var != 0) {
        DrawMesh(m_pRectMesh, m_MinibarWidth, m_Healthbar.h, m_Healthbar.min, m_Healthbar.pos_y - 80, 0, 255, 0, 0, 255);
    }

    // ISOMETRIC MAP
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    AEGfxTextureSet(m_pTexBlock2, 0, 0);
    for (int x = 15; x > 0; --x) {
        for (int y = 15; y > 0; --y) {
            Vec2 pos = GridToScreen(x - 10, y - 10);

            // For texture drawing, we manually transform because we want to use generic logic or specific texture logic
            AEMtx33 scale, trans, transform;
            AEMtx33Scale(&scale, SPRITE_W, SPRITE_H);
            AEMtx33Trans(&trans, pos.x, pos.y);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(m_pRectMesh, AE_GFX_MDM_TRIANGLES);
        }
    }

    AEGfxTextureSet(m_pTexBlock, 0, 0);
    for (int x = 10; x > 0; --x) {
        for (int y = 10; y > 0; --y) {
            Vec2 pos = GridToScreen(x - 6, y - 6);

            AEMtx33 scale, trans, transform;
            AEMtx33Scale(&scale, SPRITE_W, SPRITE_H);
            AEMtx33Trans(&trans, pos.x, pos.y);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(m_pRectMesh, AE_GFX_MDM_TRIANGLES);
        }
    }

    AEGfxTextureSet(m_pTexBlock2, 0, 0);
    for (int x = 5; x > 0; --x) {
        for (int y = 5; y > 0; --y) {
            Vec2 pos = GridToScreen(x - 2, y - 2);

            AEMtx33 scale, trans, transform;
            AEMtx33Scale(&scale, SPRITE_W, SPRITE_H);
            AEMtx33Trans(&trans, pos.x, pos.y);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(m_pRectMesh, AE_GFX_MDM_TRIANGLES);
        }
    }

    // Player
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    DrawMesh(m_pCircleMesh, m_Player.size, m_Player.size, m_Player.pos_x, m_Player.pos_y, 0, 0, 0, 255, 255);

    AESysFrameEnd();
}

void Game::Free() {
    AEGfxMeshFree(m_pCircleMesh);
    AEGfxMeshFree(m_pRectMesh);
    AEGfxTextureUnload(m_pTexBlock);
    AEGfxTextureUnload(m_pTexBlock2);
}