#pragma once

#include "../Math/Math.hpp"

#include <array>
#include <deque>
#include <mutex>

struct CachedPlayer_t {
    bool m_bIsValid = false;
    Math::BBox_t m_bBox{};
    int m_iHealth = 0;
};

struct CachedFrame_t {
    std::array< CachedPlayer_t, 65 > m_aEntityList{};
};

class CCachedEntityList {
    std::deque< CachedFrame_t > m_qCachedFrames;
    std::mutex                  m_mAccess;

public:
    void Instance();
    bool GetCache(CachedFrame_t** frame);
};

inline auto g_pCachedEntityList = new CCachedEntityList();