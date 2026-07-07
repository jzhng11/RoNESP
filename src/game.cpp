#include "game.h"
#include "offsets.h"
#include <algorithm>
#include <cctype>

GameReader::GameReader(Memory& mem)
    : m_Mem(mem)
{
    uintptr_t moduleBase = m_Mem.GetModuleBase(L"ReadyOrNotSteam-Win64-Shipping.exe");

    // GNames is the FNamePool itself (like GObjects is the TUObjectArray), not a pointer-to-pointer
    // See Dumper-7 InitGObjects: GObjectsAddress = GetImageBase() + Offsets::GObjects
    // Same pattern applies here - GNames address IS the FNamePool struct
    m_GNamesPtr = moduleBase + OffsetData::GNAMES;
    printf("[+] FNamePool at: 0x%llX\n", m_GNamesPtr);

    // Verify: read FNameEntry[0] from the pool to confirm it works
    uintptr_t poolPtr = m_GNamesPtr;
    uint32_t blockIndex = 0, blockOffset = 0;
    uintptr_t blocksPtr = poolPtr + 0x10;
    uintptr_t blockPtr = 0;
    m_Mem.Read(blocksPtr + blockIndex * 8, blockPtr);
    if (blockPtr) {
        uintptr_t entryAddr = blockPtr + (blockOffset * 2);
        uint16_t rawHeader = 0;
        m_Mem.Read(entryAddr, rawHeader);
        printf("[+] FNamePool block[0] ptr: 0x%llX\n", blockPtr);
        printf("[+] FNameEntry[0] rawHeader: 0x%04X (bWide=%u, len=%u)\n",
            rawHeader, rawHeader & 1, rawHeader >> 6);
        char testBuf[64]{};
        SIZE_T read = 0;
        ReadProcessMemory(m_Mem.Handle(), (LPCVOID)(entryAddr + 2), testBuf, 4, &read);
        testBuf[read] = 0;
        printf("[+] FNameEntry[0] string: \"%s\"\n", testBuf);
    } else {
        printf("[-] FNamePool block[0] is null! Check offset.\n");
    }
}

// Resolve an FName (8 bytes: 4 byte index + 4 byte number) to a string
// Uses UE5's FNamePool system
static bool ReadFNameEntry(Memory& mem, uintptr_t poolPtr, uint32_t index, char* buf, size_t bufSize) {
    if (!poolPtr || !buf || bufSize < 2)
        return false;

    // In UE5, FNamePool stores entries in blocks of 0x10000 entries each (16-bit offset)
    constexpr uint32_t BLOCK_SIZE = 0x10000;
    constexpr uint32_t BLOCK_BITS = 16; // log2(BLOCK_SIZE)

    uint32_t blockIndex = index >> BLOCK_BITS;
    uint32_t blockOffset = index & (BLOCK_SIZE - 1);

    // Read block pointer from pool
    uintptr_t blocksPtr = poolPtr + 0x10; // FNamePool::Blocks (TStaticIndirectArrayThreadSafeRead)
    uintptr_t blockPtr = 0;
    // Each block pointer is 8 bytes
    if (!mem.Read(blocksPtr + blockIndex * 8, blockPtr))
        return false;

    if (!blockPtr)
        return false;

    // Each entry in the block is an FNameEntry (header + string data)
    uintptr_t entryAddr = blockPtr + (blockOffset * 2); // FNameEntry are 2-byte aligned

    // Read entry header: first 2 bytes contain string length and info
    // UE5.3: bit0 = bWide, bits1-5 = probeHash, bits6-15 = len (10 bits)
    struct NameEntryHeader {
        uint16_t bWide : 1;     // bit 0: 0=ANSI, 1=wide
        uint16_t probeHash : 5; // bits 1-5: lowercase probe hash
        uint16_t len : 10;      // bits 6-15: string length
    };

    NameEntryHeader header{};
    if (!mem.Read(entryAddr, header))
        return false;

    size_t strLen = header.len;
    if (strLen == 0) {
        buf[0] = 0;
        return true;
    }

    // The string data follows after the 2-byte header
    uintptr_t strAddr = entryAddr + 2;

    if (header.bWide) {
        // UTF-16 string - convert to narrow
        std::wstring wstr;
        wstr.resize(strLen);
        size_t bytesToRead = strLen * 2;
        if (bytesToRead > bufSize - 1)
            bytesToRead = (bufSize - 1) & ~1; // align to 2

        SIZE_T read = 0;
        ReadProcessMemory(mem.Handle(), reinterpret_cast<LPCVOID>(strAddr),
            wstr.data(), bytesToRead, &read);
        size_t charsRead = read / 2;

        for (size_t i = 0; i < charsRead && i < bufSize - 1; ++i) {
            buf[i] = static_cast<char>(wstr[i]);
            if (wstr[i] > 127) buf[i] = '?';
        }
        buf[charsRead] = 0;
    } else {
        // ANSI string
        size_t toRead = (std::min)(static_cast<size_t>(strLen), bufSize - 1);
        SIZE_T read = 0;
        ReadProcessMemory(mem.Handle(), reinterpret_cast<LPCVOID>(strAddr),
            buf, toRead, &read);
        buf[read] = 0;
    }

    return true;
}

