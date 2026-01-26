#include "Player.h"
#include <math.h> // For sqrt

void Player::Init()
{
    // Initialize standard values
    m_PosX = 0.0f;
    m_PosY = 0.0f;
    m_Speed = 300.0f;
    m_Size = 40.0f;
    m_DashCooldown_Default = 0.1f;
    m_DashCooldown = 0.1f;

    // Create a local mesh for the player
    // (Assuming CreateCircleMesh is defined in Utils.h/cpp)
    m_pMesh = CreateCircleMesh(1.0f, 32, 0x50A655);
}

void Player::Update(float dt)
{
    // --- 1. Update Timers ---
    if (m_DashCooldown > 0.0f)
        m_DashCooldown -= dt;

    // --- 2. Gather Input ---
    s8 moveX = 0;
    s8 moveY = 0;

    if (AEInputCheckCurr(AEVK_W)) moveY += 1;
    if (AEInputCheckCurr(AEVK_S)) moveY -= 1;
    if (AEInputCheckCurr(AEVK_A)) moveX -= 1;
    if (AEInputCheckCurr(AEVK_D)) moveX += 1;

    // --- 3. Execute Movement Logic ---
    if (moveX != 0 || moveY != 0)
    {
        float dirX = 0.0f;
        float dirY = 0.0f;

        // Determine Direction
        if (moveX != 0 && moveY != 0)
        {
            // Diagonal Normalization
            float halfW = GRID_W * 0.5f;
            float halfH = GRID_H * 0.5f;
            float length = sqrt(halfW * halfW + halfH * halfH);

            float isoStepX = halfW / length;
            float isoStepY = halfH / length;

            dirX = (moveX > 0 ? isoStepX : -isoStepX);
            dirY = (moveY > 0 ? isoStepY : -isoStepY);
        }
        else
        {
            // Orthogonal Normalization
            dirX = (float)moveX;
            dirY = (float)moveY;
        }

        // --- 4. Dash Logic ---
        if (AEInputCheckTriggered(AEVK_SPACE) && m_DashCooldown <= 0.0f)
        {
            float blinkDist = 0.0f;

            if (moveX != 0 && moveY != 0)
            {
                // Diagonal: Move Hypotenuse of one tile
                float halfW = GRID_W * 0.5f;
                float halfH = GRID_H * 0.5f;
                blinkDist = sqrt(halfW * halfW + halfH * halfH);
            }
            else
            {
                // Orthogonal: Move Width or Height
                blinkDist = (moveX != 0) ? GRID_W : GRID_H;
            }

            // Apply Dash Position
            m_PosX += dirX * blinkDist;
            m_PosY += dirY * blinkDist;

            m_DashCooldown = m_DashCooldown_Default;
        }

        // --- 5. Apply Velocity ---
        m_PosX += dirX * m_Speed * dt;
        m_PosY += dirY * m_Speed * dt;
    }
}

void Player::Draw()
{
    // Ensure Color Mode is set
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    

    // Calculate isometric squashed height for drawing
    float isoHeight = m_Size * (GRID_H / GRID_W);

    // Draw using Utils helper
    // Color: Black (0,0,0) with full alpha (255)
    DrawMesh(m_pMesh, m_Size, isoHeight, m_PosX, m_PosY, 0.0f, 44, 145, 57, 255);
}

void Player::Free()
{
    if (m_pMesh)
        AEGfxMeshFree(m_pMesh);
}