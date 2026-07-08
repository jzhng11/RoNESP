#include "memory.h"

Memory::~Memory() {
    Close();
}

bool Memory::FindProcess(const std::wstring& name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    bool found = false;
    if (Process32FirstW(snap, &entry)) {
        do {
            if (name == entry.szExeFile) {
                m_ProcessId = entry.th32ProcessID;
                m_ProcessHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, m_ProcessId);
                found = (m_ProcessHandle != nullptr);
                break;
            }
        } while (Process32NextW(snap, &entry));
    }

    CloseHandle(snap);
    return found;
}

void Memory::Close() {
    if (m_ProcessHandle) {
        CloseHandle(m_ProcessHandle);
        m_ProcessHandle = nullptr;
    }
    m_ProcessId = 0;
}

uintptr_t Memory::GetModuleBase(const std::wstring& name) const {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_ProcessId);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;

    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    uintptr_t base = 0;
    if (Module32FirstW(snap, &entry)) {
        do {
            if (name == entry.szModule) {
                base = reinterpret_cast<uintptr_t>(entry.modBaseAddr);
                break;
            }
        } while (Module32NextW(snap, &entry));
    }

    CloseHandle(snap);
    return base;
}

bool Memory::ReadBuffer(uintptr_t addr, void* buf, size_t size) const {
    if (!m_ProcessHandle) return false;
    SIZE_T bytesRead = 0;
    return ReadProcessMemory(m_ProcessHandle, reinterpret_cast<LPCVOID>(addr), buf, size, &bytesRead)
        && bytesRead == size;
}

std::string Memory::ReadString(uintptr_t addr, size_t maxLen) const {
    if (!m_ProcessHandle) return {};
    char buf[256]{};
    size_t readSize = (std::min)(maxLen, sizeof(buf) - 1);
    SIZE_T bytesRead = 0;
    ReadProcessMemory(m_ProcessHandle, reinterpret_cast<LPCVOID>(addr), buf, readSize, &bytesRead);
    if (bytesRead > 0) {
        buf[bytesRead] = 0;
    
        for (size_t i = 0; i < bytesRead; ++i) {
            if (buf[i] == 0)
                return std::string(buf, i);
        }
        return std::string(buf, bytesRead);
    }
    return {};
}

uintptr_t Memory::ResolvePointer(uintptr_t addr, int level) const {
    uintptr_t result = addr;
    for (int i = 0; i < level; ++i) {
        if (!Read(result, result))
            return 0;
    }
    return result;
}

uintptr_t Memory::ScanPattern(const std::string& pattern, const std::string& mask) const {
    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_ProcessId);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;

    uintptr_t base = 0, size = 0;
    if (Module32FirstW(snap, &entry)) {
        base = reinterpret_cast<uintptr_t>(entry.modBaseAddr);
        size = entry.modBaseSize;
    }
    CloseHandle(snap);

    if (!base || !size)
        return 0;

    constexpr size_t CHUNK_SIZE = 0x10000;
    std::vector<uint8_t> buffer(CHUNK_SIZE);

    for (uintptr_t offset = 0; offset < size; offset += CHUNK_SIZE - pattern.size()) {
        size_t bytesToRead = (std::min)(CHUNK_SIZE, size - offset);
        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(m_ProcessHandle, reinterpret_cast<LPCVOID>(base + offset),
            buffer.data(), bytesToRead, &bytesRead))
            continue;

        for (size_t i = 0; i <= bytesRead - pattern.size(); ++i) {
            bool found = true;
            for (size_t j = 0; j < pattern.size(); ++j) {
                if (mask[j] == 'x' && buffer[i + j] != static_cast<uint8_t>(pattern[j])) {
                    found = false;
                    break;
                }
            }
            if (found)
                return base + offset + i;
        }
    }

    return 0;
}
