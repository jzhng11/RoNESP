#pragma once
#include <cstdint>

// Offsets from Dumper-7 SDK, Ready Or Not (UE5.3.2)
// Engine_classes.hpp, ReadyOrNot_classes.hpp, Basic.hpp

// Static addresses are direct struct locations, not pointers-to-pointers
// e.g. GNames address IS the FNamePool, GObjects IS the TUObjectArray
// Basic.hpp refs: GObjects=0x8CC5570, GNames=0x8BC3CB8, GWorld=0x8E33888, ProcessEvent=0xE4E030

struct OffsetData {
    static constexpr uintptr_t GOBJECTS       = 0x08CC5570;
    static constexpr uintptr_t GNAMES         = 0x08C1F180; // from AppendString LEAs, old one was wrong
    static constexpr uintptr_t GWORLD         = 0x08E33888;
    static constexpr uintptr_t PROCESSEVENT   = 0x00E4E030;
    static constexpr uintptr_t APPENDSTRING   = 0x00C927B0;

    // UObject stuff
    static constexpr uintptr_t UOBJECT_FLAGS  = 0x08;
    static constexpr uintptr_t UOBJECT_INDEX  = 0x0C;
    static constexpr uintptr_t UOBJECT_CLASS  = 0x10;
    static constexpr uintptr_t UOBJECT_NAME   = 0x18;
    static constexpr uintptr_t UOBJECT_OUTER  = 0x20;

    static constexpr uintptr_t ACTOR_ROOT_COMPONENT = 0x01A8;
    static constexpr uintptr_t ACTOR_INSTIGATOR     = 0x0190;
    static constexpr uintptr_t CHARACTER_CAPSULE_COMPONENT = 0x0338;

    // UCapsuleComponent inherits UShapeComponent (0x590), adds 0x10
    static constexpr uintptr_t CAPSULE_HALF_HEIGHT = 0x0590;

    // AReadyOrNotCharacter inherits ACharacter (0x678), members start at 0x678
    static constexpr uintptr_t RON_CHARACTER_HEALTH = 0x0900;

    // UCharacterHealthComponent: UHealthComponent(empty)->UResourceComponent(0x100)
    // HealthStatus at +0x160, Resource at +0x0E0
    static constexpr uintptr_t HEALTH_COMP_STATUS = 0x0160;

    // EPlayerHealthStatus: Healthy=0, Injured=1, Downed=2, Incapacitated=3, Dead=4, Arrested=5

    static constexpr uintptr_t PC_PLAYER_CAMERA_MANAGER = 0x358;
    static constexpr uintptr_t PC_ACKNOWLEDGED_PAWN     = 0x348;

    // CameraCachePrivate +0x1320, FCameraCacheEntry layout:
    //   Timestamp(float)+0, Pad(0xC)+4, POV+0x10
    //   FMinimalViewInfo: Location(FVector 24B)+0, Rotation(FRotator 24B)+0x18, FOV(float)+0x30
    static constexpr uintptr_t CAMERA_CACHE       = 0x1320;
    static constexpr uintptr_t CAMERA_CACHE_POV   = 0x10;
    static constexpr uintptr_t VIEW_LOCATION      = 0x00;
    static constexpr uintptr_t VIEW_ROTATION      = 0x18;
    static constexpr uintptr_t VIEW_FOV           = 0x30;

    static constexpr uintptr_t GI_LOCAL_PLAYERS   = 0x38;
    static constexpr uintptr_t UWORLD_PERSISTENT_LEVEL     = 0x30;
    static constexpr uintptr_t UWORLD_OWNING_GAME_INSTANCE = 0x1B8;
    static constexpr uintptr_t ULEVEL_ACTORS_ARRAY = 0x98;

    static constexpr uintptr_t PLAYER_CONTROLLER = 0x30;    // ULocalPlayer -> UPlayer::PlayerController
    static constexpr uintptr_t SCENE_RELATIVE_LOCATION = 0x130;
};
