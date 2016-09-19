/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include <stdlib.h>
#include <stdio.h>

#include "troids.h"
#include "troids_render.cpp"

internal loaded_bitmap
LoadBitmap(char *FileName)
{
    loaded_bitmap Result = {};
    // TODO(chris): Allocate memory from arena instead of dynamic platform alloc
    read_file_result ReadResult = PlatformReadFile(FileName);
    bitmap_header *BMPHeader = (bitmap_header *)ReadResult.Contents;

    Assert(BMPHeader->FileType == (('B' << 0) | ('M' << 8)));
    Assert(BMPHeader->FileSize == ReadResult.ContentsSize);
    Assert(BMPHeader->Reserved1 == 0);
    Assert(BMPHeader->Reserved2 == 0);
    Assert(BMPHeader->Planes == 1);
    Assert(BMPHeader->BitsPerPixel == 32);
    Assert(BMPHeader->Compression == 3);
    Assert(BMPHeader->ColorsUsed == 0);
    Assert(BMPHeader->ColorsImportant == 0);

    Result.Height = BMPHeader->Height;
    Result.Width = BMPHeader->Width;
    Result.Align = V2(0.5f, 0.5f);
    Result.Pitch = Result.Width*sizeof(u32);
    Result.Memory = (u8 *)BMPHeader + BMPHeader->BitmapOffset;

    u32 AlphaShift = BitScanForward(BMPHeader->AlphaMask).Index;
    u32 RedShift = BitScanForward(BMPHeader->RedMask).Index;
    u32 GreenShift = BitScanForward(BMPHeader->GreenMask).Index;
    u32 BlueShift = BitScanForward(BMPHeader->BlueMask).Index;

    u8 *PixelRow = (u8 *)Result.Memory;
    r32 Inv255 = 1.0f / 255.0f;
    for(s32 Y = 0;
        Y < Result.Height;
        ++Y)
    {
        u32 *Pixel = ((u32 *)PixelRow);
        for(s32 X = 0;
            X < Result.Width;
            ++X)
        {
            u32 A = ((*Pixel & BMPHeader->AlphaMask) >> AlphaShift) & 0xFF;
            if(A)
            {
                int Z = 0;
            }

            r32 Alpha = A*Inv255;
            r32 Red = Alpha*(((*Pixel & BMPHeader->RedMask) >> RedShift) & 0xFF);
            r32 Green = Alpha*(((*Pixel & BMPHeader->GreenMask) >> GreenShift) & 0xFF);
            r32 Blue = Alpha*(((*Pixel & BMPHeader->BlueMask) >> BlueShift) & 0xFF);

            u32 R = (u32)(Red + 0.5f);
            u32 G = (u32)(Green + 0.5f);
            u32 B = (u32)(Blue + 0.5f);

            *Pixel++ = ((A << 24) |
                        (R << 16) |
                        (G << 8) |
                        (B << 0));
        }
        PixelRow += Result.Pitch;
    }

    return(Result);
}

inline b32
IsEOF(char *String)
{
    b32 Result = (*String == 0);
    return(Result);
}

inline char *
SkipWhitespace(char *String)
{
    char *Result = String;
    while(!IsEOF(Result) &&
          ((*Result == ' ') ||
           (*Result == '\n') ||
           (*Result == '\r') ||
           (*Result == '\t')))
    {
        ++Result;
    }
    return(Result);
}

inline char *
SkipLine(char *String)
{
    char *Result = String;
    while(!IsEOF(Result) && *Result++ != '\n');
    return(Result);
}

internal loaded_obj
LoadObj(char *FileName, memory_arena *Arena)
{
    loaded_obj Result = {};
    Result.TextureVertexCount = 1;
    Result.VertexNormalCount = 1;
    Result.VertexCount = 1;
    Result.FaceCount = 1;
    // TODO(chris): Allocate memory from arena instead of dynamic platform alloc
    read_file_result ReadResult = PlatformReadFile(FileName);

    char *At = (char *)ReadResult.Contents;
    // NOTE(chris): Just do two passes over the file for now.
    while(!IsEOF(At))
    {
        switch(*At++)
        {
            case 'v':
            {
                if(*At == 't')
                {
                    ++Result.TextureVertexCount;
                }
                else if(*At == 'n')
                {
                    ++Result.VertexNormalCount;
                }
                else
                {
                    ++Result.VertexCount;
                }
            } break;

            case 'f':
            {
                ++Result.FaceCount;
            } break;
        }
        At = SkipLine(At);
    }

    Result.Vertices = PushArray(Arena, Result.VertexCount, v4);
    Result.TextureVertices = PushArray(Arena, Result.TextureVertexCount, v3);
    Result.VertexNormals = PushArray(Arena, Result.VertexNormalCount, v3);
    Result.Faces = PushArray(Arena, Result.FaceCount, obj_face);

    v4 *Vertices = Result.Vertices + 1;
    v3 *TextureVertices = Result.TextureVertices + 1;
    v3 *VertexNormals = Result.VertexNormals + 1;
    obj_face *Faces = Result.Faces + 1;
    
    At = (char *)ReadResult.Contents;
    while(!IsEOF(At))
    {
        switch(*At++)
        {
            case 'v':
            {
                if(*At == 't')
                {
                    ++At;
                    TextureVertices->u = strtof(At, &At);
                    TextureVertices->v = strtof(At, &At);
                    // TODO(chris): Don't read optional w value
                    TextureVertices->w = strtof(At, &At);

                    ++TextureVertices;
                }
                else if(*At == 'n')
                {
                    ++At;
                    VertexNormals->x = strtof(At, &At);
                    VertexNormals->y = strtof(At, &At);
                    VertexNormals->z = strtof(At, &At);

                    ++VertexNormals;
                }
                else
                {
                    Vertices->x = strtof(At, &At);
                    Vertices->y = strtof(At, &At);
                    Vertices->z = strtof(At, &At);
                    // TODO(chris): Read optional w value
                    Vertices->w = 1.0f;

                    ++Vertices;
                }
            } break;

            case 'f':
            {
                Faces->VertexIndexA = strtoul(At, &At, 10);
                ++At;
                Faces->TextureVertexIndexA = strtoul(At, &At, 10);
                ++At;
                Faces->VertexNormalIndexA = strtoul(At, &At, 10);

                Faces->VertexIndexB = strtoul(At, &At, 10);
                ++At;
                Faces->TextureVertexIndexB = strtoul(At, &At, 10);
                ++At;
                Faces->VertexNormalIndexB = strtoul(At, &At, 10);

                Faces->VertexIndexC = strtoul(At, &At, 10);
                ++At;
                Faces->TextureVertexIndexC = strtoul(At, &At, 10);
                ++At;
                Faces->VertexNormalIndexC = strtoul(At, &At, 10);

                ++Faces;
            } break;

            case '#':
            {
                // NOTE(chris): This is a comment in the .obj file
                At = SkipLine(At);
            } break;

            case 'g':
            {
                // TODO(chris): Implement .obj groups
                At = SkipLine(At);
            } break;

            case 's':
            {
                // TODO(chris): Implement .obj smoothing
                At = SkipLine(At);
            } break;

            InvalidDefaultCase;
        }
        At = SkipWhitespace(At);
    }

    return(Result);
}