// Pattern-scan for FNamePool in the game module.
// Common UE5 pattern: 74 09 48 8D 15 ?? ?? ?? ?? EB 16
// The LEA instruction loads an RVA to the FNamePool struct.
uintptr_t GameReader::FindFNamePool(uintptr_t moduleBase) {
    // Try common UE5.3 pattern
    // "74 09 48 8D 15 ?? ?? ?? ?? EB 16"
    // This is: JE +9; LEA RDX, [rip+offset]; JMP +22
    const uint8_t pat[] = { 0x74, 0x09, 0x48, 0x8D, 0x15, 0x00, 0x00, 0x00, 0x00, 0xEB, 0x16 };
    const char    mask[] = "xxxx????xxx";

    uintptr_t addr = m_Mem.ScanPattern(std::string((const char*)pat, sizeof(pat)), mask);
    if (addr) {
        // Extract 32-bit relative offset from bytes 5-8
        int32_t relOffset = 0;
        m_Mem.Read(addr + 5, relOffset);
        // LEA is 7 bytes; pool addr = instruction_end + relOffset
        return addr + 7 + relOffset;
    }
    return 0;
}

std::string GameReader::ResolveName(uintptr_t namePtr) {
    // Read FName (uint32 index + uint32 number)
    struct FName {
        uint32_t comparisonIndex = 0;
        uint32_t number = 0;
    };

    FName name{};
    if (!m_Mem.Read(namePtr, name) || name.comparisonIndex == 0)
        return {};

    char buf[256]{};
    if (!ReadFNameEntry(m_Mem, m_GNamesPtr, name.comparisonIndex, buf, sizeof(buf)))
        return {};

    return std::string(buf);
}

bool GameReader::Update(GameState& state) {
    if (!m_Mem.Handle())
        return false;

    // Resolve module base each update (in case of re-read)
    state.moduleBase = m_Mem.GetModuleBase(L"ReadyOrNotSteam-Win64-Shipping.exe");
    if (!state.moduleBase) { printf("[-] Update: no moduleBase\n"); return false; }

    if (!ReadWorld(state)) return false;
    if (!ReadLocalPlayer(state)) return false;
    if (!ReadCamera(state)) return false;
    if (!ReadEntities(state)) return false;

    return true;
}

bool GameReader::ReadWorld(GameState& state) {
    // GWorld is a static pointer at moduleBase + GWORLD
    uintptr_t gworldAddr = state.moduleBase + OffsetData::GWORLD;
    if (!m_Mem.Read(gworldAddr, state.worldPtr) || !state.worldPtr)
        return false;

    // Read OwningGameInstance
    uintptr_t giAddr = state.worldPtr + OffsetData::UWORLD_OWNING_GAME_INSTANCE;
    return m_Mem.Read(giAddr, state.gameInstancePtr);
}

bool GameReader::ReadLocalPlayer(GameState& state) {
    if (!state.gameInstancePtr) return false;

    // GameInstance::LocalPlayers is a TArray<ULocalPlayer*>
    uintptr_t localPlayersPtr = state.gameInstancePtr + OffsetData::GI_LOCAL_PLAYERS;

    // TArray: 8 bytes data ptr + 4 bytes count + 4 bytes max
    uintptr_t localPlayersData = 0;
    int32_t localPlayersCount = 0;
    if (!m_Mem.Read(localPlayersPtr, localPlayersData)) return false;
    if (!m_Mem.Read(localPlayersPtr + 8, localPlayersCount)) return false;

    if (localPlayersCount < 1 || !localPlayersData) return false;

    // Get first local player
    if (!m_Mem.Read(localPlayersData, state.localPlayerPtr) || !state.localPlayerPtr)
        return false;

    // UPlayer::PlayerController at +0x30 (ULocalPlayer inherits this)
    uintptr_t pcAddr = state.localPlayerPtr + OffsetData::PLAYER_CONTROLLER;
    if (!m_Mem.Read(pcAddr, state.playerControllerPtr))
        return false;

    return state.playerControllerPtr != 0;
}

