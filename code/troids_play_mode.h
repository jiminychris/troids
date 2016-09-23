#if !defined(TROIDS_PLAY_MODE_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

enum entity_type
{
    EntityType_Ship = 1<<0,
    EntityType_FloatingHead = 1<<1,
    EntityType_Bullet = 1<<2,
    EntityType_Asteroid = 1<<3,
    EntityType_Letter = 1<<4,
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
    r32 BoundingRadius;
    u32 CollisionShapeCount;
    collision_shape CollisionShapes[8];
    
    m33 RotationMatrix;
    char Character;
#if TROIDS_INTERNAL
    b32 BoundingCircleCollided;
#endif
};

struct play_state
{
    b32 IsInitialized;
    
    v3 ShipStartingP;
    r32 ShipStartingYaw;

    physics_state PhysicsState;

    s32 Lives;

    // TODO(chris): Remove this
    entity *DEBUGEntity;

    u32 EntityCount;
    entity Entities[256];
};

inline entity *
CreateEntity(play_state *State)
{
    entity *Result = 0;
    if(State->EntityCount < ArrayCount(State->Entities))
    {
        Result = State->Entities + State->EntityCount++;
    }
    else
    {
        Assert(!"Created too many entities.");
    }
    *Result = {};
    return(Result);
}

inline void
DestroyEntity(play_state *State, entity *Entity)
{
    *Entity = State->Entities[--State->EntityCount];
}

inline b32
CanCollide(entity *Entity)
{
    b32 Result = !Entity->Destroyed && (Entity->ColliderType != ColliderType_None);
    return(Result);
}

inline b32
CanCollide(play_state *State, entity *A, entity *B)
{
    b32 Result = !A->Destroyed && !B->Destroyed &&
        CanCollide(&State->PhysicsState, A->ColliderType, B->ColliderType);
    return(Result);
}

#define TROIDS_PLAY_MODE_H
#endif
