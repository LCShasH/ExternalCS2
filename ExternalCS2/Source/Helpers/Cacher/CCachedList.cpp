#include "CCachedList.hpp"

#include "../MemoryManager/CMemoryManager.hpp"
#include "../CoolPtr/CoolPtr.hpp"
#include "../Globals.hpp"

#include <vector>

constexpr auto MAX_ENTITIES_IN_LIST = 512;
constexpr auto MAX_ENTITY_LISTS = 64; // 0x3F
constexpr auto MAX_TOTAL_ENTITIES = MAX_ENTITIES_IN_LIST * MAX_ENTITY_LISTS; // 0x8000

class CEntityIdentity {
public:
    void* m_pEntity;
    void* dunno;
    int                     entHandle; // LOWORD(handle) & 0x7FFF = entID
    int                     m_nameStringableIndex; // always seems to be -1
    const char* internalName; // these two strings are optional!
    const char* m_designerName; // ex: item_tpscroll
private:
    std::uint8_t __pad0028[0x8]; // 0x28
public:
    std::uint32_t m_flags; // 0x30
private:
    std::uint8_t __pad0034[0x4]; // 0x34
public:
    int           m_worldGroupId; // 0x38
    std::uint32_t m_fDataObjectTypes; // 0x3c
    short         m_PathIndex; // 0x40
private:
    std::uint8_t __pad0042[0x16]; // 0x42
public:
    CEntityIdentity* m_pPrev; // 0x58
    CEntityIdentity* m_pNext; // 0x60
    CEntityIdentity* m_pPrevByClass; // 0x68
    CEntityIdentity* m_pNextByClass; // 0x70
}; // sizeof == 0x78

class CEntityIdentities {
public:
    CEntityIdentity m_pIdentities[MAX_ENTITY_LISTS][MAX_ENTITIES_IN_LIST];
};

inline auto g_pLocalIdentityChunks = new CEntityIdentities();

class CGameEntitySystem {
    char         pad[16];
    CEntityIdentities* m_pIdentityChunks;
public:
    void Update()
    {
        return;
        static std::uintptr_t ppAddr = g_pMemoryManager->GetClientInfo().dwBaseAddress + Offsets::EntityList;
        const auto            pAddr = g_pMemoryManager->RPM< std::uintptr_t >(ppAddr);

        g_pMemoryManager->RPM< CGameEntitySystem >(pAddr, this);
        g_pMemoryManager->RPM< CEntityIdentities >(reinterpret_cast<std::uintptr_t>(this->m_pIdentityChunks), g_pLocalIdentityChunks);
    }

    template < class tClass = std::uintptr_t >
    tClass GetPlayerController(const std::int32_t index) {
        static std::uintptr_t ppAddr = g_pMemoryManager->GetClientInfo().dwBaseAddress + Offsets::EntityList;
        std::uintptr_t entityList = g_pMemoryManager->RPM(ppAddr);
        // ENTITY_SPACING = 0x78
        // i goes from 1 to 64 in a loop
        std::uintptr_t listEntry = g_pMemoryManager->RPM(entityList + (8 * (index & 0x7FFF) >> 9) + 16);
        // CCSPlayerController 
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

    const auto pLocalPlayerPawn = g_pMemoryManager->RPM(clientBase + Offsets::LocalPlayerPawn);

    if (!pLocalPlayerPawn)
        return;

    const auto iLocalTeam = g_pMemoryManager->RPM<std::int32_t>(pLocalPlayerPawn + Offsets::iTeamNum);

    const auto pViewMatrix = g_pMemoryManager->RPM< VMatrix >(clientBase + Offsets::ViewMatrix);

    pEntitySystem->Update();

    for (int iEntityIdx = 1; iEntityIdx < 64; iEntityIdx++) {
        auto pPlayer = GeniusPtr(pEntitySystem->GetPlayerController(iEntityIdx), 0x848);

        if (!pPlayer.IsValid())
            continue;

        const auto bPawnIsAlive = pPlayer.Get<bool>(Offsets::bPawnIsAlive);

        if (!bPawnIsAlive)
            continue;

        auto pEntityPawn = GeniusPtr(pEntitySystem->GetEntityFromHandle(pPlayer.Get<std::uint32_t>(Offsets::m_hPawn)), 0x1238);

        if (!pEntityPawn.IsValid()) 
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

        const auto iHealth = pPlayer.Get<std::uint32_t>(Offsets::iHealth);

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
