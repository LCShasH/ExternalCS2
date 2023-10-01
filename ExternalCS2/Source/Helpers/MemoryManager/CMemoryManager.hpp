#pragma once

#include <cstdint>
#include <string>
#include <Windows.h>
#include <psapi.h>
#include <memory>

#include "../Offsets/Offsets.hpp"

struct ModuleInfo_t {
    std::uintptr_t dwBaseAddress = 0;
    std::string strModulePath{};
};

class CMemoryManager {
public:
    bool Initialize();

    // RPM
    template < typename T = std::uintptr_t >
    T RPM(std::uintptr_t address) {
        T    buffer{ };
        auto status = ReadProcessMemory(m_hGameHandle, reinterpret_cast<LPVOID>(address), &buffer, sizeof(buffer), nullptr);
        return buffer;
    }

    template < typename T = std::uintptr_t >
    void RPM(std::uintptr_t address, T* buffer) {
        ReadProcessMemory(m_hGameHandle, reinterpret_cast<LPVOID>(address), static_cast<void*>(buffer), sizeof(T), nullptr);
    }

    template < typename T = std::uintptr_t >
    void WPM(std::uintptr_t address, T* buffer) {
        ReadProcessMemory(m_hGameHandle, reinterpret_cast<LPVOID>(address), static_cast<void*>(buffer), sizeof(T), nullptr);
    }

    void RPM(std::uintptr_t address, void* buffer, size_t size) const {
        auto status = ReadProcessMemory(m_hGameHandle, reinterpret_cast<LPVOID>(address), buffer, size, nullptr);
        auto ee = 1;
    }

    ModuleInfo_t GetClientInfo() { return m_ClientModule; }
    DWORD GetGamePid() const { return m_dwGamePid; }

private:
    ModuleInfo_t GetProcessModuleInfo(const char* szModuleName) const;
    void SearchGamePid(const char* szProcessName);
    std::uintptr_t PatternScan(void* module, const char* signature);

    void SetupOffsets();

    HANDLE m_hGameHandle = 0;
    DWORD m_dwGamePid = 0;

    ModuleInfo_t m_ClientModule{};
};

inline auto g_pMemoryManager = std::make_unique< CMemoryManager >();