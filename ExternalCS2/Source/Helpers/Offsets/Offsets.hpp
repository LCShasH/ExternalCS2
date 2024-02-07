#pragma once

#include <cstdint>

namespace Offsets {
    inline std::uintptr_t EntityList = 0;
    inline std::uintptr_t LocalPlayerController = 0;
    inline std::uintptr_t ViewMatrix = 0;

    inline std::uintptr_t iTeamNum = 0x3CB;
    inline std::uintptr_t iHealth = 0x334;
    inline std::uintptr_t m_hPawn = 0x604;
    inline std::uintptr_t GameSceneNode = 0x318;
    inline std::uintptr_t vecOrigin = 0x80;
    inline std::uintptr_t pCollision = 0x328;
    inline std::uintptr_t vecMins = 0x40;
    inline std::uintptr_t vecMaxs = 0x4C;

}