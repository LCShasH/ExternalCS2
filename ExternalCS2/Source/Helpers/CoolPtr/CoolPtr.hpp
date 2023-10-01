#pragma once

#include <cstdint>
#include <array>
#include "../MemoryManager/CMemoryManager.hpp"

class GeniusPtr {
    std::array< uint8_t, 0x2088 > m_pBuffer;
    std::size_t m_uiSize = 0;
    std::size_t m_uiBase = 0;
public:
    GeniusPtr(std::uintptr_t uiBase, std::size_t uiSize) : m_uiSize(uiSize), m_uiBase(uiBase) {
        g_pMemoryManager->RPM(uiBase, m_pBuffer.data(), uiSize);
    }

    bool IsValid() const
    {
        return m_uiBase;
    }

    template<class T>
    T Get(std::uintptr_t uiOffset) {
        if (uiOffset > m_uiSize)
            return T{};

        return *reinterpret_cast<T*>(reinterpret_cast<std::uintptr_t>(m_pBuffer.data()) + uiOffset);
    }
};