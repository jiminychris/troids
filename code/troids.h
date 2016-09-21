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

#define DEBUG_LINEAR_COLLISION 1
#define DEBUG_ANGULAR_COLLISION 0
#define DEBUG_COLLISION DEBUG_LINEAR_COLLISION|DEBUG_ANGULAR_COLLISION
#define COLLISION_EPSILON 0.01f
#define COLLISION_T_ADJUST 0.1f
#define COLLISION_ITERATIONS 4

platform_read_file *PlatformReadFile;
platform_push_thread_work *PlatformPushThreadWork;

enum collision_type
{
    CollisionType_None,
    CollisionType_Circle,
    CollisionType_Line,
};

struct collision
{
    collision_type Type;
    union
    {
        union
        {
            v2 LinePoints[2];
            struct
            {
                v2 A;
                v2 B;
            };
        };
        struct
        {
            v2 Deflection;
        };
    };
};

enum collision_shape_type
{
    CollisionShapeType_None = 0,
    CollisionShapeType_Triangle = 1<<0,
    CollisionShapeType_Circle = 1<<1,
};

global_variable const u32 CollisionShapePair_TriangleTriangle = CollisionShapeType_Triangle;
global_variable const u32 CollisionShapePair_CircleCircle = CollisionShapeType_Circle;
global_variable const u32 CollisionShapePair_TriangleCircle = CollisionShapeType_Circle&CollisionShapeType_Triangle;

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
                v2 A;
                v2 B;
                v2 C;
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
