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

enum collision_shape_type
{
    CollisionShapeType_Rectangle,
    CollisionShapeType_Triangle,
    CollisionShapeType_Circle,
};

struct collision_shape
{
    collision_shape_type Type;
    union
    {
        rectangle2 Rectangle;
        union
        {
            v2 TrianglePoints[3];
            struct
            {
                r32 TrianglePointA;
                r32 TrianglePointB;
                r32 TrianglePointC;
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
    b32 Collided;
    rectangle2 BoundingBox;
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
