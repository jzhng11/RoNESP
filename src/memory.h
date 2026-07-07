#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <TlHelp32.h>

class Memory {
public:
    Memory() = default;
    ~Memory();

    bool FindProcess(const std::wstring& name);
    void Close();

    HANDLE Handle() const { return m_ProcessHandle; }
    DWORD ProcessId() const { return m_ProcessId; }

    uintptr_t GetModuleBase(const std::wstring& name) const;

    template<typename T>
    bool Read(uintptr_t addr, T& out) const {
        return ReadBuffer(addr, &out, sizeof(T));
    }

    template<typename T>
    T Read(uintptr_t addr) const {
        T val{};
        ReadBuffer(addr, &val, sizeof(T));
        return val;
    }

    bool ReadBuffer(uintptr_t addr, void* buf, size_t size) const;
    std::string ReadString(uintptr_t addr, size_t maxLen = 64) const;

    uintptr_t ResolvePointer(uintptr_t addr, int level = 1) const;

    uintptr_t ScanPattern(const std::string& pattern, const std::string& mask) const;

private:
    HANDLE m_ProcessHandle = nullptr;
    DWORD m_ProcessId = 0;
};