bool GameReader::ReadCamera(GameState& state) {
    if (!state.playerControllerPtr) return false;

    uintptr_t camMgrAddr = state.playerControllerPtr + OffsetData::PC_PLAYER_CAMERA_MANAGER;
    uintptr_t camMgrPtr = 0;
    if (!m_Mem.Read(camMgrAddr, camMgrPtr) || !camMgrPtr)
        return false;

    // APlayerCameraManager::CameraCachePrivate at +0x1320
    // FCameraCacheEntry:
    //   +0x00: Timestamp (float)
    //   +0x04: Pad (0x0C bytes)
    //   +0x10: POV (FMinimalViewInfo)
    //     +0x00: Location (FVector = 3 doubles, 24 bytes)
    //     +0x18: Rotation (FRotator = 3 doubles, 24 bytes)
    //     +0x30: FOV (float)
    uintptr_t viewInfoAddr = camMgrPtr + OffsetData::CAMERA_CACHE + OffsetData::CAMERA_CACHE_POV;

    double viewLoc[3] = {};
    if (!m_Mem.Read(viewInfoAddr + OffsetData::VIEW_LOCATION, viewLoc))
        return false;
    state.camera.location.x = viewLoc[0];
    state.camera.location.y = viewLoc[1];
    state.camera.location.z = viewLoc[2];

    double viewRot[3] = {};
    if (!m_Mem.Read(viewInfoAddr + OffsetData::VIEW_ROTATION, viewRot))
        return false;
    state.camera.rotation.pitch = viewRot[0];
    state.camera.rotation.yaw = viewRot[1];
    state.camera.rotation.roll = viewRot[2];

    float fov = 0.f;
    if (!m_Mem.Read(viewInfoAddr + OffsetData::VIEW_FOV, fov))
        return false;
    state.camera.fov = fov;

    return true;
}

