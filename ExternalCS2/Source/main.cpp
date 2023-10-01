#include <iostream>
#include <Windows.h>
#include <vector>

#include "Helpers/MemoryManager/CMemoryManager.hpp"
#include "Helpers/Overlay/Overlay.h"
#include "Helpers/Math/Math.hpp"
#include "Helpers/CoolPtr/CoolPtr.hpp"
#include "Helpers/Cacher/CCachedList.hpp"

#include "Helpers/Globals.hpp"

DWORD WINAPI CachedEntityListThread(LPVOID pParameter) {
    while (true)
        g_pCachedEntityList->Instance();

    return 1;
}

int main() {
    if (!g_pMemoryManager->Initialize()) {
        printf("[x] Failed to initialize memory system\n");
        return -1;
    }

    const auto dwPid = g_pMemoryManager->GetGamePid();

    const HWND hWnd = FindWindowA(0, "Counter-Strike 2");

    if(!hWnd) {
        printf("[x] Failed to get game window\n");
        return -1;
    }

    RECT rcClientRect;
    GetClientRect(hWnd, &rcClientRect);

    Math::g_Width = rcClientRect.right;
    Math::g_Height = rcClientRect.bottom;

    printf("[+] All is fine. Starting...\n");
    printf(" -- Panic key -> DELETE\n");

    const auto hCachedEntityListThread = CreateThread(nullptr, 0, CachedEntityListThread, nullptr, 0, nullptr);

    Overlay overlay(dwPid, 1000);

    while (!(GetAsyncKeyState(VK_DELETE) & 1)) {

        if (!overlay.BeginFrame())
            continue;

        CachedFrame_t* cCachedEntities{};
        if (!g_pCachedEntityList->GetCache(&cCachedEntities)) {
            overlay.EndFrame();
            continue;
        }

        if (!cCachedEntities) {
            overlay.EndFrame();
            continue;
        }

        for (const auto vecEntityList = cCachedEntities->m_aEntityList; const auto & cEntity : vecEntityList) {
            if (!cEntity.m_bIsValid)
                continue;

            const auto bBox = cEntity.m_bBox;
            const auto iHealth = cEntity.m_iHealth;

            // ESP box
            overlay.DrawRect( bBox.x - 1, bBox.y - 1, bBox.w + 1, bBox.h + 1, 1, 0xFF000000);
            overlay.DrawRect (bBox.x, bBox.y, bBox.w, bBox.h, 1, 0xFFFFFFFF);
            overlay.DrawRect( bBox.x + 1, bBox.y + 1, bBox.w - 1, bBox.h - 1, 1, 0xFF000000);

            // Health bar
            // Bar
            const auto flHealthPercent = iHealth / 100.f;
            const auto flHeight = std::abs(bBox.h - bBox.y);

            // Premium skeet.cc healthbar ( why not :) )
            auto color = 0;
            if (iHealth >= 25) {
                color = 0xA0D7C850;
                if (iHealth >= 50)
                    color = 0xA050FF50;
            }
            else
                color = 0xA0FF3250;

            overlay.DrawFilledRect(bBox.x - 5, bBox.y - 1, bBox.x - 3, bBox.h + 1, 0xFF000000);

            overlay.DrawFilledRect(bBox.x - 5, bBox.h - (flHeight * flHealthPercent), bBox.x - 3, bBox.h + 1, color);


            // Outlined rect
            overlay.DrawRect( bBox.x - 6, bBox.y - 1, bBox.x - 3, bBox.h + 1, 1, 0xFF000000);
        }
        overlay.EndFrame();
    }

    nGlobals::IsUnloading = true;

    Sleep(100);

    CloseHandle(hCachedEntityListThread);
    return 0;
}