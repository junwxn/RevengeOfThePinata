#include "Game.h"
#include <iostream>
#include "Camera.h"

void Game::Init() {
    // Use Utils to create meshes
    m_pCircleMesh = CreateCircleMesh(1.0f, 32, 0xFFFFFFFF);
    m_pRectMesh = CreateRectMesh(0xFFFFFFFF);

    m_pTexBlock2 = AEGfxTextureLoad("Assets/block2.png");
    m_pTexBlock = AEGfxTextureLoad("Assets/block.png");

    // Initialize the Player Class
    m_Player.Init();
    m_Enemy.Init();

    // Logic Objects
    m_HealCircle = { -400.0f, 0.0f, 150.0f };   // x, y, radius
    m_DmgCircle = { 400.0f, 0.0f, 150.0f };     // x, y, radius

    // Healthbar Init
    m_Healthbar.w = 1200;
    m_Healthbar.h = 50;
    m_Healthbar.pos_x = -m_Healthbar.w / 2;
    m_Healthbar.pos_y = 350;
    m_Healthbar.max = m_Healthbar.pos_x + m_Healthbar.w;
    m_Healthbar.min = m_Healthbar.pos_x;
    m_Healthbar.var = 100;
    m_Healthbar.current = (m_Healthbar.var / 100) * (m_Healthbar.max - m_Healthbar.min);

	// Camera Init
    m_Camera.Init(m_Player.GetX(), m_Player.GetY());

    // Augment Ball Init
    aug_ball.Init();
}

void Game::Update() {
    f32 dt = (f32)AEFrameRateControllerGetFrameTime();

    // --- 1. Update Player ---
    // The player class now handles its own Input, Movement, and Dashing
    m_Player.Update(dt, m_Enemy);
    m_Enemy.Update(dt, m_Player);

    // --- 2. Map Boundaries (Clamping) ---
    // We get the player's new position to ensure they haven't walked off the map
    f32 playerX = m_Player.GetX();
    f32 playerY = m_Player.GetY();

    float halfW = GRID_W * 0.5f;
    float halfH = GRID_H * 0.5f;

    // Convert Screen -> Grid
    float invX = playerX / halfW;
    float invY = playerY / halfH;
    float gridX = 0.5f * (invX + invY);
    float gridY = 0.5f * (invY - invX);

    // Map Limits (Matches your previous loop logic)
    const float MAP_MAX_X = 6.0f;
    const float MAP_MIN_X = -8.0f;
    const float MAP_MAX_Y = 5.0f;
    const float MAP_MIN_Y = -9.0f;

    bool clamped = false;
    if (gridX < MAP_MIN_X) { gridX = MAP_MIN_X; clamped = true; }
    if (gridX > MAP_MAX_X) { gridX = MAP_MAX_X; clamped = true; }
    if (gridY < MAP_MIN_Y) { gridY = MAP_MIN_Y; clamped = true; }
    if (gridY > MAP_MAX_Y) { gridY = MAP_MAX_Y; clamped = true; }

    // If we clamped the coordinates, convert back to screen space and update player
    if (clamped) {
        float newScreenX = (gridX - gridY) * halfW;
        float newScreenY = (gridX + gridY) * halfH;
        m_Player.SetPosition(newScreenX, newScreenY);
    }

    // --- 3. Camera ---
    // Update camera to follow the player

    m_Camera.Update(dt, m_Player.GetX(), m_Player.GetY());

    // --- 4. Game Logic (Health Circles) ---
    // We use getters for player position/size
    if (AreCirclesIntersecting(m_HealCircle.pos_x, m_HealCircle.pos_y, m_HealCircle.r, m_Player.GetX(), m_Player.GetY(), m_Player.GetSize())) {
        m_Healthbar.var += 15 * dt;
    }
    if (AreCirclesIntersecting(m_DmgCircle.pos_x, m_DmgCircle.pos_y, m_DmgCircle.r, m_Player.GetX(), m_Player.GetY(), m_Player.GetSize())) {
        m_Healthbar.var -= 15 * dt;
    }

    // Healthbar Cap
    if (m_Healthbar.var < 0) m_Healthbar.var = 0;
    if (m_Healthbar.var > 100) m_Healthbar.var = 100;

    m_Healthbar.current = (m_Healthbar.var / 100) * (m_Healthbar.max - m_Healthbar.min);
    m_Barcount = m_Healthbar.current / (m_Healthbar.w / 10);
    m_CurrentBars = (m_Barcount >= 1) ? 1 : 0;

    if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
        m_GameRunning = 0;

    // Augment Ball Interaction
    aug_ball.Interact(m_Player.GetX(), m_Player.GetY());
    aug_ball.Choosing();
}

