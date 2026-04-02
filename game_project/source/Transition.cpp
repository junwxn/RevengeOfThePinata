/*************************************************************************
@file		Transition.cpp
@Author		Chew Zheng Hui, Timothy Caleb z.chew@digipen.edu
@Co-authors	nil
@brief		This file contains the implementation of the transition system
            mainly containing functions and pointers to the various spritesheet
            textures. Transition sheet setters and players are available here

Copyright ? 2026 DigiPen, All rights reserved.
*************************************************************************/

#include "pch.h"
#include "Transition.h"
#include "AEEngine.h"
#include <iostream>
#include "Utils.h"

namespace
{
    TransitionPhase g_TransitionPhase = TransitionPhase::TRANSITION_NONE;
    int g_PendingState = GS_NONE;

    AEGfxVertexList* g_BlackBackgroundMesh = nullptr;

    AEGfxVertexList* g_TransitionMesh = nullptr;
    AEGfxVertexList* g_Transition_Level1_Mesh = nullptr;
    AEGfxVertexList* g_Transition_Level2_Mesh = nullptr;
    AEGfxVertexList* g_Transition_Level3_Mesh = nullptr;
    AEGfxVertexList* g_Transition_BossLevel_Mesh = nullptr;

    AEGfxTexture* g_TransitionSpriteSheet = nullptr;
    AEGfxTexture* g_Transition_Level1_SpriteSheet = nullptr;
    AEGfxTexture* g_Transition_Level2_SpriteSheet = nullptr;
    AEGfxTexture* g_Transition_Level3_SpriteSheet = nullptr;
    AEGfxTexture* g_Transition_BossLevel_SpriteSheet = nullptr;

    TransitionSheet g_CurrentTransitionSheet = TransitionSheet::DEFAULT;
    AEGfxTexture* g_ActiveTransitionSpriteSheet = nullptr;
    AEGfxVertexList* g_ActiveTransitionMesh = nullptr;

    int g_CurrentFrame = 0;
    float g_FrameTimer = 0.0f;

    float g_TransitionDuration = 0.55f;
    float g_FrameDuration = 0.35f / 8.0f;

    float g_U0 = 0.0f;
    float g_V0 = 0.0f;

    bool g_SwitchReady = false;
}

static void Transition_SetSheet(TransitionSheet sheet)
{
    g_CurrentTransitionSheet = sheet;

    switch (sheet)
    {
    case TransitionSheet::LEVEL1:
        g_ActiveTransitionSpriteSheet = g_Transition_Level1_SpriteSheet;
        g_ActiveTransitionMesh = g_Transition_Level1_Mesh;
        break;

    case TransitionSheet::LEVEL2:
        g_ActiveTransitionSpriteSheet = g_Transition_Level2_SpriteSheet;
        g_ActiveTransitionMesh = g_Transition_Level2_Mesh;
        break;

    case TransitionSheet::LEVEL3:
        g_ActiveTransitionSpriteSheet = g_Transition_Level3_SpriteSheet;
        g_ActiveTransitionMesh = g_Transition_Level3_Mesh;
        break;

    case TransitionSheet::BOSSLEVEL:
        g_ActiveTransitionSpriteSheet = g_Transition_BossLevel_SpriteSheet;
        g_ActiveTransitionMesh = g_Transition_BossLevel_Mesh;
        break;

    case TransitionSheet::DEFAULT:
    default:
        g_ActiveTransitionSpriteSheet = g_TransitionSpriteSheet;
        g_ActiveTransitionMesh = g_TransitionMesh;
        break;
    }

    if (!g_ActiveTransitionSpriteSheet || !g_ActiveTransitionMesh)
    {
        g_ActiveTransitionSpriteSheet = g_TransitionSpriteSheet;
        g_ActiveTransitionMesh = g_TransitionMesh;
        g_CurrentTransitionSheet = TransitionSheet::DEFAULT;
    }
}