#define LOG_CONTROLLER(Controller)                                      \
    {                                                                   \
        DEBUG_VALUE("LeftStick", (Controller)->LeftStick);              \
        DEBUG_VALUE("RightStick", (Controller)->RightStick);            \
        DEBUG_VALUE("LeftTrigger", (Controller)->LeftTrigger);          \
        DEBUG_VALUE("RightTrigger", (Controller)->RightTrigger);        \
        DEBUG_VALUE("ActionUp", (Controller)->ActionUp.EndedDown);      \
        DEBUG_VALUE("ActionLeft", (Controller)->ActionLeft.EndedDown);  \
        DEBUG_VALUE("ActionDown", (Controller)->ActionDown.EndedDown);  \
        DEBUG_VALUE("ActionRight", (Controller)->ActionRight.EndedDown); \
        DEBUG_VALUE("LeftShoulder1", (Controller)->LeftShoulder1.EndedDown); \
        DEBUG_VALUE("RightShoulder1", (Controller)->RightShoulder1.EndedDown); \
        DEBUG_VALUE("LeftShoulder2", (Controller)->LeftShoulder2.EndedDown); \
        DEBUG_VALUE("RightShoulder2", (Controller)->RightShoulder2.EndedDown); \
        DEBUG_VALUE("Select", (Controller)->Select.EndedDown);          \
        DEBUG_VALUE("Start", (Controller)->Start.EndedDown);            \
        DEBUG_VALUE("LeftClick", (Controller)->LeftClick.EndedDown);    \
        DEBUG_VALUE("RightClick", (Controller)->RightClick.EndedDown);  \
        DEBUG_VALUE("Power", (Controller)->Power.EndedDown);            \
        DEBUG_VALUE("CenterClick", (Controller)->CenterClick.EndedDown); \
    }

// TODO(chris): Actually look at collision geometry and calculate this. Just find point farthest from
// center.
internal r32
CalculateBoundingRadius(entity *Entity)
{
    r32 MaxRadius = 0.0f;
    for(u32 ShapeIndex = 0;
        ShapeIndex < Entity->CollisionShapeCount;
        ++ShapeIndex)
    {
        collision_shape *Shape = Entity->CollisionShapes + ShapeIndex;
        r32 Radius = 0.0f;
        switch(Shape->Type)
        {
            case CollisionShapeType_Circle:
            {
                Radius = Length(Shape->Center) + Shape->Radius;
            } break;

            case CollisionShapeType_Triangle:
            {
                Radius = Maximum(Maximum(Length(Shape->TrianglePointA),
                                         Length(Shape->TrianglePointB)),
                                 Length(Shape->TrianglePointC));
            } break;
        }
        MaxRadius = Maximum(Radius, MaxRadius);
    }
    return(MaxRadius);
}

inline entity *
CreateEntity(game_state *State)
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
    return(Result);
}

inline void
DestroyEntity(game_state *State, entity *Entity)
{
    *Entity = State->Entities[--State->EntityCount];
}

inline collision_shape *
AddCollisionShape(entity *Entity)
{
    collision_shape *Result = 0;
    if(Entity->CollisionShapeCount < ArrayCount(Entity->CollisionShapes))
    {
        Result = Entity->CollisionShapes + Entity->CollisionShapeCount++;
    }
    else
    {
        Assert(!"Added too many collision shapes");
    }
    return(Result);
}

inline void
AddCollisionTriangle(entity *Entity, v2 PointA, v2 PointB, v2 PointC)
{
    collision_shape *Triangle = AddCollisionShape(Entity);
    Triangle->Type = CollisionShapeType_Triangle;
    Triangle->TrianglePointA = PointA;
    Triangle->TrianglePointB = PointB;
    Triangle->TrianglePointC = PointC;
}

inline void
AddCollisionCircle(entity *Entity, r32 Radius, v2 Center = V2(0, 0))
{
    collision_shape *Circle = AddCollisionShape(Entity);
    Circle->Type = CollisionShapeType_Circle;
    Circle->Center = Center;
    Circle->Radius = Radius;
}

inline void
AddCollisionCircleDiameter(entity *Entity, r32 Diameter, v2 Center = V2(0, 0))
{
    collision_shape *Circle = AddCollisionShape(Entity);
    Circle->Type = CollisionShapeType_Circle;
    Circle->Center = Center;
    Circle->Radius = Diameter*0.5f;
}

