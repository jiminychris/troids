#if !defined(TROIDS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include "troids_platform.h"
#include "troids_intrinsics.h"
#include "troids_math.h"
#include "troids_render.h"
#include "troids_debug.h"

platform_read_file *PlatformReadFile;
platform_push_thread_work *PlatformPushThreadWork;

struct entity
{
    v3 P;
    v3 dP;
    r32 Yaw;
    r32 dYaw;
    r32 Pitch;
    r32 Roll;
    r32 Timer;
    v2 Dim;

    u32 CollisionBoxCount;
    rectangle2 CollisionBoxes[8];
    
    r32 Scale;
    m33 RotationMatrix;
};

struct game_state
{
    b32 IsInitialized;

    r32 MetersToPixels;
    v3 CameraP;
    r32 CameraRot;
    
    u32 RunningSampleCount;

    u32 BulletCount;
    entity Bullets[64];
    u32 AsteroidCount;
    entity Asteroids[64];
    entity Ship;
    entity FloatingHead;

    loaded_bitmap ShipBitmap;
    loaded_bitmap AsteroidBitmap;
    loaded_bitmap BulletBitmap;
#if TROIDS_INTERNAL
    loaded_bitmap DebugBitmap;
#endif
    loaded_obj HeadMesh;
};

struct transient_state
{
    b32 IsInitialized;

    memory_arena TranArena;
    render_buffer RenderBuffer;
};

#define TROIDS_H
#endif