bool GameReader::ReadEntities(GameState& state) {
    static bool once = false;
    bool diag = !once;
    once = true;

    state.entities.clear();

    if (!state.worldPtr) { if (diag) printf("[-] ReadEntities: no world\n"); return false; }

    // Get PersistentLevel
    uintptr_t levelPtr = 0;
    if (!m_Mem.Read(state.worldPtr + OffsetData::UWORLD_PERSISTENT_LEVEL, levelPtr) || !levelPtr)
        { if (diag) printf("[-] ReadEntities: no level\n"); return false; }

    // Read Actors TArray (data ptr + count)
    uintptr_t actorsData = 0;
    int32_t actorsCount = 0;
    if (!m_Mem.Read(levelPtr + OffsetData::ULEVEL_ACTORS_ARRAY, actorsData)) { if (diag) printf("[-] ReadEntities: can't read actorsData\n"); return false; }
    if (!m_Mem.Read(levelPtr + OffsetData::ULEVEL_ACTORS_ARRAY + 8, actorsCount)) { if (diag) printf("[-] ReadEntities: can't read actorsCount\n"); return false; }

    if (diag) printf("[i] Actors count: %d (data=0x%llX)\n", actorsCount, actorsData);

    if (!actorsData || actorsCount <= 0 || actorsCount > 20000)
        { if (diag) printf("[-] ReadEntities: bad actors array (%d)\n", actorsCount); return false; }

    // Read all actor pointers at once in a batch read
    std::vector<uintptr_t> actorPtrs(actorsCount);
    SIZE_T bytesRead = 0;
    if (!ReadProcessMemory(m_Mem.Handle(), reinterpret_cast<LPCVOID>(actorsData),
        actorPtrs.data(), actorsCount * sizeof(uintptr_t), &bytesRead)) {
        return false;
    }
    size_t actualCount = bytesRead / sizeof(uintptr_t);

    state.entities.reserve(actualCount);

    // Diagnostic: one-shot, comprehensive level and actor scan
    if (diag) {
        printf("\n===== ONE-SHOT DIAGNOSTIC =====\n");

        // 0. Camera info
        printf("[D] Camera: (%.1f, %.1f, %.1f) rot=(%.1f, %.1f, %.1f) FOV=%.1f\n",
            state.camera.location.x, state.camera.location.y, state.camera.location.z,
            state.camera.rotation.pitch, state.camera.rotation.yaw, state.camera.rotation.roll,
            state.camera.fov);
        printf("[D] LocalPlayer: 0x%llX  PC: 0x%llX\n",
            state.localPlayerPtr, state.playerControllerPtr);

        // 1. UWorld info
        printf("[D] UWorld ptr: 0x%llX\n", state.worldPtr);

        // 2. GameState (non-null = in-game)
        uintptr_t gsPtr = 0;
        m_Mem.Read(state.worldPtr + 0x158, gsPtr);
        printf("[D] GameState: 0x%llX (%s)\n", gsPtr, gsPtr ? "IN-GAME" : "NULL - likely menu/loading");

        // 3. Levels array (ALL loaded levels)
        uintptr_t levelsData = 0;
        int32_t levelsCount = 0;
        m_Mem.Read(state.worldPtr + 0x170, levelsData);
        m_Mem.Read(state.worldPtr + 0x178, levelsCount);
        printf("[D] Levels array: data=0x%llX count=%d\n", levelsData, levelsCount);

        // 4. StreamingLevels array
        uintptr_t slData = 0;
        int32_t slCount = 0;
        m_Mem.Read(state.worldPtr + 0x88, slData);
        m_Mem.Read(state.worldPtr + 0x90, slCount);
        printf("[D] StreamingLevels: data=0x%llX count=%d\n", slData, slCount);

        // 5. For each loaded level, print actor count and sample class names
        if (levelsData && levelsCount > 0 && levelsCount < 100) {
            for (int32_t li = 0; li < levelsCount; ++li) {
                uintptr_t lvlPtr = 0;
                m_Mem.Read(levelsData + li * 8, lvlPtr);
                if (!lvlPtr) continue;

                uintptr_t lvlActorsData = 0;
                int32_t lvlActorsCount = 0;
                m_Mem.Read(lvlPtr + OffsetData::ULEVEL_ACTORS_ARRAY, lvlActorsData);
                m_Mem.Read(lvlPtr + OffsetData::ULEVEL_ACTORS_ARRAY + 8, lvlActorsCount);

                printf("[D] Level[%d] @0x%llX: %d actors\n", li, lvlPtr, lvlActorsCount);

                // Sample first 5 actor class names
                if (lvlActorsData && lvlActorsCount > 0) {
                    for (int32_t ai = 0; ai < (std::min)(lvlActorsCount, 5); ++ai) {
                        uintptr_t ap = 0;
                        m_Mem.Read(lvlActorsData + ai * 8, ap);
                        if (!ap) continue;
                        uintptr_t clsPtr = 0;
                        m_Mem.Read(ap + OffsetData::UOBJECT_CLASS, clsPtr);
                        if (!clsPtr) continue;
                        std::string name = ResolveName(clsPtr + OffsetData::UOBJECT_NAME);
                        printf("[D]   LvlActor[%d] class=\"%s\"\n", ai, name.c_str());
                    }
                }
            }
        } else {
            printf("[D] Levels array invalid or empty\n");
        }

        // 6. Scan persistent level actors for character class names
        bool foundAny = false;
        for (size_t i = 0; i < actualCount; ++i) {
            uintptr_t ap = actorPtrs[i];
            if (!ap) continue;
            uint32_t flags = 0;
            if (!m_Mem.Read(ap + 8, flags)) continue;
            if ((flags & 0x05) != 0) continue;
            uintptr_t clsPtr = 0;
            if (!m_Mem.Read(ap + OffsetData::UOBJECT_CLASS, clsPtr) || !clsPtr) continue;
            std::string name = ResolveName(clsPtr + OffsetData::UOBJECT_NAME);
            if (!name.empty() && (
                name.find("Pawn") != std::string::npos ||
                name.find("Character") != std::string::npos ||
                name.find("ORN_") != std::string::npos ||
                name.find("ReadyOrNot") != std::string::npos ||
                (name.find("SWAT") != std::string::npos && name.find("Controller") == std::string::npos) ||
                (name.find("Suspect") != std::string::npos && name.find("Controller") == std::string::npos) ||
                (name.find("Civilian") != std::string::npos && name.find("Controller") == std::string::npos))) {
                printf("[D] PERSISTENT Pawn/Char Actor[%zu] class=\"%s\"\n", i, name.c_str());
                foundAny = true;
            }
        }
        if (!foundAny) printf("[D] No pawn/character class names in persistent level\n");

        printf("===============================\n\n");
    }

    // Classifier cache: read all class pointers, resolve only unknown ones
    // Data buffer for reading flags + class pointer in one RPM per actor
    struct ActorFastData {
        uint32_t flags;
        uint32_t pad;
        uintptr_t clsPtr;
    };

    for (size_t i = 0; i < actualCount; ++i) {
        uintptr_t actorPtr = actorPtrs[i];
        if (!actorPtr) continue;

        // Batch-read flags + class pointer in one RPM call (16 bytes)
        ActorFastData fd{};
        if (!m_Mem.Read(actorPtr + 8, fd))
            continue;
        if ((fd.flags & 0x05) != 0)
            continue;
        if (!fd.clsPtr)
            continue;

        // Check class cache
        auto it = m_ClassCache.find(fd.clsPtr);
        if (it == m_ClassCache.end()) {
            // Cache miss: resolve class name and classify
            std::string clsName = ResolveName(fd.clsPtr + OffsetData::UOBJECT_NAME);
            if (clsName.empty()) {
                // Mark as rejected so we don't retry
                m_ClassCache[fd.clsPtr] = ClassInfo{};
                continue;
            }
            ClassInfo ci = ClassifyClass(clsName);
            ci.className = clsName;
            it = m_ClassCache.insert({fd.clsPtr, ci}).first;
        }

        const ClassInfo& ci = it->second;
        if (!ci.isPlayer && !ci.isSuspect && !ci.isCivilian)
            continue;

        // This is a character - read full entity info
        Entity ent{};
        ent.address = actorPtr;
        ent.isPlayer = ci.isPlayer;
        ent.isSuspect = ci.isSuspect;
        ent.isCivilian = ci.isCivilian;
        ent.className = ci.className;

        // Friendly name based on classification
        if (ent.isSuspect)
            ent.name = "Enemy";
        else if (ent.isCivilian)
            ent.name = "Innocent";
        else if (ent.isPlayer)
            ent.name = "Player";
        else
            ent.name = "Unknown";

        // Read root component
        uintptr_t rootComp = 0;
        if (!m_Mem.Read(actorPtr + OffsetData::ACTOR_ROOT_COMPONENT, rootComp) || !rootComp)
            continue;

        // Read position from root component
        double relLoc[3] = {};
        if (!m_Mem.Read(rootComp + OffsetData::SCENE_RELATIVE_LOCATION, relLoc))
            continue;
        ent.position.x = relLoc[0];
        ent.position.y = relLoc[1];
        ent.position.z = relLoc[2];

        // Read capsule half-height
        uintptr_t capsuleComp = 0;
        float halfHeight = 88.f;
        if (m_Mem.Read(actorPtr + OffsetData::CHARACTER_CAPSULE_COMPONENT, capsuleComp) && capsuleComp) {
            float hh = 0.f;
            if (m_Mem.Read(capsuleComp + OffsetData::CAPSULE_HALF_HEIGHT, hh) && hh > 0.f && hh < 500.f)
                halfHeight = hh;
        }

        ent.feetPosition = ent.position;
        ent.feetPosition.z -= halfHeight;

        ent.headPosition = ent.position;
        ent.headPosition.z += halfHeight + 20.f;

        // Read health
        uintptr_t healthComp = 0;
        bool hasHealthComp = m_Mem.Read(actorPtr + OffsetData::RON_CHARACTER_HEALTH, healthComp) && healthComp;
        if (hasHealthComp) {
            uint8_t healthStatus = 0;
            if (m_Mem.Read(healthComp + OffsetData::HEALTH_COMP_STATUS, healthStatus)) {
                if (healthStatus >= 4) {
                    ent.isAlive = false;
                    ent.health = 0.f;
                    ent.maxHealth = 100.f;
                } else if (healthStatus > 0) {
                    // Any non-healthy, non-dead status = downed
                    // (HS_Injured=1, HS_Downed=2, HS_Incapacitated=3)
                    ent.isDowned = true;
                    ent.health = 0.f;
                    ent.maxHealth = 100.f;
                } else {
                    ent.health = 100.f;
                    ent.maxHealth = 100.f;
                }
            }
        } else if (ent.isSuspect || ent.isCivilian) {
            // Suspects/civilians always have a health component while alive.
            // If it's null/missing they're dead — filter them out.
            ent.isAlive = false;
        }

        // Additional arrest/downed detection: actor +0x348 holds movement flags.
        // Healthy uncuffed = 7 (bit 0+1+2), cuffed = 1 (only bit 0).
        // This catches healthy cuffed civilians that health status alone misses.
        if (ent.isAlive && !ent.isDowned) {
            uint8_t arrestFlags = 0;
            if (m_Mem.Read(actorPtr + 0x348, arrestFlags)) {
                // If movement flags (bits 1-2) are missing, character is restrained
                // i.e. value < 4 or specifically value == 1
                if (arrestFlags < 4)
                    ent.isDowned = true;
            }
        }

        // Calculate distance
        double dx = ent.position.x - state.camera.location.x;
        double dy = ent.position.y - state.camera.location.y;
        double dz = ent.position.z - state.camera.location.z;
        ent.distance = static_cast<float>(sqrt(dx * dx + dy * dy + dz * dz) * 0.01);

        state.entities.push_back(ent);
    }

    return true;
}