inline entity *
CreateShip(game_state *State, v3 P, r32 Yaw)
{
    entity *Ship = CreateEntity(State);
    Ship->Type = EntityType_Ship;
    Ship->Dim = V2(10.0f, 10.0f);
    Ship->P = P;
    Ship->Yaw = Yaw;
    Ship->Collides = true;
    v2 HalfDim = Ship->Dim*0.5f;
#if 0
    AddCollisionTriangle(Ship,
                         0.97f*V2(HalfDim.x, 0),
                         0.96f*V2(-HalfDim.x, HalfDim.y),
                         0.69f*V2(-HalfDim.x, 0));
    AddCollisionTriangle(Ship,
                         0.97f*V2(HalfDim.x, 0),
                         0.69f*V2(-HalfDim.x, 0),
                         0.96f*V2(-HalfDim.x, -HalfDim.y));
#else
    AddCollisionCircle(Ship, HalfDim.x, V2(0.0f, 0.0f));
#endif
    return(Ship);
}

inline entity *
CreateFloatingHead(game_state *State)
{
    entity *FloatingHead = CreateEntity(State);
    FloatingHead->Type = EntityType_FloatingHead;
    FloatingHead->P = V3(0.0f, 0.0f, 0.0f);
    FloatingHead->Dim = V2(100.0f, 100.0f);
    FloatingHead->RotationMatrix =
        {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
        };
    FloatingHead->Collides = false;
    return(FloatingHead);
}

inline entity *
CreateAsteroid(game_state *State, v3 P, r32 Radius)
{
    entity *Asteroid = CreateEntity(State);
    Asteroid->Type = EntityType_Asteroid;
    Asteroid->P = P;
    Asteroid->Dim = 2.0f*V2(Radius, Radius);
    Asteroid->Collides = true;
#if 0
    AddCollisionCircle(Asteroid, Radius, V2(2.0f, 0.0f));
#else
    AddCollisionCircle(Asteroid, Radius);
#endif
    return(Asteroid);
}

inline entity *
CreateBullet(game_state *State, v3 P, v3 dP, r32 Yaw)
{
    entity *Bullet = CreateEntity(State);
    Bullet->Type = EntityType_Bullet;
    Bullet->P = P;
    Bullet->dP = dP;
    Bullet->Yaw = Yaw;
    Bullet->Dim = V2(1.5f, 3.0f);
    Bullet->Timer = 2.0f;
    Bullet->Collides = true;
    return(Bullet);
}

inline b32
ProcessIntersection(r32 Intersection, r32 *tMove, r32 *tMax, v2 dP)
{
    b32 Result = false;
    if(HasIntersection(Intersection))
    {
        r32 NormalizedEpsilonTest = LengthSq(Intersection*dP);
        if(-COLLISION_EPSILON < NormalizedEpsilonTest && NormalizedEpsilonTest < COLLISION_EPSILON)
        {
            Intersection = 0.0f;
        }
        if(0.0f <= Intersection && Intersection < *tMax)
        {
            *tMove = Intersection;
            Result = true;
        }
    }
    return(Result);
}

