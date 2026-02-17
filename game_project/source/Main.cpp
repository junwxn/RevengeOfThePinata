#include "pch.h"
#include "GameStateManager.h"
#include "Game.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, true, NULL);
    AESysSetWindowTitle("Revenge of the Pinata");

    f32 dt = (f32)AEFrameRateControllerGetFrameTime();

    while (current != GS_QUIT) {
        if (current == GS_RESTART) {
            current = previous;
            next = previous;
        }
        else {
            GSM_Update();
            fpLoad();
        }

        fpInitialize();

        while (current == next) {
            fpUpdate(dt);
            fpDraw();
        }

        fpFree();

        if (next != GS_RESTART) {
            fpUnload();
        }

        previous = current;
        current = next;
    }
    AESysExit();
}