void GameReader::DumpEntityDiagnostic(const GameState& state) {
    if (state.entities.empty()) {
        printf("[D] No entities to dump\n");
        return;
    }

    // Find nearest alive non-player entity
    const Entity* e = nullptr;
    float minDist = FLT_MAX;
    for (const auto& ent : state.entities) {
        if (ent.isPlayer) continue;
        if (ent.distance < minDist) {
            minDist = ent.distance;
            e = &ent;
        }
    }
    if (!e) {
        printf("[D] No suitable entity near crosshair\n");
        return;
    }
    const auto& entRef = *e;

    printf("\n===== ENTITY DUMP (nearest, dist=%.0fm) =====\n", entRef.distance);
    printf("  name=%s addr=0x%llX cls=%s\n", entRef.name.c_str(), entRef.address, entRef.className.c_str());
    printf("  isSuspect=%d isCivilian=%d isDowned=%d isAlive=%d\n",
        entRef.isSuspect, entRef.isCivilian, entRef.isDowned, entRef.isAlive);

    uintptr_t hc = 0;
    m_Mem.Read(entRef.address + OffsetData::RON_CHARACTER_HEALTH, hc);
    if (hc) {
        uint8_t status = 0xFF;
        m_Mem.Read(hc + OffsetData::HEALTH_COMP_STATUS, status);
        printf("  healthComp=0x%llX status=%u\n", hc, status);

        uint8_t full[256] = {};
        m_Mem.ReadBuffer(hc, full, sizeof(full));
        printf("  healthComp full [+0x00..+0xFF]:");
        for (int i = 0; i < 256; ++i) {
            if (i % 32 == 0) printf("\n    ");
            printf(" %02X", full[i]);
        }
        printf("\n");
    } else {
        printf("  healthComp=null\n");
    }

    // Full actor data from +0x260 (after capsule component area)
    uint8_t actorFull[256] = {};
    m_Mem.ReadBuffer(entRef.address + 0x260, actorFull, sizeof(actorFull));
    printf("  actor full [+0x260..+0x35F]:");
    for (int i = 0; i < 256; ++i) {
        if (i % 32 == 0) printf("\n    ");
        printf(" %02X", actorFull[i]);
    }
    printf("\n");

    printf("=========================\n\n");
}

