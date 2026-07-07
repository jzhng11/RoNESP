#pragma once
#include <cstdint>

// All offsets verified from Dumper-7 SDK for Ready Or Not (UE5.3.2)
// Primary source: SDK struct definitions (Engine_classes.hpp, ReadyOrNot_classes.hpp)
// Static pointer source: Basic.hpp

// -- Static addresses (relative to module base) --
// These are direct addresses of global structs, NOT pointers-to-pointers.
// (e.g. GNames address IS the FNamePool, same pattern as GObjects being the TUObjectArray)
// Basic.hpp: "GObjects at 0x8CC5570"
// Basic.hpp: "GNames at 0x8BC3CB8"
// Basic.hpp: "GWorld at 0x8E33888"
// Basic.hpp: "ProcessEvent at 0xE4E030"

struct OffsetData {
    static constexpr uintptr_t GOBJECTS       = 0x08CC5570;
    static constexpr uintptr_t GNAMES         = 0x08C1F180; // From AppendString LEAs; old 0x08BC3CB8 was wrong
    static constexpr uintptr_t GWORLD         = 0x08E33888;
    static constexpr uintptr_t PROCESSEVENT   = 0x00E4E030;
    static constexpr uintptr_t APPENDSTRING   = 0x00C927B0;

    // -- UObject (Engine_classes.hpp:92) --
    static constexpr uintptr_t UOBJECT_FLAGS  = 0x08;
    static constexpr uintptr_t UOBJECT_INDEX  = 0x0C;
    static constexpr uintptr_t UOBJECT_CLASS  = 0x10;
    static constexpr uintptr_t UOBJECT_NAME   = 0x18;
    static constexpr uintptr_t UOBJECT_OUTER  = 0x20;

    // -- AActor (Engine_classes.hpp:631) --
    static constexpr uintptr_t ACTOR_ROOT_COMPONENT = 0x01A8;
    static constexpr uintptr_t ACTOR_INSTIGATOR     = 0x0190;

    // -- ACharacter (Engine_classes.hpp:3223) --
    static constexpr uintptr_t CHARACTER_CAPSULE_COMPONENT = 0x0338;

    // -- UCapsuleComponent (Engine_classes.hpp:29812) --
    // Inherits UShapeComponent (0x590). Capsule adds 0x10 bytes.
    // CapsuleHalfHeight at +0x590 per SDK struct
    static constexpr uintptr_t CAPSULE_HALF_HEIGHT = 0x0590;

    // -- AReadyOrNotCharacter (ReadyOrNot_classes.hpp:9807) --
    // Inherits ACharacter (0x678). ReadyOrNotCharacter members start at 0x678.
    static constexpr uintptr_t RON_CHARACTER_HEALTH = 0x0900;

    // -- UCharacterHealthComponent (ReadyOrNot_classes.hpp:9399) --
    // Inherits UHealthComponent (empty) -> UResourceComponent (0x100 total)
    // Resource: at +0x0E0 (from UResourceComponent)
    // HealthStatus: at +0x160 (EPlayerHealthStatus uint8)
    static constexpr uintptr_t HEALTH_COMP_STATUS = 0x0160;

    // -- EPlayerHealthStatus (ReadyOrNot_structs.hpp:459) --
    // HS_Healthy=0, HS_Injured=1, HS_Downed=2, HS_Incapacitated=3, HS_Dead=4, HS_Arrested=5

    // -- APlayerController (Engine_classes.hpp) --
    static constexpr uintptr_t PC_PLAYER_CAMERA_MANAGER = 0x358;
    static constexpr uintptr_t PC_ACKNOWLEDGED_PAWN     = 0x348;

    // -- APlayerCameraManager (Engine_classes.hpp) --
    // CameraCachePrivate at +0x1320
    // FCameraCacheEntry: Timestamp(float)+0, Pad_4(0xC), POV+0x10
    //   FMinimalViewInfo: Location(FVector 24B)+0, Rotation(FRotator 24B)+0x18, FOV(float)+0x30
    static constexpr uintptr_t CAMERA_CACHE       = 0x1320;
    static constexpr uintptr_t CAMERA_CACHE_POV   = 0x10;    // offset within FCameraCacheEntry
    static constexpr uintptr_t VIEW_LOCATION      = 0x00;    // FVector (3 doubles)
    static constexpr uintptr_t VIEW_ROTATION      = 0x18;    // FRotator (3 doubles)
    static constexpr uintptr_t VIEW_FOV           = 0x30;    // float

    // -- UGameInstance (Engine_classes.hpp:17109) --
    static constexpr uintptr_t GI_LOCAL_PLAYERS   = 0x38;    // TArray<ULocalPlayer*>

    // -- UWorld (Engine_classes.hpp:13725) --
    static constexpr uintptr_t UWORLD_PERSISTENT_LEVEL     = 0x30;
    static constexpr uintptr_t UWORLD_OWNING_GAME_INSTANCE = 0x1B8;

    // -- ULevel (Engine_classes.hpp:27861) --
    static constexpr uintptr_t ULEVEL_ACTORS_ARRAY = 0x98;   // TArray<AActor*>

    // -- UPlayer (Engine_classes.hpp) --
    // ULocalPlayer inherits UPlayer which has PlayerController at +0x30
    static constexpr uintptr_t PLAYER_CONTROLLER = 0x30;

    // -- USceneComponent (Engine_classes.hpp:490) --
    static constexpr uintptr_t SCENE_RELATIVE_LOCATION = 0x130;
};
