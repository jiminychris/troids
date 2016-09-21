#if !defined(TROIDS_PHYSICS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#define COLLISION_DEBUG_LINEAR 0
#define COLLISION_DEBUG_ANGULAR 0
#define COLLISION_DEBUG COLLISION_DEBUG_LINEAR|COLLISION_DEBUG_ANGULAR
#define COLLISION_T_ADJUST 0.1f
#define COLLISION_ITERATIONS 2

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
