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

#define DEBUG_COLLISION 1
#define DEBUG_COLLISION_UNDER 0
#define COLLISION_EPSILON 0.000001f
#define COLLISION_ITERATIONS 4

platform_read_file *PlatformReadFile;
platform_push_thread_work *PlatformPushThreadWork;

enum collision_shape_type
{
    CollisionShapeType_Triangle,
    CollisionShapeType_Circle,
};

struct collision_shape
{
    collision_shape_type Type;
#if DEBUG_COLLISION
        b32 Collided;
#endif
    union
    {
        rectangle2 Rectangle;
        union
        {
            v2 TrianglePoints[3];
            struct
            {
                v2 TrianglePointA;
                v2 TrianglePointB;
                v2 TrianglePointC;
            };
        };
        struct
        {
            r32 Radius;
            v2 Center;
        };
    };
};

enum entity_type
{
    EntityType_Ship,
    EntityType_FloatingHead,
    EntityType_Bullet,
    EntityType_Asteroid,
};

struct entity
{
    entity_type Type;
    v3 P;
    v3 dP;
    r32 Yaw;
    r32 dYaw;
    r32 Pitch;
    r32 Roll;
    r32 Timer;
    v2 Dim;

    b32 Collides;
#if DEBUG_COLLISION
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
    v3 AsteroidStartingP;

    u32 EntityCount;
    entity Entities[256];

    loaded_bitmap ShipBitmap;
    loaded_bitmap AsteroidBitmap;
    loaded_bitmap BulletBitmap;
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
