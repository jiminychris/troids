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
    collision_shape *CollisionShapes;
    
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
CreateEntity(play_state *State, u32 ShapeCount = 0, collision_shape *Shapes = 0)
{
    entity *Result = 0;
    physics_state *PhysicsState = &State->PhysicsState;
    memory_arena *ShapeArena = &PhysicsState->ShapeArena;
    if(State->EntityCount < ArrayCount(State->Entities))
    {
        if(HasRoomForArray(ShapeArena, ShapeCount, collision_shape))
        {
            Result = State->Entities + State->EntityCount++;
            *Result = {};
            for(u32 ShapeIndex = 0;
                ShapeIndex < ShapeCount;
                ++ShapeIndex)
            {
                collision_shape *Shape = PhysicsState->FirstFreeShape;
                if(Shape)
                {
                    PhysicsState->FirstFreeShape = Shape->NextFree;
                }
                else
                {
                    Shape = PushStruct(ShapeArena, collision_shape);
                }
                *Shape = *(Shapes + ShapeIndex);
                Shape->Next = Result->CollisionShapes;
                Result->CollisionShapes = Shape;
            }
        }
        else
        {
            Assert(!"Created too many collision shapes.");
        }
    }
    else
    {
        Assert(!"Created too many entities.");
    }
    return(Result);
}

inline void
DestroyEntity(play_state *State, entity *Entity)
{
    physics_state *PhysicsState = &State->PhysicsState;
    *Entity = State->Entities[--State->EntityCount];
    for(collision_shape *Shape = Entity->CollisionShapes;
        Shape;
        )
    {
        collision_shape *Next = Shape->Next;

        Shape->NextFree = PhysicsState->FirstFreeShape;
        PhysicsState->FirstFreeShape = Shape;
        
        Shape = Next;
    }
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