void Transition_Init()
{
    g_TransitionPhase = TransitionPhase::TRANSITION_NONE;
    g_PendingState = GS_NONE;

    g_CurrentFrame = 0;
    g_FrameTimer = 0.0f;
    g_SwitchReady = false;

    g_FrameDuration = g_TransitionDuration / 8.0f;
    g_U0 = 0.0f;
    g_V0 = 0.0f;

    // DEFAULT TRANSITION
    if (g_TransitionSpriteSheet)
    {
        AEGfxTextureUnload(g_TransitionSpriteSheet);
        g_TransitionSpriteSheet = nullptr;
    }

    g_TransitionSpriteSheet = AEGfxTextureLoad("Assets/Sprites/Pinata_Transition_Spritesheet_Loading.png");
    if (!g_TransitionSpriteSheet)
    {
        std::cout << "ERROR LOADING DEFAULT TRANSITION SPRITESHEET" << std::endl;
        return;
    }

    if (g_TransitionMesh)
    {
        AEGfxMeshFree(g_TransitionMesh);
        g_TransitionMesh = nullptr;
    }

    // LEVEL 1 TRANSITION
    if (g_Transition_Level1_SpriteSheet)
    {
        AEGfxTextureUnload(g_Transition_Level1_SpriteSheet);
        g_Transition_Level1_SpriteSheet = nullptr;
    }

    g_Transition_Level1_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Pinata_Transition_Spritesheet_Level1.png");
    if (!g_Transition_Level1_SpriteSheet)
    {
        std::cout << "ERROR LOADING LEVEL1 TRANSITION SPRITESHEET" << std::endl;
    }

    if (g_Transition_Level1_Mesh)
    {
        AEGfxMeshFree(g_Transition_Level1_Mesh);
        g_Transition_Level1_Mesh = nullptr;
    }

    // LEVEL 2 TRANSITION
    if (g_Transition_Level2_SpriteSheet)
    {
        AEGfxTextureUnload(g_Transition_Level2_SpriteSheet);
        g_Transition_Level2_SpriteSheet = nullptr;
    }

    g_Transition_Level2_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Pinata_Transition_Spritesheet_Level2.png");
    if (!g_Transition_Level2_SpriteSheet)
    {
        std::cout << "ERROR LOADING LEVEL2 TRANSITION SPRITESHEET" << std::endl;
    }

    if (g_Transition_Level2_Mesh)
    {
        AEGfxMeshFree(g_Transition_Level2_Mesh);
        g_Transition_Level2_Mesh = nullptr;
    }

    // LEVEL 3 TRANSITION
    if (g_Transition_Level3_SpriteSheet)
    {
        AEGfxTextureUnload(g_Transition_Level3_SpriteSheet);
        g_Transition_Level3_SpriteSheet = nullptr;
    }

    g_Transition_Level3_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Pinata_Transition_Spritesheet_Level3.png");
    if (!g_Transition_Level3_SpriteSheet)
    {
        std::cout << "ERROR LOADING LEVEL3 TRANSITION SPRITESHEET" << std::endl;
    }

    if (g_Transition_Level3_Mesh)
    {
        AEGfxMeshFree(g_Transition_Level3_Mesh);
        g_Transition_Level3_Mesh = nullptr;
    }

    // BOSS LEVEL TRANSITION
    if (g_Transition_BossLevel_SpriteSheet)
    {
        AEGfxTextureUnload(g_Transition_BossLevel_SpriteSheet);
        g_Transition_BossLevel_SpriteSheet = nullptr;
    }

    g_Transition_BossLevel_SpriteSheet = AEGfxTextureLoad("Assets/Sprites/Pinata_Transition_Spritesheet_LevelBoss.png");
    if (!g_Transition_BossLevel_SpriteSheet)
    {
        std::cout << "ERROR LOADING BOSS LEVEL TRANSITION SPRITESHEET" << std::endl;
    }

    if (g_Transition_BossLevel_Mesh)
    {
        AEGfxMeshFree(g_Transition_BossLevel_Mesh);
        g_Transition_BossLevel_Mesh = nullptr;
    }

    // BLACK BACKGROUND
    if (g_BlackBackgroundMesh)
    {
        AEGfxMeshFree(g_BlackBackgroundMesh);
        g_BlackBackgroundMesh = nullptr;
    }

    // Create meshes
    g_TransitionMesh = CreateSpriteRectMesh(0xFFFFFFFF, 8.0f, 1.0f);
    g_Transition_Level1_Mesh = CreateSpriteRectMesh(0xFFFFFFFF, 8.0f, 1.0f);
    g_Transition_Level2_Mesh = CreateSpriteRectMesh(0xFFFFFFFF, 8.0f, 1.0f);
    g_Transition_Level3_Mesh = CreateSpriteRectMesh(0xFFFFFFFF, 8.0f, 1.0f);
    g_Transition_BossLevel_Mesh = CreateSpriteRectMesh(0xFFFFFFFF, 8.0f, 1.0f);

    AEGfxMeshStart();

    AEGfxTriAdd(
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f,
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f);

    AEGfxTriAdd(
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f);

    //AEGfxTriAdd(
    //    -0.5f, 0.5f, 0x5CDF53FF, 0.0f, 0.0f,
    //    -0.5f, -0.5f, 0x5CDF53FF, 0.0f, 1.0f,
    //    0.5f, -0.5f, 0x5CDF53FF, 1.0f, 1.0f);

    //AEGfxTriAdd(
    //    -0.5f, 0.5f, 0x5CDF53FF, 0.0f, 0.0f,
    //    0.5f, -0.5f, 0x5CDF53FF, 1.0f, 1.0f,
    //    0.5f, 0.5f, 0x5CDF53FF, 1.0f, 0.0f);

    g_BlackBackgroundMesh = AEGfxMeshEnd();

    // Default active transition must be set AFTER meshes/textures are ready
    Transition_SetSheet(TransitionSheet::DEFAULT);
}

