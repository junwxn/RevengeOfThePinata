/*************************************************************************
@file		Main.cpp
@Author		Chiu Jun Wen j.chiu@digipen.edu
@Co-authors	nil
@brief		This file contains the entry point and main loop for the game,
            including initialization, updating, rendering, and cleanup.

Copyright � 2026 DigiPen, All rights reserved.
*************************************************************************/
#include "pch.h"
#include "Audio.h"
#include "GameStateManager.h"
#include "GameStateList.h"
#include "Transition.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    AESysInit(hInstance, nCmdShow, 1600, 900, 0, 60, true, NULL);
    AESysSetWindowTitle("Revenge of the Pinata");

    // Set custom window icon from embedded resource
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    if (hIcon) {
        HWND hwnd = AESysGetWindowHandle();
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }

    gAudio.Audio_Init();

    GSM_Initialize(GS_SPLASH);

    while (AESysDoesWindowExist() && current != GS_QUIT)
    {
        AESysFrameStart();

        float dt = static_cast<float>(AEFrameRateControllerGetFrameTime());

        GSM_Update(dt);
        GSM_Draw();

        AESysFrameEnd();
    }

    if (fpFree)   fpFree();
    if (fpUnload) fpUnload();

    Transition_Free();

    gAudio.UnloadBGM();
    gAudio.UnloadCombatSFX();
    gAudio.UnloadEnemySFX();
    gAudio.UnloadPlayerSFX();
    gAudio.UnloadGeneralSFX();
    gAudio.UnloadFireworksSFX();

    AESysExit();
    return 0;
}