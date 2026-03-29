#include "pch.h"
#include "Transition.h"
#include "AEEngine.h"
#include <iostream>
#include "Utils.h"

namespace
{
    TransitionPhase g_TransitionPhase = TransitionPhase::TRANSITION_NONE;
    int g_PendingState = GS_NONE;

    AEGfxVertexList* g_TransitionMesh = nullptr;
    AEGfxVertexList* g_BlackBackgroundMesh = nullptr;
    AEGfxTexture* g_TransitionSpriteSheet = nullptr;

    int g_CurrentFrame = 0;
    float g_FrameTimer = 0.0f;

    float g_TransitionDuration = 0.35f;
    float g_FrameDuration = 0.35f / 8.0f;

    float g_U0 = 0.0f;
    float g_V0 = 0.0f;

    bool g_SwitchReady = false;
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

    if (g_TransitionSpriteSheet)
    {
        AEGfxTextureUnload(g_TransitionSpriteSheet);
        g_TransitionSpriteSheet = nullptr;
    }

    g_TransitionSpriteSheet = AEGfxTextureLoad("Assets/Sprites/Pinata_Transition_SpritesheetTEST2.png");
    //g_TransitionSpriteSheet = AEGfxTextureLoad("Assets/Sprites/Pinata_Transition_SpritesheetTEST.png");
    if (!g_TransitionSpriteSheet)
    {
        std::cout << "ERROR LOADING TRANSITION SPRITESHEET" << std::endl;
        return;
    }

    if (g_TransitionMesh)
    {
        AEGfxMeshFree(g_TransitionMesh);
        g_TransitionMesh = nullptr;
    }

    if (g_BlackBackgroundMesh)
    {
        AEGfxMeshFree(g_BlackBackgroundMesh);
        g_BlackBackgroundMesh = nullptr;
    }

    // White mesh for textured transition sheet
    g_TransitionMesh = CreateSpriteRectMesh(0xFFFFFFFF, 8.0f, 1.0f);

    AEGfxMeshStart();

    AEGfxTriAdd(
        -0.5f, 0.5f, 0x00000000, 0.0f, 0.0f,
        -0.5f, -0.5f, 0x00000000, 0.0f, 1.0f,
        0.5f, -0.5f, 0x00000000, 1.0f, 1.0f);

    AEGfxTriAdd(
        -0.5f, 0.5f, 0x00000000, 0.0f, 0.0f,
        0.5f, -0.5f, 0x00000000, 1.0f, 1.0f,
        0.5f, 0.5f, 0x00000000, 1.0f, 0.0f);

    g_BlackBackgroundMesh = AEGfxMeshEnd();
}

void Transition_Free()
{
    if (g_TransitionMesh) {
        AEGfxMeshFree(g_TransitionMesh);
        g_TransitionMesh = nullptr;
    }
    if (g_BlackBackgroundMesh) {
        AEGfxMeshFree(g_BlackBackgroundMesh);
        g_BlackBackgroundMesh = nullptr;
    }
    if (g_TransitionSpriteSheet) {
        AEGfxTextureUnload(g_TransitionSpriteSheet);
        g_TransitionSpriteSheet = nullptr;
    }
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
    std::cout << "Transition_Start called -> " << nextState << std::endl;

    if (g_TransitionPhase != TransitionPhase::TRANSITION_NONE)
        return;

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

    if (!g_TransitionMesh || !g_TransitionSpriteSheet || !g_BlackBackgroundMesh)
        return;

    AEMtx33 scale{};
    AEMtx33 trans{};
    AEMtx33 final{};

    AEMtx33Scale(&scale,
        static_cast<float>(AEGfxGetWindowWidth()),
        static_cast<float>(AEGfxGetWindowHeight()));

    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&final, &trans, &scale);

    // 1) Draw solid black background
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxSetTransparency(1.0f);
    AEGfxSetTransform(final.m);
    AEGfxMeshDraw(g_BlackBackgroundMesh, AE_GFX_MDM_TRIANGLES);

    // 2) Draw the transition texture on top
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    AEGfxTextureSet(g_TransitionSpriteSheet, g_U0, g_V0);
    AEGfxSetTransform(final.m);
    AEGfxMeshDraw(g_TransitionMesh, AE_GFX_MDM_TRIANGLES);

    AEGfxSetTransparency(1.0f);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
}