void Transition_Free()
{
    if (g_TransitionMesh) {
        AEGfxMeshFree(g_TransitionMesh);
        g_TransitionMesh = nullptr;
    }
    if (g_Transition_Level1_Mesh) {
        AEGfxMeshFree(g_Transition_Level1_Mesh);
        g_Transition_Level1_Mesh = nullptr;
    }
    if (g_Transition_Level2_Mesh) {
        AEGfxMeshFree(g_Transition_Level2_Mesh);
        g_Transition_Level2_Mesh = nullptr;
    }
    if (g_Transition_Level3_Mesh) {
        AEGfxMeshFree(g_Transition_Level3_Mesh);
        g_Transition_Level3_Mesh = nullptr;
    }
    if (g_Transition_BossLevel_Mesh) {
        AEGfxMeshFree(g_Transition_BossLevel_Mesh);
        g_Transition_BossLevel_Mesh = nullptr;
    }

    if (g_BlackBackgroundMesh) {
        AEGfxMeshFree(g_BlackBackgroundMesh);
        g_BlackBackgroundMesh = nullptr;
    }

    if (g_TransitionSpriteSheet) {
        AEGfxTextureUnload(g_TransitionSpriteSheet);
        g_TransitionSpriteSheet = nullptr;
    }
    if (g_Transition_Level1_SpriteSheet) {
        AEGfxTextureUnload(g_Transition_Level1_SpriteSheet);
        g_Transition_Level1_SpriteSheet = nullptr;
    }
    if (g_Transition_Level2_SpriteSheet) {
        AEGfxTextureUnload(g_Transition_Level2_SpriteSheet);
        g_Transition_Level2_SpriteSheet = nullptr;
    }
    if (g_Transition_Level3_SpriteSheet) {
        AEGfxTextureUnload(g_Transition_Level3_SpriteSheet);
        g_Transition_Level3_SpriteSheet = nullptr;
    }
    if (g_Transition_BossLevel_SpriteSheet) {
        AEGfxTextureUnload(g_Transition_BossLevel_SpriteSheet);
        g_Transition_BossLevel_SpriteSheet = nullptr;
    }

    g_ActiveTransitionSpriteSheet = nullptr;
    g_ActiveTransitionMesh = nullptr;
    g_CurrentTransitionSheet = TransitionSheet::DEFAULT;
}

void Transition_StartImmediate(GS_STATES nextState)
{
    g_PendingState = nextState;
    g_SwitchReady = true;
    g_TransitionPhase = TransitionPhase::TRANSITION_NONE;

    g_CurrentFrame = 0;
    g_FrameTimer = 0.0f;
    g_U0 = 0.0f;
    g_V0 = 0.0f;
}

void Transition_Start(GS_STATES nextState)
{
    Transition_Start(nextState, TransitionSheet::DEFAULT);
}

void Transition_Start(GS_STATES nextState, TransitionSheet sheet)
{
    std::cout << "Transition_Start called -> " << nextState
        << " | sheet -> " << static_cast<int>(sheet) << std::endl;

    if (g_TransitionPhase != TransitionPhase::TRANSITION_NONE)
        return;

    Transition_SetSheet(sheet);

    g_PendingState = nextState;
    g_TransitionPhase = TransitionPhase::TRANSITION_OUT;

    g_CurrentFrame = 0;
    g_FrameTimer = 0.0f;
    g_SwitchReady = false;

    g_U0 = g_CurrentFrame * (1.0f / 8.0f);
    g_V0 = 0.0f;
}