void Game::Draw() {
    f32 dt = (f32)AEFrameRateControllerGetFrameTime();
    AESysFrameStart();
    AEGfxSetBackgroundColor(0.0f, 0.23f, 0.34f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    // --- Draw UI/Logic Elements ---
    DrawMesh(m_pCircleMesh, m_HealCircle.r, m_HealCircle.r, m_HealCircle.pos_x, m_HealCircle.pos_y, 0, 0, 255, 0, 255);
    DrawMesh(m_pCircleMesh, m_DmgCircle.r, m_DmgCircle.r, m_DmgCircle.pos_x, m_DmgCircle.pos_y, 0, 255, 0, 0, 255);

    DrawMesh(m_pRectMesh, m_Healthbar.w, m_Healthbar.h, m_Healthbar.pos_x, m_Healthbar.pos_y, 0, 255, 0, 0, 150);
    DrawMesh(m_pRectMesh, m_Healthbar.current, m_Healthbar.h, m_Healthbar.pos_x, m_Healthbar.pos_y, 0, 255, 0, 0, 255);

    // Draw Minibars
    int tempBars = m_CurrentBars;
    while (tempBars <= m_Barcount && tempBars != 0) {
        float xPos = (tempBars == 1) ? m_Healthbar.min : m_Healthbar.min + (tempBars - 1) * ((m_Healthbar.w / 10) + (((m_Healthbar.w / 10.0f) - m_MinibarWidth) / 9.0f));
        DrawMesh(m_pRectMesh, m_MinibarWidth, m_Healthbar.h, xPos, m_Healthbar.pos_y - 80, 0, 255, 0, 0, 255);
        tempBars++;
    }
    if (m_Healthbar.var != 0) {
        DrawMesh(m_pRectMesh, m_MinibarWidth, m_Healthbar.h, m_Healthbar.min, m_Healthbar.pos_y - 80, 0, 255, 0, 0, 255);
    }

    // --- Draw Map ---
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    AEGfxTextureSet(m_pTexBlock2, 0, 0);
    for (int x = 15; x > 0; --x) {
        for (int y = 15; y > 0; --y) {
            Vec2 pos = GridToScreen(x - 10, y - 10);
            AEMtx33 scale, trans, transform;
            AEMtx33Scale(&scale, SPRITE_W, SPRITE_H);
            AEMtx33Trans(&trans, pos.x, pos.y);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(m_pRectMesh, AE_GFX_MDM_TRIANGLES);
        }
    }

    // --- Draw Player ---
    m_Player.Draw();
    m_Enemy.Draw();

    // Draw Augment Ball
    aug_ball.Draw(m_Player.GetX(), m_Player.GetY(),dt);

    AESysFrameEnd();
}

void Game::Free() {
    AEGfxMeshFree(m_pCircleMesh);
    AEGfxMeshFree(m_pRectMesh);
    AEGfxTextureUnload(m_pTexBlock);
    AEGfxTextureUnload(m_pTexBlock2);

    // Free Player resources
    m_Player.Free();
    m_Enemy.Free();
}