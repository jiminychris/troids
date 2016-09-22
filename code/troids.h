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
#include "troids_physics.h"
#include "troids_render.h"
#include "troids_debug.h"

platform_read_file *PlatformReadFile;
platform_push_thread_work *PlatformPushThreadWork;

enum entity_type
{
    EntityType_Ship = 1<<0,
    EntityType_FloatingHead = 1<<1,
    EntityType_Bullet = 1<<2,
    EntityType_Asteroid = 1<<3,
};

global_variable const u32 EntityPair_AsteroidBullet = EntityType_Asteroid|EntityType_Bullet;
global_variable const u32 EntityPair_AsteroidShip = EntityType_Asteroid|EntityType_Ship;

struct entity
{
    entity_type Type;
    collider_type ColliderType;
    v3 P;
    v3 dP;
    r32 Yaw;
    r32 dYaw;
    r32 Pitch;
    r32 Roll;
    r32 Timer;
    v2 Dim;

    b32 Destroyed;
#if COLLISION_DEBUG
    b32 BoundingCircleCollided;
#endif
    r32 BoundingRadius;
    u32 CollisionShapeCount;
    collision_shape CollisionShapes[8];
    
    m33 RotationMatrix;
};

struct game_state
{
    b32 IsInitialized;

    r32 MetersToPixels;
    v3 CameraP;
    r32 CameraRot;
    
    u32 RunningSampleCount;

    v3 ShipStartingP;
    r32 ShipStartingYaw;

    physics_state PhysicsState;

    s32 Lives;

    u32 EntityCount;
    entity Entities[256];

    loaded_bitmap ShipBitmap;
    loaded_bitmap AsteroidBitmap;
    loaded_bitmap BulletBitmap;
    loaded_obj HeadMesh;
};

inline b32
CanCollide(entity *Entity)
{
    b32 Result = !Entity->Destroyed && (Entity->ColliderType != ColliderType_None);
    return(Result);
}

inline b32
CanCollide(game_state *State, entity *A, entity *B)
{
    b32 Result = !A->Destroyed && !B->Destroyed &&
        CanCollide(&State->PhysicsState, A->ColliderType, B->ColliderType);
    return(Result);
}

struct transient_state
{
    b32 IsInitialized;

    memory_arena TranArena;
    render_buffer RenderBuffer;
};

#define TROIDS_H
#endif