void Transition_Update(float dt)
{
    if (g_TransitionPhase == TransitionPhase::TRANSITION_NONE)
        return;

    g_FrameTimer += dt;

    while (g_FrameTimer >= g_FrameDuration)
    {
        g_FrameTimer -= g_FrameDuration;

        if (g_TransitionPhase == TransitionPhase::TRANSITION_OUT)
        {
            if (g_CurrentFrame < 7)
            {
                ++g_CurrentFrame;
                g_U0 = g_CurrentFrame * (1.0f / 8.0f);
            }

            if (g_CurrentFrame >= 7)
            {
                g_CurrentFrame = 7;
                g_U0 = g_CurrentFrame * (1.0f / 8.0f);
                g_SwitchReady = true;
                break;
            }
        }
        else if (g_TransitionPhase == TransitionPhase::TRANSITION_IN)
        {
            if (g_CurrentFrame > 0)
            {
                --g_CurrentFrame;
                g_U0 = g_CurrentFrame * (1.0f / 8.0f);
            }

            if (g_CurrentFrame <= 0)
            {
                g_CurrentFrame = 0;
                g_U0 = g_CurrentFrame * (1.0f / 8.0f);
                Transition_Reset();
                break;
            }
        }
    }
}

bool Transition_IsSwitchReady()
{
    return g_SwitchReady;
}

void Transition_BeginFadeIn()
{
    g_TransitionPhase = TransitionPhase::TRANSITION_IN;

    g_CurrentFrame = 7;
    g_FrameTimer = 0.0f;
    g_SwitchReady = false;

    g_U0 = g_CurrentFrame * (1.0f / 8.0f);
    g_V0 = 0.0f;
}

void Transition_Reset()
{
    g_TransitionPhase = TransitionPhase::TRANSITION_NONE;
    g_PendingState = GS_NONE;

    g_CurrentFrame = 0;
    g_FrameTimer = 0.0f;
    g_SwitchReady = false;

    g_U0 = 0.0f;
    g_V0 = 0.0f;

    // Reset back to default once transition is done
    Transition_SetSheet(TransitionSheet::DEFAULT);
}

bool Transition_IsActive()
{
    return g_TransitionPhase != TransitionPhase::TRANSITION_NONE;
}

int Transition_GetState()
{
    return g_PendingState;
}

void Transition_Draw()
{
    if (g_TransitionPhase == TransitionPhase::TRANSITION_NONE)
        return;

    if (!g_ActiveTransitionMesh || !g_ActiveTransitionSpriteSheet || !g_BlackBackgroundMesh)
        return;

    // Force screen-space drawing so camera from the level does not affect transition
    AEGfxSetCamPosition(0.0f, 0.0f);

    AEMtx33 scale{};
    AEMtx33 trans{};
    AEMtx33 final{};

    AEMtx33Scale(&scale,
        static_cast<float>(AEGfxGetWindowWidth()),
        static_cast<float>(AEGfxGetWindowHeight()));

    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&final, &trans, &scale);

    // =========================
    // 1) Draw colored background
    // =========================
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    switch (g_CurrentTransitionSheet)
    {
    case TransitionSheet::LEVEL1:
        AEGfxSetColorToMultiply(0.63f, 0.29f, 0.81f, 1.0f); // a04acf
        break;

    case TransitionSheet::LEVEL2:
        AEGfxSetColorToMultiply(0.93f, 0.48f, 0.10f, 1.0f); // ed7b1a
        break;

    case TransitionSheet::LEVEL3:
        AEGfxSetColorToMultiply(0.25f, 0.75f, 0.95f, 1.0f); // light blue-ish
        break;

    case TransitionSheet::BOSSLEVEL:
        AEGfxSetColorToMultiply(0.08f, 0.08f, 0.08f, 1.0f); // dark gray / near black
        break;

    case TransitionSheet::DEFAULT:
    default:
        AEGfxSetColorToMultiply(0.36f, 0.87f, 0.33f, 1.0f); // 5cdf53
        break;
    }

    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxSetTransparency(1.0f);
    AEGfxSetTransform(final.m);
    AEGfxMeshDraw(g_BlackBackgroundMesh, AE_GFX_MDM_TRIANGLES);

    // =========================
    // 2) Draw selected transition sheet
    // =========================
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    AEGfxTextureSet(g_ActiveTransitionSpriteSheet, g_U0, g_V0);
    AEGfxSetTransform(final.m);
    AEGfxMeshDraw(g_ActiveTransitionMesh, AE_GFX_MDM_TRIANGLES);

    // Reset states
    AEGfxSetTransparency(1.0f);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxSetCamPosition(0.0f, 0.0f);
}