inline b32
ProcessIntersection(circle_ray_intersection_result Intersection, r32 *tMove, r32 *tMax, v2 dP)
{
    b32 Result = false;
    if(HasIntersection(Intersection))
    {
        r32 t = Minimum(Intersection.t1, Intersection.t2);
        r32 NormalizedEpsilonTest = LengthSq(t*dP);
        if(-COLLISION_EPSILON < NormalizedEpsilonTest && NormalizedEpsilonTest < COLLISION_EPSILON)
        {
            t = 0.0f;
        }
        if(0.0f <= t && t < *tMax)
        {
            *tMove = t;
            Result = true;
        }
    }
    return(Result);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
#if TROIDS_INTERNAL
    GlobalDebugState = (debug_state *)GameMemory->DebugMemory;
#endif
    TIMED_FUNCTION();
    game_state *State = (game_state *)GameMemory->PermanentMemory;
    PlatformReadFile = GameMemory->PlatformReadFile;
    PlatformPushThreadWork = GameMemory->PlatformPushThreadWork;

    if(!State->IsInitialized)
    {
        State->IsInitialized = true;

        State->MetersToPixels = 3674.9418959066769192359305459154f;

        State->ShipBitmap = LoadBitmap("ship_opaque.bmp");
        State->AsteroidBitmap = LoadBitmap("asteroid_opaque.bmp");
        State->BulletBitmap = LoadBitmap("bullet.bmp");

        entity *Ship = CreateShip(State, V3(0, 0, 0), 0.0f);
        entity *FloatingHead = CreateFloatingHead(State);

        entity *SmallAsteroid = CreateAsteroid(State, V3(50.0f, 0.0f, 0.0f), 5.0f);
#if 0
        SmallAsteroid->P.x = SmallAsteroid->CollisionShapes[0].Radius +
            Ship->CollisionShapes[0].TrianglePointA.x;
#endif
        entity *SmallAsteroid3 = CreateAsteroid(State, Ship->P + V3(0, 40.0f, 0), 5.0f);
        entity *SmallAsteroid2 = CreateAsteroid(State, Ship->P + V3(0, 20.0f, 0), 5.0f);
//        entity *SmallAsteroid2 = CreateAsteroid(State, V3(Perp(AsteroidStartingP.xy), 0), 10.0f);
        entity *LargeAsteroid = CreateAsteroid(State, SmallAsteroid->P + V3(30.0f, 30.0f, 0.0f), 10.0f);

        State->ShipStartingP = Ship->P;
        State->ShipStartingYaw = Ship->Yaw;
        State->AsteroidStartingP = SmallAsteroid->P;
        
        State->CameraP = V3(Ship->P.xy, 100.0f);
    }

    transient_state *TranState = (transient_state *)GameMemory->TemporaryMemory;
    if(!TranState->IsInitialized)
    {
        InitializeArena(&TranState->TranArena,
                        GameMemory->TemporaryMemorySize - sizeof(transient_state),
                        (u8 *)GameMemory->TemporaryMemory + sizeof(transient_state));

        TranState->RenderBuffer.Arena = SubArena(&TranState->TranArena, Megabytes(1));
        TranState->RenderBuffer.Width = BackBuffer->Width;
        TranState->RenderBuffer.Height = BackBuffer->Height;
        TranState->RenderBuffer.MetersToPixels = State->MetersToPixels;
        TranState->RenderBuffer.Projection = Projection_Orthographic;
        
        State->HeadMesh = LoadObj("head.obj", &TranState->TranArena);

        TranState->IsInitialized = true;
    }
    render_buffer *RenderBuffer = &TranState->RenderBuffer;
    RenderBuffer->CameraP = State->CameraP;
    RenderBuffer->CameraRot = State->CameraRot;
    temporary_memory RenderMemory = BeginTemporaryMemory(&RenderBuffer->Arena);
    PushClear(RenderBuffer, V4(0.1f, 0.1f, 0.1f, 1.0f));

    DEBUG_SUMMARY();
    {DEBUG_GROUP("Debug System");
        DEBUG_MEMORY();
        DEBUG_EVENTS();
    }   
    {DEBUG_GROUP("Memory");
        DEBUG_VALUE("TranArena", TranState->TranArena);
    }
    {DEBUG_GROUP("Profiler");
        DEBUG_FRAME_TIMELINE();
        DEBUG_PROFILER();
    }
    {DEBUG_GROUP("Controllers");
        {DEBUG_GROUP("Keyboard");
            LOG_CONTROLLER(&Input->Keyboard);
        }
        {DEBUG_GROUP("Game Pad 0");
            LOG_CONTROLLER(Input->GamePads + 0);
        }
        {DEBUG_GROUP("Game Pad 1");
            LOG_CONTROLLER(Input->GamePads + 1);
        }
        {DEBUG_GROUP("Game Pad 2");
            LOG_CONTROLLER(Input->GamePads + 2);
        }
        {DEBUG_GROUP("Game Pad 3");
            LOG_CONTROLLER(Input->GamePads + 3);
        }
    }

    game_controller *Keyboard = &Input->Keyboard;
    game_controller *ShipController = Input->GamePads + 0;
    game_controller *AsteroidController = Input->GamePads + 1;

    r32 PixelsToMeters = 1.0f / State->MetersToPixels;
    b32 IsFirstAsteroid = true;

    // NOTE(chris): Pre collision pass
    for(u32 EntityIndex = 0;
        EntityIndex < State->EntityCount;
        )
    {
        u32 NextIndex = EntityIndex + 1;
        entity *Entity = State->Entities + EntityIndex;
        v3 Facing = V3(Cos(Entity->Yaw), Sin(Entity->Yaw), 0.0f);
        switch(Entity->Type)
        {
            case EntityType_Ship:
            {
                if(Keyboard->Start.EndedDown || ShipController->Start.EndedDown)
                {
                    Entity->P = State->ShipStartingP;
                }

                r32 LeftStickX = (Keyboard->LeftStick.x ?
                                  Keyboard->LeftStick.x :
                                  ShipController->LeftStick.x);
                r32 Thrust;
                if(Keyboard->LeftStick.y)
                {
                    Thrust = Clamp01(Keyboard->LeftStick.y);
                }
                else
                {
                    Thrust = Clamp01(ShipController->LeftStick.y);
                    ShipController->LowFrequencyMotor = Thrust;
                }
    
                r32 MaxDYaw = 0.2*Tau;
                Entity->dYaw = Clamp(-MaxDYaw, Entity->dYaw + Input->dtForFrame*-LeftStickX, MaxDYaw);
    
                v3 Acceleration = {};
                if((WentDown(ShipController->RightShoulder1) || WentDown(Keyboard->RightShoulder1)))
                {
                    if(Entity->Timer <= 0.0f)
                    {
                        entity *Bullet = CreateBullet(State, Entity->P, Entity->dP + Facing*100.0f,
                                                      Entity->Yaw);
                        r32 BulletOffset = 0.5f*(Entity->Dim.y + Bullet->Dim.y);
                        Bullet->P += Facing*(BulletOffset);
                        Entity->Timer = 0.1f;
                        Acceleration += -Facing*100.0f;
                    }
                }

#if 0
                Acceleration += 5000.0f*Facing*Thrust;
#else
                Acceleration += 100.0f*Facing*Thrust;
#endif
                Entity->dP += Acceleration*Input->dtForFrame;

                if(Entity->Timer > 0.0f)
                {
                    Entity->Timer -= Input->dtForFrame;
                    ShipController->HighFrequencyMotor = 1.0f;
                }
                else
                {
                    ShipController->HighFrequencyMotor = 0.0f;
                }

            } break;

            case EntityType_Asteroid:
            {
                if(IsFirstAsteroid)
                {
                    IsFirstAsteroid = false;
                    if(WentDown(ShipController->Select))
                    {
                        // NOTE(chris): Halving volume results in dividing radius by cuberoot(2)
                        Entity->Dim *= 0.79370052598f;
                        entity *NewAsteroid = CreateAsteroid(State, Entity->P, Entity->Dim.x);

                        r32 Angle = Tau/12.0f;
                        Entity->dP = RotateZ(Entity->dP, -Angle);
                        NewAsteroid->dP = RotateZ(NewAsteroid->dP, Angle);
                    }

                    if(AsteroidController->Start.EndedDown)
                    {
                        Entity->P = State->AsteroidStartingP;
                    }
#if 0
                    v3 Acceleration = (5000.0f*V3(AsteroidController->LeftStick.x,
                                                  AsteroidController->LeftStick.y,
                                                  0.0f));
#else
                    v3 Acceleration = (100.0f*V3(AsteroidController->LeftStick.x,
                                                  AsteroidController->LeftStick.y,
                                                  0.0f));
#endif

                    Entity->dP += Acceleration*Input->dtForFrame;
                }
            } break;

            case EntityType_Bullet:
            {
                if(Entity->Timer <= 0.0f)
                {
                    DestroyEntity(State, Entity);
                    NextIndex = EntityIndex;
                }
                else
                {
                    Entity->Timer -= Input->dtForFrame;
                }
            } break;

            case EntityType_FloatingHead:
            {
#if 0
                if(Keyboard->ActionUp.EndedDown || ShipController->ActionUp.EndedDown)
                {
                    Entity->P.z += 100.0f*Input->dtForFrame;
                }
                if(Keyboard->ActionDown.EndedDown || ShipController->ActionDown.EndedDown)
                {
                    Entity->P.z -= 100.0f*Input->dtForFrame;
                }
#endif
                r32 YRotation = Input->dtForFrame*(Keyboard->RightStick.x ?
                                                   Keyboard->RightStick.x :
                                                   ShipController->RightStick.x);
                r32 XRotation = Input->dtForFrame*(Keyboard->RightStick.y ?
                                                   Keyboard->RightStick.y :
                                                   ShipController->RightStick.y);
                r32 ZRotation = Input->dtForFrame*((Keyboard->LeftTrigger ?
                                                    Keyboard->LeftTrigger :
                                                    ShipController->LeftTrigger) -
                                                   (Keyboard->RightTrigger ?
                                                    Keyboard->RightTrigger :
                                                    ShipController->RightTrigger));

                r32 CosXRotation = Cos(XRotation);
                r32 SinXRotation = Sin(XRotation);
                r32 CosYRotation = Cos(YRotation);
                r32 SinYRotation = Sin(YRotation);
                r32 CosZRotation = Cos(ZRotation);
                r32 SinZRotation = Sin(ZRotation);

                m33 XRotationMatrix =
                    {
                        1, 0,             0,
                        0, CosXRotation, -SinXRotation,
                        0, SinXRotation,  CosXRotation,
                    };
                m33 YRotationMatrix =
                    {
                        CosYRotation, 0, SinYRotation,
                        0,            1, 0,
                        -SinYRotation, 0, CosYRotation,
                    };
                m33 ZRotationMatrix =
                    {
                        CosZRotation, -SinZRotation, 0,
                        SinZRotation,  CosZRotation, 0,
                        0,             0,            1,
                    };

                Entity->RotationMatrix =
                    ZRotationMatrix*YRotationMatrix*XRotationMatrix*Entity->RotationMatrix;
            } break;

            InvalidDefaultCase;
        }
        if(Entity->Collides)
        {
            Entity->BoundingRadius = CalculateBoundingRadius(Entity);
#if DEBUG_COLLISION
            Entity->BoundingCircleCollided = false;
            for(u32 ShapeIndex = 0;
                ShapeIndex < Entity->CollisionShapeCount;
                ++ShapeIndex)
            {
                collision_shape *Shape = Entity->CollisionShapes + ShapeIndex;
                Shape->Collided = false;
            }
#endif
        }
        EntityIndex = NextIndex;
    }

    // NOTE(chris): Collision pass
    for(u32 EntityIndex = 0;
        EntityIndex < State->EntityCount;
        ++EntityIndex)
    {
        entity *Entity = State->Entities + EntityIndex;
        r32 tMax = 1.0f;
        if(Entity->Collides)
        {
            for(u32 CollisionIndex = 0;
                CollisionIndex < COLLISION_ITERATIONS;
                ++CollisionIndex)
            {
                entity *CollidedWith = 0;
                collision_shape *CollidingShape = 0;
                collision_shape *OtherCollidingShape = 0;
                v3 PointOfIntersection = {};
                v3 OldP = Entity->P;
                v3 NewP = OldP + Input->dtForFrame*Entity->dP;
                v3 dP = NewP - OldP;
                r32 OldYaw = Entity->Yaw;
                r32 NewYaw = OldYaw + Input->dtForFrame*Entity->dYaw;
                r32 dYaw = NewYaw - OldYaw;
                r32 tMove = tMax;
                if(dP.x != 0 || dP.y != 0)
                {
                    r32 InvdPY = 1.0f / dP.y;
                    for(u32 OtherEntityIndex = 0;
                        OtherEntityIndex < State->EntityCount;
                        ++OtherEntityIndex)
                    {
                        entity *OtherEntity = State->Entities + OtherEntityIndex;
                        if(!OtherEntity->Collides || (OtherEntityIndex == EntityIndex)) continue;

                        r32 t1 = -1.0f;
                        r32 t2 = t1;
                        {
                            r32 HitRadius = OtherEntity->BoundingRadius + Entity->BoundingRadius;

                            circle_ray_intersection_result Intersection =
                                CircleRayIntersection(OtherEntity->P.xy, HitRadius, OldP.xy, NewP.xy);

                            if(HasIntersection(Intersection))
                            {
                                t1 = Clamp(0.0f, Intersection.t1, tMax);
                                t2 = Clamp(0.0f, Intersection.t2, tMax);
                            }
                        }

                        if(t1 != t2)
                        {
#if DEBUG_COLLISION
                            Entity->BoundingCircleCollided = true;
                            OtherEntity->BoundingCircleCollided = true;
#endif
                            // TODO(chris): Search actual collision shapes
                            for(u32 CollisionShapeIndex = 0;
                                CollisionShapeIndex < Entity->CollisionShapeCount;
                                ++CollisionShapeIndex)
                            {
                                for(u32 OtherCollisionShapeIndex = 0;
                                    OtherCollisionShapeIndex < OtherEntity->CollisionShapeCount;
                                    ++OtherCollisionShapeIndex)
                                {
                                    collision_shape *CollisionShape = Entity->CollisionShapes + CollisionShapeIndex;
                                    collision_shape *OtherCollisionShape = OtherEntity->CollisionShapes + OtherCollisionShapeIndex;

                                    if(CollisionShape->Type == CollisionShapeType_Triangle &&
                                       OtherCollisionShape->Type == CollisionShapeType_Triangle)
                                    {
                                        NotImplemented;
                                    }
                                    else if(CollisionShape->Type == CollisionShapeType_Circle &&
                                            OtherCollisionShape->Type == CollisionShapeType_Circle)
                                    {
                                        r32 HitRadius = CollisionShape->Radius + OtherCollisionShape->Radius;
                                        v2 HitCenter = (OtherEntity->P.xy +
                                                        RotateZ(OtherCollisionShape->Center, OtherEntity->Yaw)
                                                        - RotateZ(CollisionShape->Center, Entity->Yaw));

                                        r32 OverlapDelta = LengthSq(OldP.xy - HitCenter) - (HitRadius*HitRadius);
                                        b32 Overlapping = OverlapDelta < -COLLISION_EPSILON;
//                                        Assert(!Overlapping);

                                        circle_ray_intersection_result Intersection =
                                                                         CircleRayIntersection(HitCenter, HitRadius, OldP.xy, NewP.xy);

                                        if(ProcessIntersection(Intersection, &tMove, &tMax, NewP.xy - OldP.xy))
                                        {
                                            CollidedWith = OtherEntity;
                                            CollidingShape = CollisionShape;
                                            OtherCollidingShape = OtherCollisionShape;
                                            PointOfIntersection = OldP + dP*tMove;
                                        }
                                    }
                                    else if((CollisionShape->Type == CollisionShapeType_Triangle &&
                                             OtherCollisionShape->Type == CollisionShapeType_Circle) ||
                                            (CollisionShape->Type == CollisionShapeType_Circle &&
                                             OtherCollisionShape->Type == CollisionShapeType_Triangle))
                                    {
                                        v3 Start;
                                        v3 End;
                                        r32 Radius;
                                        v2 A, B, C;
                        
                                        if(CollisionShape->Type == CollisionShapeType_Triangle)
                                        {
                                            Start = OtherEntity->P;
                                            End = OtherEntity->P - Input->dtForFrame*Entity->dP;
                                            Radius = OtherCollisionShape->Radius;
                                            v2 CenterOffset = RotateZ(OtherCollisionShape->Center, OtherEntity->Yaw);
                                            A = Entity->P.xy + RotateZ(CollisionShape->TrianglePointA, Entity->Yaw) -
                                                CenterOffset;
                                            B = Entity->P.xy + RotateZ(CollisionShape->TrianglePointB, Entity->Yaw) -
                                                CenterOffset;
                                            C = Entity->P.xy + RotateZ(CollisionShape->TrianglePointC, Entity->Yaw) -
                                                CenterOffset;
                                        }
                                        else
                                        {
                                            Start = OldP;
                                            End = NewP;
                                            Radius = CollisionShape->Radius;
                                            v2 CenterOffset = RotateZ(CollisionShape->Center, Entity->Yaw);
                                            A = OtherEntity->P.xy +
                                                RotateZ(OtherCollisionShape->TrianglePointA, OtherEntity->Yaw) -
                                                CenterOffset;
                                            B = OtherEntity->P.xy +
                                                RotateZ(OtherCollisionShape->TrianglePointB, OtherEntity->Yaw) -
                                                CenterOffset;
                                            C = OtherEntity->P.xy +
                                                RotateZ(OtherCollisionShape->TrianglePointC, OtherEntity->Yaw) -
                                                CenterOffset;
                                        }
                        
                                        v2 AB = B - A;
                                        v2 BC = C - B;
                                        v2 CA = A - C;
                                        r32 ABLength = Length(AB);
                                        r32 BCLength = Length(BC);
                                        r32 CALength = Length(CA);
                                        r32 InvABLength = 1.0f/ABLength;
                                        r32 InvBCLength = 1.0f/BCLength;
                                        r32 InvCALength = 1.0f/CALength;
                                        v2 ABTranslate = -Perp(AB)*InvABLength*Radius;
                                        v2 BCTranslate = -Perp(BC)*InvBCLength*Radius;
                                        v2 CATranslate = -Perp(CA)*InvCALength*Radius;
                                        v2 ABHitA = A + ABTranslate;
                                        v2 ABHitB = B + ABTranslate;
                                        v2 BCHitB = B + BCTranslate;
                                        v2 BCHitC = C + BCTranslate;
                                        v2 CAHitC = C + CATranslate;
                                        v2 CAHitA = A + CATranslate;

                                        circle_ray_intersection_result IntersectionA =
                                            CircleRayIntersection(A, Radius, Start.xy, End.xy);
                                        circle_ray_intersection_result IntersectionB =
                                            CircleRayIntersection(B, Radius, Start.xy, End.xy);
                                        circle_ray_intersection_result IntersectionC =
                                            CircleRayIntersection(C, Radius, Start.xy, End.xy);

                                        r32 IntersectionAB = SegmentRayIntersection(ABHitA, ABHitB, Start.xy, End.xy);
                                        r32 IntersectionBC = SegmentRayIntersection(BCHitB, BCHitC, Start.xy, End.xy);
                                        r32 IntersectionCA = SegmentRayIntersection(CAHitC, CAHitA, Start.xy, End.xy);

                                        b32 IntersectionAUpdated = ProcessIntersection(IntersectionA, &tMove, &tMax, (End-Start).xy);
                                        b32 IntersectionBUpdated = ProcessIntersection(IntersectionB, &tMove, &tMax, (End-Start).xy);
                                        b32 IntersectionCUpdated = ProcessIntersection(IntersectionC, &tMove, &tMax, (End-Start).xy);
                                        b32 IntersectionABUpdated = ProcessIntersection(IntersectionAB, &tMove, &tMax, (End-Start).xy);
                                        b32 IntersectionBCUpdated = ProcessIntersection(IntersectionBC, &tMove, &tMax, (End-Start).xy);
                                        b32 IntersectionCAUpdated = ProcessIntersection(IntersectionCA, &tMove, &tMax, (End-Start).xy);

                                        if(IntersectionAUpdated || IntersectionBUpdated || IntersectionCUpdated ||
                                           IntersectionABUpdated || IntersectionBCUpdated || IntersectionCAUpdated)
                                        {
                                            CollidedWith = OtherEntity;
                                            CollidingShape = CollisionShape;
                                            OtherCollidingShape = OtherCollisionShape;
                                            PointOfIntersection = Start + (End-Start)*tMove;
                                        }
                                    }
                                    else
                                    {
                                        NotImplemented;
                                    }
                                }
                            }
                        }
                    }
                    dP = Entity->dP*Input->dtForFrame*tMove;
                    Entity->P += dP;
                    NewP = Entity->P;
                    tMax -= tMove;

                    if(CollidedWith)
                    {
                        // TODO(chris): Collision resolution
#if DEBUG_COLLISION
                        CollidingShape->Collided = true;
                        OtherCollidingShape->Collided = true;
#endif
                        switch(OtherCollidingShape->Type)
                        {
                            case CollisionShapeType_Circle:
                            {
                                v3 RV = CollidedWith->P + V3(OtherCollidingShape->Center, 0) -
                                    PointOfIntersection;
                                v3 Adjustment = (-Inner(Entity->dP, RV)/LengthSq(RV))*RV;
                                Entity->dP += Adjustment;
                            } break;
                        }
                    }
                }
                if(dYaw)
                {
                    // TODO(chris): Detect continuous collisions through rotation. Yikes.
                    dYaw = Entity->dYaw*Input->dtForFrame*tMove;
                    Entity->Yaw += dYaw;
                    NewYaw = Entity->Yaw;
                }

                {
#if DEBUG_COLLISION
#define BOUNDING_EXTRUSION_Z_OFFSET -0.0004f
#define BOUNDING_Z_OFFSET -0.0002f
#define SHAPE_EXTRUSION_Z_OFFSET 0.0002f
#define SHAPE_Z_OFFSET 0.0004f
#if 0
                    PushLine(RenderBuffer,
                             Entity->P + V3(0, 0, 0.1f),
                             Entity->P + Entity->dP + V3(0, 0, 0.1f),
                             V4(0, 0, 1, 0));
#endif
                    v3 BoundingExtrusionOffset = V3(0, 0, BOUNDING_EXTRUSION_Z_OFFSET);
                    v3 BoundingCircleOffset = V3(0, 0, BOUNDING_Z_OFFSET);
                    v3 ShapeExtrusionOffset = V3(0, 0, SHAPE_EXTRUSION_Z_OFFSET);
                    v3 ShapeOffset = V3(0, 0, SHAPE_Z_OFFSET);
            
                    v4 BoundingColor = Entity->BoundingCircleCollided ? V4(1, 0, 0, 1) : V4(1, 0, 1, 1);
                    v4 BoundingExtrusionColor = Entity->BoundingCircleCollided ? V4(0.8f, 0, 0, 1) : V4(0.8f, 0, 0.8f, 1);
                    PushCircle(RenderBuffer, OldP + BoundingCircleOffset, Entity->BoundingRadius, BoundingColor);
                    PushCircle(RenderBuffer, NewP + BoundingCircleOffset, Entity->BoundingRadius, BoundingColor);

                    r32 dPLength = Length(dP.xy);
                    r32 InvdPLength = 1.0f / dPLength;
                    v2 XAxis = InvdPLength*-Perp(dP.xy);
                    v2 YAxis = Perp(XAxis);

                    v2 BoundingDim = V2(2.0f*Entity->BoundingRadius, dPLength);
                    PushRotatedRectangle(RenderBuffer, 0.5f*(OldP + NewP) + BoundingExtrusionOffset,
                                         XAxis, YAxis, BoundingDim, BoundingExtrusionColor);
#if 1
                    for(u32 ShapeIndex = 0;
                        ShapeIndex < Entity->CollisionShapeCount;
                        ++ShapeIndex)
                    {
                        collision_shape *Shape = Entity->CollisionShapes + ShapeIndex;
                        v4 ShapeColor = Shape->Collided ? V4(0, 1, 0, 1) : V4(0, 1, 1, 1);
                        v4 ShapeExtrusionColor = Shape->Collided ? V4(0, 0.8f, 0, 1) : V4(0, 0.8f, 0.8f, 1);
                        switch(Shape->Type)
                        {
                            case CollisionShapeType_Circle:
                            {
                                v3 Center = V3(Shape->Center, 0);
                                v3 RotatedOldCenter = RotateZ(Center, OldYaw);
                                v3 RotatedNewCenter = RotateZ(Center, NewYaw);
                                PushCircle(RenderBuffer, OldP + RotatedOldCenter  + ShapeOffset,
                                           Shape->Radius, ShapeColor);
                                PushCircle(RenderBuffer, NewP + RotatedNewCenter + ShapeOffset,
                                           Shape->Radius, ShapeColor);
                                v2 ShapeDim = V2(2.0f*Shape->Radius, dPLength);
                                PushRotatedRectangle(RenderBuffer,
                                                     RotatedOldCenter + 0.5f*(OldP + NewP) + ShapeExtrusionOffset,
                                                     XAxis, YAxis, ShapeDim, ShapeExtrusionColor);
                            } break;

                            case CollisionShapeType_Triangle:
                            {
                                v3 A = V3(Shape->TrianglePointA, 0);
                                v3 B = V3(Shape->TrianglePointB, 0);
                                v3 C = V3(Shape->TrianglePointC, 0);
                                A = RotateZ(A, Entity->Yaw);
                                B = RotateZ(B, Entity->Yaw);
                                C = RotateZ(C, Entity->Yaw);
                                PushTriangle(RenderBuffer,
                                             OldP + A + ShapeOffset,
                                             OldP + B + ShapeOffset,
                                             OldP + C + ShapeOffset,
                                             ShapeColor);
                                PushTriangle(RenderBuffer,
                                             NewP + A + ShapeOffset,
                                             NewP + B + ShapeOffset,
                                             NewP + C + ShapeOffset,
                                             ShapeColor);
                            } break;
                        }
                    }
#endif
#endif
                }


                if(tMax <= 0.0f)
                {
                    break;
                }
            }
        }
    }

    if(Keyboard->ActionUp.EndedDown || ShipController->ActionUp.EndedDown)
    {
        State->CameraP.z += 100.0f*Input->dtForFrame;
    }

    if(Keyboard->ActionDown.EndedDown || ShipController->ActionDown.EndedDown)
    {
        State->CameraP.z -= 100.0f*Input->dtForFrame;
    }

    // NOTE(chris): Post collision pass
    for(u32 EntityIndex = 0;
        EntityIndex < State->EntityCount;
        ++EntityIndex)
    {
        entity *Entity = State->Entities + EntityIndex;
        
        v3 Facing = V3(Cos(Entity->Yaw), Sin(Entity->Yaw), 0.0f);
        v2 YAxis = Facing.xy;
        v2 XAxis = -Perp(YAxis);
        switch(Entity->Type)
        {
            case EntityType_Ship:
            {
//                State->CameraP.xy += 0.2f*(Entity->P.xy - State->CameraP.xy);
//                State->CameraRot += 0.05f*((Entity->Yaw - 0.25f*Tau) - State->CameraRot);
                PushBitmap(RenderBuffer, &State->ShipBitmap, Entity->P,
                           XAxis, YAxis, Entity->Dim);
            } break;

            case EntityType_Asteroid:
            {
                PushBitmap(RenderBuffer, &State->AsteroidBitmap, Entity->P,
                           XAxis, YAxis, Entity->Dim);
            } break;

            case EntityType_Bullet:
            {
                PushBitmap(RenderBuffer, &State->BulletBitmap, Entity->P, XAxis, YAxis,
                           Entity->Dim,
                           V4(1.0f, 1.0f, 1.0f, Unlerp(0.0f, Entity->Timer, 2.0f)));
//            DrawLine(BackBuffer, State->P, Bullet->P, V4(0.0f, 0.0f, 1.0f, 1.0f));
            } break;

            case EntityType_FloatingHead:
            {
#if 0
                for(u32 FaceIndex = 1;
                    FaceIndex < State->HeadMesh.FaceCount;
                    ++FaceIndex)
                {
                    obj_face *Face = State->HeadMesh.Faces + FaceIndex;
                    v3 VertexA = (State->HeadMesh.Vertices + Face->VertexIndexA)->xyz;
                    v3 VertexB = (State->HeadMesh.Vertices + Face->VertexIndexB)->xyz;
                    v3 VertexC = (State->HeadMesh.Vertices + Face->VertexIndexC)->xyz;

                    v4 White = V4(1.0f, 1.0f, 1.0f, 1.0f);
                    PushLine(RenderBuffer, Entity->P, Entity->RotationMatrix,
                             VertexA, VertexB, Entity->Dim, White);
                    PushLine(RenderBuffer, Entity->P, Entity->RotationMatrix,
                             VertexB, VertexC, Entity->Dim, White);
                    PushLine(RenderBuffer, Entity->P, Entity->RotationMatrix,
                             VertexC, VertexA, Entity->Dim, White);
                }
#endif
            } break;

            InvalidDefaultCase;
        }
    }

#if 0
    {
        v2 P = V2(0, BackBuffer->Height*0.5f);
        for(char GlyphIndex = 'A';
            GlyphIndex <= 'Z';
            ++GlyphIndex)
        {
            loaded_bitmap *Glyph = GameMemory->DebugFont.Glyphs + GlyphIndex;
            if(Glyph->Memory)
            {
                v2 YAxis = V2(0, 1);
                v2 XAxis = -Perp(YAxis);
                r32 Scale = (r32)Glyph->Height;
#if 0
                PushRectangle(BackBuffer, CenterDim(P, Scale*(XAxis + YAxis)),
                              V4(1.0f, 0.0f, 1.0f, 1.0f));
#endif
                PushBitmap(RenderBuffer, Glyph, P, XAxis, YAxis, Scale);
                P.x += XAxis.x*Scale;
            }
        }
    }
#endif
    {
        TIMED_BLOCK("RenderGame");
        RenderBufferToBackBuffer(RenderBuffer, BackBuffer);
    }
    EndTemporaryMemory(RenderMemory);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
#if 0
    r32 Frequency = 440.0f;
    u32 SamplesPerPeriod = (u32)((r32)SoundBuffer->SamplesPerSecond / Frequency + 0.5f);
    r32 HalfSamplesPerPeriod = 0.5f*SamplesPerPeriod;
    u32 SampleSize = SoundBuffer->Channels*SoundBuffer->BitsPerSample / 8;
    u32 BufferSizeInSamples = SoundBuffer->Size/SampleSize;

    // NOTE(chris): Assumes sample size
    s16 Amplitude = 400;
    s16 *Sample = (s16 *)SoundBuffer->Region1;
    for(u32 SampleIndex = 0;
        SampleIndex < SoundBuffer->Region1Size;
        SampleIndex += SampleSize)
    {
        s16 Value = (State->RunningSampleCount++ < HalfSamplesPerPeriod) ? Amplitude : -Amplitude;
        if(State->RunningSampleCount == SamplesPerPeriod)
        {
            State->RunningSampleCount = 0;
        }
        for(u8 Channel = 0;
            Channel < SoundBuffer->Channels;
            ++Channel)
        {
            *Sample++ = Value;
        }
    }
    Sample = (s16 *)SoundBuffer->Region2;
    for(u32 SampleIndex = 0;
        SampleIndex < SoundBuffer->Region2Size;
        SampleIndex += SampleSize)
    {
        s16 Value = (State->RunningSampleCount++ < HalfSamplesPerPeriod) ? Amplitude : -Amplitude;
        if(State->RunningSampleCount == SamplesPerPeriod)
        {
            State->RunningSampleCount = 0;
        }
        for(u8 Channel = 0;
            Channel < SoundBuffer->Channels;
            ++Channel)
        {
            *Sample++ = Value;
        }
    }
#endif
}

#if TROIDS_INTERNAL
#include "troids_debug.cpp"
#endif
