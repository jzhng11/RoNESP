#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include "memory.h"
#include "w2s.h"

struct Entity {
    uintptr_t address = 0;
    FVector   position{};       // capsule center
    FVector   feetPosition{};   // floor level (capsule bottom)
    FVector   headPosition{};   // above head
    std::string name;
    std::string className;
    bool      isPlayer = false;
    bool      isSuspect = false;
    bool      isCivilian = false;
    bool      isDowned = false;
    bool      isAlive = true;
    float     health = 100.f;
    float     maxHealth = 100.f;
    float     distance = 0.f;

    bool IsValid() const { return address != 0; }
};

struct CameraData {
    FVector location{};
    FRotator rotation{};
    float fov = 90.f;
    uint32_t screenWidth = 0;
    uint32_t screenHeight = 0;
};

struct GameState {
    uintptr_t worldPtr = 0;
    uintptr_t gameInstancePtr = 0;
    uintptr_t localPlayerPtr = 0;
    uintptr_t playerControllerPtr = 0;
    CameraData camera{};
    std::vector<Entity> entities{};
    uintptr_t moduleBase = 0;
};

struct ClassInfo {
    std::string className;
    bool isPlayer = false;
    bool isSuspect = false;
    bool isCivilian = false;
};

class GameReader {
public:
    explicit GameReader(Memory& mem);

    bool Update(GameState& state);
    void DumpEntityDiagnostic(const GameState& state);

private:
    bool ReadWorld(GameState& state);
    bool ReadLocalPlayer(GameState& state);
    bool ReadCamera(GameState& state);
    bool ReadEntities(GameState& state);
    ClassInfo ClassifyClass(const std::string& className);
    std::string ResolveName(uintptr_t namePtr);
    uintptr_t FindFNamePool(uintptr_t moduleBase);

    Memory& m_Mem;
    uintptr_t m_GNamesPtr = 0;
    std::unordered_map<uintptr_t, ClassInfo> m_ClassCache;
};
