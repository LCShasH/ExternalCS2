#include "CMemoryManager.hpp"

#include <TlHelp32.h>
#include <vector>

bool CMemoryManager::Initialize() {
    SearchGamePid("cs2.exe");

    if (!m_dwGamePid) {
        printf("[x] Failed to get game PID");
        return false;
    }

    m_hGameHandle = OpenProcess(0x1FFFFFu, 0, m_dwGamePid);

    if (m_hGameHandle == INVALID_HANDLE_VALUE) {
        printf("[x] Failed to open game process\n");
        return false;
    }

    m_ClientModule = GetProcessModuleInfo("client.dll");

    if (!m_ClientModule.dwBaseAddress) {
        printf("[x] Failed to get client.dll module\n");
        return false;
    }

    SetupOffsets();
}

ModuleInfo_t CMemoryManager::GetProcessModuleInfo(const char* szModuleName) const
{
    const HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, m_dwGamePid);
    std::uintptr_t dwModuleBaseAddress = 0;
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 ModuleEntry32 = { 0 };
        ModuleEntry32.dwSize = sizeof(MODULEENTRY32);
        if (Module32First(hSnapshot, &ModuleEntry32))
        {
            do
            {
                if (strcmp(ModuleEntry32.szModule, szModuleName) == 0)
                {
                    dwModuleBaseAddress = reinterpret_cast<std::uintptr_t>(ModuleEntry32.modBaseAddr);
                    break;
                }
            } while (Module32Next(hSnapshot, &ModuleEntry32));
        }
        CloseHandle(hSnapshot);
    }

    char pBuffer[MAX_PATH + 1];
    K32GetModuleFileNameExA(m_hGameHandle, reinterpret_cast<HMODULE>(dwModuleBaseAddress), pBuffer, MAX_PATH);

    return ModuleInfo_t{ dwModuleBaseAddress, pBuffer };
}

void CMemoryManager::SearchGamePid(const char* szProcessName) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (strcmp(entry.szExeFile, szProcessName) == 0)
            {
                m_dwGamePid = entry.th32ProcessID;
                break;
            }
        }
    }

    CloseHandle(snapshot);
}

std::uintptr_t CMemoryManager::PatternScan(void* module, const char* signature)
{
    static auto pattern_to_byte = [](const char* pattern) {
        auto bytes = std::vector<int>{};
        auto start = const_cast<char*>(pattern);
        auto end = const_cast<char*>(pattern) + strlen(pattern);

        for (auto current = start; current < end; ++current) {
            if (*current == '?') {
                ++current;
                if (*current == '?')
                    ++current;
                bytes.push_back(-1);
            }
            else {
                bytes.push_back(strtoul(current, &current, 16));
            }
        }
        return bytes;
        };

    auto dosHeader = (PIMAGE_DOS_HEADER)module;
    auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)module + dosHeader->e_lfanew);

    auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
    auto patternBytes = pattern_to_byte(signature);
    auto scanBytes = reinterpret_cast<std::uint8_t*>(module);

    auto s = patternBytes.size();
    auto d = patternBytes.data();

    for (auto i = 0ul; i < sizeOfImage - s; ++i) {
        bool found = true;
        for (auto j = 0ul; j < s; ++j) {
            if (scanBytes[i + j] != d[j] && d[j] != -1) {
                found = false;
                break;
            }
        }
        if (found) {
            return (std::uintptr_t) & scanBytes[i];
        }
    }
    return 0;
}

std::uintptr_t GetDwordOffset(std::uintptr_t uiAddress, std::uintptr_t uiPreOffset, std::uintptr_t uiPostOffset, std::uintptr_t pBaseAddress) noexcept {
    const auto pCallOffset = *reinterpret_cast<std::int32_t*>(uiAddress + 3 + uiPreOffset);
    return pCallOffset + ((uiAddress + uiPostOffset) - pBaseAddress);
}

std::uintptr_t GetCallAddress(std::uintptr_t uiAddress) noexcept {
    const auto pCallOffset = *reinterpret_cast<std::int32_t*>(uiAddress + 1);
    return pCallOffset + uiAddress + 5;
}

void CMemoryManager::SetupOffsets() {
    // BB ? ? ? ? 83 FF 01 0F 8C ? ? ? ? 3B F8
    auto hClientDll = LoadLibraryExA(m_ClientModule.strModulePath.c_str(), 0, DONT_RESOLVE_DLL_REFERENCES);

    /*
    .text:0000000180564ED9 FF 90 E0 03 00 00                       call    qword ptr [rax+3E0h]
    .text:0000000180564EDF 84 C0                                   test    al, al
    .text:0000000180564EE1 74 35                                   jz      short loc_180564F18
    .text:0000000180564EE3 48 8B 0D 3E 62 27 01                    mov     rcx, cs:qword_1817DA118 <- what we need
    .text:0000000180564EEA 48 85 C9                                test    rcx, rcx
    */
    Offsets::LocalPlayerController = GetDwordOffset(GetCallAddress(PatternScan(hClientDll, "48 8B 1D ?? ?? ?? ?? 48 8B 6C 24 30") + 2), 0, 0, reinterpret_cast<std::uintptr_t>(hClientDll));

    /*
    .text:0000000180232220 48 63 C2                                movsxd  rax, edx
    .text:0000000180232223 48 8D 0D D6 84 64 01                    lea     rcx, unk_1818796f0 <- what we need
    .text:000000018023222A 48 C1 E0 06                             shl     rax, 6
    .text:000000018023222E 48 03 C1                                add     rax, rcx
    */
    const auto pGameMatrixAddr = PatternScan(hClientDll, "48 63 C2 48 8D 0D ? ? ? ? 48 C1 E0 06");
    Offsets::ViewMatrix = GetDwordOffset(pGameMatrixAddr, 0x3, 0xA, reinterpret_cast<std::uintptr_t>(hClientDll));

    /*
    .text:0000000180D85241 48 8B 0D 60 76 A0 00                    mov     rcx, cs:qword_18178b898 <- what we need
    .text:0000000180D85248 41 8B D0                                mov     edx, r8d
    .text:0000000180D8524B 41 C1 E8 0E                             shr     r8d, 0Eh
    .text:0000000180D8524F 81 E2 FF 3F 00 00                       and     edx, 3FFFh
    .text:0000000180D85255 41 81 E0 FF 03 00 00                    and     r8d, 3FFh
    */

    const auto pGameEntityListAddr = PatternScan(hClientDll, "48 83 EC 28 4C 8B 01") + 0x41;
    Offsets::EntityList = GetDwordOffset(pGameEntityListAddr, 0, 0x7, reinterpret_cast<std::uintptr_t>(hClientDll));

    FreeLibrary(hClientDll);
}
