#pragma once

#include <cstdint>

namespace Offsets {
    inline std::uintptr_t EntityList = 0;
    inline std::uintptr_t LocalPlayerPawn = 0;
    inline std::uintptr_t ViewMatrix = 0;

    inline std::uintptr_t iTeamNum = 0x3Bf;
    inline std::uintptr_t iHealth = 0x808;
    inline std::uintptr_t bPawnIsAlive = 0x804;
    inline std::uintptr_t m_hPawn = 0x5dc;
    inline std::uintptr_t GameSceneNode = 0x310;
    inline std::uintptr_t vecOrigin = 0xC8;
    inline std::uintptr_t pCollision = 0x320;
    inline std::uintptr_t vecMins = 0x40;
    inline std::uintptr_t vecMaxs = 0x4C;

}