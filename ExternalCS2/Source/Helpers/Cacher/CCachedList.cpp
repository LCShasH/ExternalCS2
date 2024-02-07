#include "CCachedList.hpp"

#include "../MemoryManager/CMemoryManager.hpp"
#include "../CoolPtr/CoolPtr.hpp"
#include "../Globals.hpp"

#include <vector>

class CGameEntitySystem {
public:

    template < class tClass = std::uintptr_t >
    tClass GetPlayerController(const std::int32_t index) {
        static std::uintptr_t ppAddr = g_pMemoryManager->GetClientInfo().dwBaseAddress + Offsets::EntityList;

        std::uintptr_t entityList = g_pMemoryManager->RPM(ppAddr);
        std::uintptr_t listEntry = g_pMemoryManager->RPM(entityList + (8 * (index & 0x7FFF) >> 9) + 16);
        std::uintptr_t controllerBase = g_pMemoryManager->RPM(listEntry + 0x78 * (index & 0x1FF));

        return static_cast<tClass>(controllerBase);
    }

    template < class tClass = std::uintptr_t>
    tClass GetEntityFromHandle(const std::uint32_t uiHandle) {
        static std::uintptr_t ppAddr = g_pMemoryManager->GetClientInfo().dwBaseAddress + Offsets::EntityList;

        std::uintptr_t entityList = g_pMemoryManager->RPM(ppAddr);
        std::uintptr_t listEntry2 = g_pMemoryManager->RPM(entityList + 0x8 * ((uiHandle & 0x7FFF) >> 9) + 16);

        return static_cast<tClass>(g_pMemoryManager->RPM(listEntry2 + 0x78 * (uiHandle & 0x1FF)));
    }
};

CGameEntitySystem* pEntitySystem = new CGameEntitySystem();

void CCachedEntityList::Instance() {
    if (nGlobals::IsUnloading)
        return;

    m_mAccess.lock();
    const auto iSize = m_qCachedFrames.size();
    if (iSize > 5) {
        m_qCachedFrames.erase(m_qCachedFrames.begin(), m_qCachedFrames.begin() + (iSize - 3));
    }
    const auto cachedFrame = &m_qCachedFrames.emplace_back();
    m_mAccess.unlock();

    if (!cachedFrame)
        return;

    const auto clientBase = g_pMemoryManager->GetClientInfo().dwBaseAddress;

    if (!clientBase)
        return;

    auto pLocalPlayerController = GeniusPtr(clientBase + Offsets::LocalPlayerController, 0x848);

    if (!pLocalPlayerController.IsValid())
        return;

    auto pLocalPlayerPawn = GeniusPtr(pEntitySystem->GetEntityFromHandle(pLocalPlayerController.Get<std::uint32_t>(Offsets::m_hPawn)), 0x1238);

    if (!pLocalPlayerPawn.IsValid())
        return;

    const auto iLocalTeam = pLocalPlayerPawn.Get<std::int32_t>(Offsets::iTeamNum);

    const auto pViewMatrix = g_pMemoryManager->RPM< VMatrix >(clientBase + Offsets::ViewMatrix);

    for (int iEntityIdx = 1; iEntityIdx < 64; iEntityIdx++) {
        auto pPlayer = GeniusPtr(pEntitySystem->GetPlayerController(iEntityIdx), 0x848);

        if (!pPlayer.IsValid())
            continue;

        auto pEntityPawn = GeniusPtr(pEntitySystem->GetEntityFromHandle(pPlayer.Get<std::uint32_t>(Offsets::m_hPawn)), 0x1238);

        if (!pEntityPawn.IsValid()) 
            continue;

        const auto iHealth = pEntityPawn.Get<std::int32_t>(Offsets::iHealth);

        if (iHealth <= 0)
            continue;

        const auto iTeamNum = pEntityPawn.Get<std::int32_t>(Offsets::iTeamNum);

        if (iTeamNum == iLocalTeam)
            continue;

        auto pGameSceneNode = GeniusPtr(pEntityPawn.Get<std::uintptr_t>(Offsets::GameSceneNode), 0x150);

        if (!pGameSceneNode.IsValid())
            continue;

        auto pCollision = GeniusPtr(pEntityPawn.Get<std::uintptr_t>(Offsets::pCollision), 0xB0);

        if (!pCollision.IsValid())
            continue;

        const auto vecMins = pCollision.Get< Vector >(Offsets::vecMins);
        const auto vecMaxs = pCollision.Get< Vector >(Offsets::vecMaxs);
        const auto vecOrigin = pGameSceneNode.Get< Vector >(Offsets::vecOrigin);

        //printf("[%i/64]has %i hp\n", iEntityIdx, iHealth);

        auto& cCachedEntity = cachedFrame->m_aEntityList[iEntityIdx];

        Math::BBox_t bBox;
        cCachedEntity.m_bIsValid = Math::GetBoundingBox(vecOrigin, vecMins, vecMaxs, pViewMatrix, cCachedEntity.m_bBox);
        cCachedEntity.m_iHealth = iHealth;
    }
}

bool CCachedEntityList::GetCache(CachedFrame_t** frame) {
    m_mAccess.lock();
    const auto iSize = m_qCachedFrames.size();
    if (iSize <= 1) {
        m_mAccess.unlock();
        return false;
    }

    *frame = &m_qCachedFrames[iSize - 2];

    // Deleting all old frames
    if (iSize > 5) {
        m_qCachedFrames.erase(m_qCachedFrames.begin(), m_qCachedFrames.begin() + (iSize - 3));
    }
    m_mAccess.unlock();

    return true;
}
