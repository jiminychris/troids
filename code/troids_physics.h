#if !defined(TROIDS_PHYSICS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#define COLLISION_DEBUG_LINEAR 1
#define COLLISION_DEBUG_ANGULAR 0
#define COLLISION_DEBUG COLLISION_DEBUG_LINEAR|COLLISION_DEBUG_ANGULAR
#define COLLISION_T_ADJUST 0.1f
#define COLLISION_ITERATIONS 2

enum collider_type
{
    ColliderType_Identity = 0,
    
    ColliderType_None = 1<<0,
    ColliderType_Ship = 1<<1,
    ColliderType_Asteroid = 1<<2,
};

struct physics_state
{
    b32 ColliderTable[256];
};

inline void
AllowCollision(physics_state *State, collider_type A, collider_type B)
{
    State->ColliderTable[A|B] = 1;
}

inline b32
CanCollide(physics_state *State, collider_type A, collider_type B)
{
    b32 Result = State->ColliderTable[A|B];
    return(Result);
}

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
global_variable const u32 CollisionShapePair_TriangleCircle = CollisionShapeType_Circle|CollisionShapeType_Triangle;

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
#if TROIDS_INTERNAL
        b32 Collided;
#endif
};

struct circle_ray_intersection_result
{
    r32 t1;
    r32 t2;
};

struct arc_triangle_edge_intersection_result
{
    r32 t1;
    r32 t2;
};

struct circle_circle_intersection_result
{
    v2 P1;
    v2 P2;
};

struct arc_circle_intersection_result
{
    r32 t1;
    r32 t2;
};

inline b32
HasIntersection(circle_circle_intersection_result Intersection)
{
    b32 Result = !IsNaN(Intersection.P1.x);
    return(Result);
}

inline b32
HasIntersection(circle_ray_intersection_result Intersection)
{
    b32 Result = !IsNaN(Intersection.t1);
    return(Result);
}

inline b32
HasIntersection(r32 Intersection)
{
    b32 Result = !IsNaN(Intersection);
    return(Result);
}

inline b32
HasIntersection(arc_circle_intersection_result Intersection)
{
    b32 Result = !IsNaN(Intersection.t1);
    return(Result);
}

#define TROIDS_PHYSICS_H
#endif