ClassInfo GameReader::ClassifyClass(const std::string& clsName) {
    ClassInfo ci;
    ci.className = clsName;

    // Reject non-actor base classes
    if (clsName.find("GM_") == 0 || clsName.find("GS_") == 0 ||
        clsName.find("PS_") == 0 || clsName.find("BP_PhysActor") != std::string::npos ||
        clsName.find("BP_AISpawn") != std::string::npos)
        return ci;

    // Reject AI controllers
    if (clsName.find("Controller") != std::string::npos)
        return ci;

    // Classify
    if (clsName.find("BP_SWAT") != std::string::npos ||
        clsName.find("SWATCharacter") != std::string::npos) {
        ci.isPlayer = true;
    } else if (clsName.find("CivilianCharacter") != std::string::npos ||
               clsName.find("BP_Civilian") != std::string::npos ||
               clsName.find("Cybernetics_Civilian") != std::string::npos ||
               clsName.find("Faction_GenericCivilians") != std::string::npos) {
        ci.isCivilian = true;
    } else if (clsName.find("SuspectCharacter") != std::string::npos ||
               clsName.find("BP_Suspect") != std::string::npos ||
               clsName.find("CyberneticsSuspect") != std::string::npos) {
        ci.isSuspect = true;
    }

    return ci;
}
