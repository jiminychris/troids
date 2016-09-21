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
#include "troids_physics.cpp"
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
                Radius = Maximum(Maximum(Length(Shape->A),
                                         Length(Shape->B)),
                                 Length(Shape->C));
            } break;
        }
        MaxRadius = Maximum(Radius, MaxRadius);
    }
    r32 Result = MaxRadius + 1.0f;
    return(Result);
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
    Triangle->A = PointA;
    Triangle->B = PointB;
    Triangle->C = PointC;
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
#if 0
    AddCollisionTriangle(Ship,
                         4.0f*V2(HalfDim.x, 0),
                         4.0f*V2(-HalfDim.x, HalfDim.y),
                         4.0f*V2(-HalfDim.x, -HalfDim.y));
#else
    AddCollisionCircle(Ship, Ship->Dim.x, V2(Ship->Dim.x, 0));
#endif
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
            Ship->CollisionShapes[0].A.x;
#endif
        entity *SmallAsteroid3 = CreateAsteroid(State, Ship->P + V3(0, 40.0f, 0), 5.0f);
        entity *SmallAsteroid2 = CreateAsteroid(State, Ship->P + V3(0, 28.0f, 0), 5.0f);
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
        DEBUG_VALUE("Tran Arena", TranState->TranArena);
        DEBUG_VALUE("String Arena", GlobalDebugState->StringArena);
        DEBUG_VALUE("Render Arena", TranState->RenderBuffer.Arena);
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
                // TODO(chris): IMPORTANT Now that I have a pretty clear grasp of what needs to be done,
                // work on angular collision next!
//                Entity->dYaw = Clamp(-MaxDYaw, Entity->dYaw + Input->dtForFrame*-LeftStickX, MaxDYaw);
    
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
#if COLLISION_DEBUG
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
                collision Collision = {};
                v3 OldP = Entity->P;
                v3 NewP = OldP + Input->dtForFrame*Entity->dP;
                v3 dP = NewP - OldP;
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

                        b32 BoundingBoxesOverlap;
                        {
                            r32 HitRadius = OtherEntity->BoundingRadius + Entity->BoundingRadius;

                            BoundingBoxesOverlap = (Square(HitRadius) >=
                                                    LengthSq(OtherEntity->P - Entity->P));
                        }

                        if(BoundingBoxesOverlap)
                        {
#if COLLISION_DEBUG_LINEAR
                            Entity->BoundingCircleCollided = true;
                            OtherEntity->BoundingCircleCollided = true;
#endif
                            for(u32 ShapeIndex = 0;
                                ShapeIndex < Entity->CollisionShapeCount;
                                ++ShapeIndex)
                            {
                                for(u32 OtherShapeIndex = 0;
                                    OtherShapeIndex < OtherEntity->CollisionShapeCount;
                                    ++OtherShapeIndex)
                                {
                                    collision_shape *Shape = Entity->CollisionShapes + ShapeIndex;
                                    collision_shape *OtherShape = OtherEntity->CollisionShapes + OtherShapeIndex;

                                    switch(Shape->Type&OtherShape->Type)
                                    {
                                        case CollisionShapePair_TriangleTriangle:
                                        {
                                            NotImplemented;
                                        } break;

                                        case CollisionShapePair_CircleCircle:
                                        {
                                            v2 Start = Entity->P.xy + RotateZ(Shape->Center, Entity->Yaw);
                                            v2 End = Start + Input->dtForFrame*Entity->dP.xy;
                                            v2 dMove = End - Start;
                                            r32 HitRadius = Shape->Radius + OtherShape->Radius;
                                            v2 HitCenter = (OtherEntity->P.xy +
                                                            RotateZ(OtherShape->Center, OtherEntity->Yaw));

                                            circle_ray_intersection_result Intersection =
                                                                             CircleRayIntersection(HitCenter, HitRadius, Start, End);

                                            if(ProcessIntersection(Intersection, &tMove, tMax, dMove))
                                            {
                                                Collision.Type = CollisionType_Circle;
                                                Collision.Deflection = Start - HitCenter;

                                                CollidedWith = OtherEntity;
                                                CollidingShape = Shape;
                                                OtherCollidingShape = OtherShape;
                                            }
                                        } break;

                                        case CollisionShapePair_TriangleCircle:
                                        {
                                            v2 Start;
                                            v2 End;
                                            r32 Radius;
                                            v2 A, B, C;
                        
                                            if(Shape->Type == CollisionShapeType_Triangle)
                                            {
                                                Start = OtherEntity->P.xy + RotateZ(OtherShape->Center, OtherEntity->Yaw);
                                                End = Start - Input->dtForFrame*Entity->dP.xy;
                                                Radius = OtherShape->Radius;
                                                A = Entity->P.xy + RotateZ(Shape->A, Entity->Yaw);
                                                B = Entity->P.xy + RotateZ(Shape->B, Entity->Yaw);
                                                C = Entity->P.xy + RotateZ(Shape->C, Entity->Yaw);
                                            }
                                            else
                                            {
                                                Start = Entity->P.xy + RotateZ(Shape->Center, Entity->Yaw);
                                                End = Start + Input->dtForFrame*Entity->dP.xy;
                                                Radius = Shape->Radius;
                                                A = OtherEntity->P.xy +
                                                    RotateZ(OtherShape->A, OtherEntity->Yaw);
                                                B = OtherEntity->P.xy +
                                                    RotateZ(OtherShape->B, OtherEntity->Yaw);
                                                C = OtherEntity->P.xy +
                                                    RotateZ(OtherShape->C, OtherEntity->Yaw);
                                            }

                                            v2 dMove = End - Start;

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
                                                CircleRayIntersection(A, Radius, Start, End);
                                            circle_ray_intersection_result IntersectionB =
                                                CircleRayIntersection(B, Radius, Start, End);
                                            circle_ray_intersection_result IntersectionC =
                                                CircleRayIntersection(C, Radius, Start, End);

                                            r32 IntersectionAB = TriangleEdgeRayIntersection(ABHitA, ABHitB, Start, End, C);
                                            r32 IntersectionBC = TriangleEdgeRayIntersection(BCHitB, BCHitC, Start, End, A);
                                            r32 IntersectionCA = TriangleEdgeRayIntersection(CAHitC, CAHitA, Start, End, B);

                                            b32 IntersectionAUpdated = ProcessIntersection(IntersectionA, &tMove, tMax, dMove);
                                            b32 IntersectionBUpdated = ProcessIntersection(IntersectionB, &tMove, tMax, dMove);
                                            b32 IntersectionCUpdated = ProcessIntersection(IntersectionC, &tMove, tMax, dMove);
                                            b32 IntersectionABUpdated = ProcessIntersection(IntersectionAB, &tMove, tMax, dMove);
                                            b32 IntersectionBCUpdated = ProcessIntersection(IntersectionBC, &tMove, tMax, dMove);
                                            b32 IntersectionCAUpdated = ProcessIntersection(IntersectionCA, &tMove, tMax, dMove);

                                            b32 Updated = (IntersectionAUpdated || IntersectionBUpdated ||
                                                           IntersectionCUpdated || IntersectionABUpdated ||
                                                           IntersectionBCUpdated || IntersectionCAUpdated);

                                            if(Updated)
                                            {
                                                if(OtherShape->Type == CollisionShapeType_Circle)
                                                {
                                                    if(IntersectionAUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Circle;
                                                        Collision.Deflection = A - Start;
                                                    }
                                                    if(IntersectionBUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Circle;
                                                        Collision.Deflection = B - Start;
                                                    }
                                                    if(IntersectionCUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Circle;
                                                        Collision.Deflection = C - Start;
                                                    }
                                                    if(IntersectionABUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Line;
                                                        Collision.A = B;
                                                        Collision.B = A;
                                                    }
                                                    if(IntersectionBCUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Line;
                                                        Collision.A = C;
                                                        Collision.B = B;
                                                    }
                                                    if(IntersectionCAUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Line;
                                                        Collision.A = A;
                                                        Collision.B = C;
                                                    }
                                                }
                                                else
                                                {
                                                    if(IntersectionAUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Circle;
                                                        Collision.Deflection = Start - A;
                                                    }
                                                    if(IntersectionBUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Circle;
                                                        Collision.Deflection = Start - B;
                                                    }
                                                    if(IntersectionCUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Circle;
                                                        Collision.Deflection = Start - C;
                                                    }
                                                    if(IntersectionABUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Line;
                                                        Collision.A = A;
                                                        Collision.B = B;
                                                    }
                                                    if(IntersectionBCUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Line;
                                                        Collision.A = B;
                                                        Collision.B = C;
                                                    }
                                                    if(IntersectionCAUpdated)
                                                    {
                                                        Collision.Type = CollisionType_Line;
                                                        Collision.A = C;
                                                        Collision.B = A;
                                                    }
                                                }

                                                CollidedWith = OtherEntity;
                                                CollidingShape = Shape;
                                                OtherCollidingShape = OtherShape;
                                            }
                                        } break;

                                        default:
                                        {
                                            NotImplemented;
                                        } break;
                                    }
                                }
                            }
                        }
                    }
#if 0
                    if(tMove != tMax)
                    {
                        v3 dP = Entity->dP*Input->dtForFrame*tMove;
                        r32 Root = LengthSq(dP)-COLLISION_EPSILON;
                        r32 Len = Length(dP);
                        if(Root >= 0 && Len >= 0)
                        {
                            tMove *= SquareRoot(Root)/Len;
                        }
                    }
#endif
                    Entity->P += Entity->dP*Input->dtForFrame*tMove;
                    tMax -= tMove;
                    
                    if(CollidedWith)
                    {
                        // TODO(chris): Collision resolution
#if COLLISION_DEBUG_LINEAR
                        CollidingShape->Collided = true;
                        OtherCollidingShape->Collided = true;
#endif
                    }
                    switch(Collision.Type)
                    {
                        case CollisionType_Circle:
                        {
                            r32 DeflectionMagnitudeSq = LengthSq(Collision.Deflection);
                            if(DeflectionMagnitudeSq > 0.0f)
                            {
                                v2 Adjustment = (-Collision.Deflection *
                                                 (Inner(Entity->dP.xy, Collision.Deflection) /
                                                  DeflectionMagnitudeSq));
                                Assert(!IsNaN(Adjustment.x));
                                Assert(!IsNaN(Adjustment.y));
                                Entity->dP += 1.01f*V3(Adjustment, 0);
                            }
                        } break;

                        case CollisionType_Line:
                        {
                            v2 DeflectionVector = Perp(Collision.A - Collision.B);
                            r32 DeflectionMagnitudeSq = LengthSq(DeflectionVector);
                            if(DeflectionMagnitudeSq > 0.0f)
                            {
                                v2 Adjustment = (-DeflectionVector *
                                                 (Inner(Entity->dP.xy, DeflectionVector) /
                                                  DeflectionMagnitudeSq));
                                Assert(!IsNaN(Adjustment.x));
                                Assert(!IsNaN(Adjustment.y));
                                Entity->dP += 1.01f*V3(Adjustment, 0);
                            }
                        } break;

                        case CollisionType_None:
                        {
                        } break;

                        InvalidDefaultCase;
                    }
                }

                CollidedWith = 0;
                Collision.Type = CollisionType_None;
                r32 OldYaw = Entity->Yaw;
                r32 NewYaw = OldYaw + Input->dtForFrame*Entity->dYaw;
                r32 dYaw = NewYaw - OldYaw;
                if(dYaw)
                {
                    r32 tYawMax = tMove;
                    for(u32 OtherEntityIndex = 0;
                        OtherEntityIndex < State->EntityCount;
                        ++OtherEntityIndex)
                    {
                        entity *OtherEntity = State->Entities + OtherEntityIndex;
                        if(!OtherEntity->Collides || (OtherEntityIndex == EntityIndex)) continue;

                        b32 BoundingBoxesOverlap;
                        {
                            r32 HitRadius = OtherEntity->BoundingRadius + Entity->BoundingRadius;

                            BoundingBoxesOverlap = (HitRadius*HitRadius >=
                                                    LengthSq(OtherEntity->P - Entity->P));
                        }

                        if(BoundingBoxesOverlap)
                        {
#if COLLISION_DEBUG_ANGULAR
                            Entity->BoundingCircleCollided = true;
                            OtherEntity->BoundingCircleCollided = true;
#endif
                            for(u32 ShapeIndex = 0;
                                ShapeIndex < Entity->CollisionShapeCount;
                                ++ShapeIndex)
                            {
                                for(u32 OtherShapeIndex = 0;
                                    OtherShapeIndex < OtherEntity->CollisionShapeCount;
                                    ++OtherShapeIndex)
                                {
                                    collision_shape *Shape = Entity->CollisionShapes + ShapeIndex;
                                    collision_shape *OtherShape = OtherEntity->CollisionShapes + OtherShapeIndex;

                                    switch(Shape->Type&OtherShape->Type)
                                    {
                                        case CollisionShapePair_TriangleTriangle:
                                        {
                                            NotImplemented;
                                        } break;

                                        case CollisionShapePair_CircleCircle:
                                        {
                                            r32 HitRadius = Shape->Radius + OtherShape->Radius;
                                            v2 CenterOffset = RotateZ(Shape->Center, Entity->Yaw);
                                            v2 StartP = Entity->P.xy + CenterOffset;
                                            r32 dYaw = NewYaw-OldYaw;
                                            r32 RotationRadius = Length(Shape->Center);
                                            v2 A = OtherEntity->P.xy +
                                                RotateZ(OtherShape->Center, OtherEntity->Yaw);

                                            arc_circle_intersection_result IntersectionA =
                                                ArcCircleIntersection(Entity->P.xy, RotationRadius, StartP, dYaw, A, HitRadius);

                                            if(ProcessIntersection(IntersectionA, RotationRadius, &tMove, tYawMax, dYaw))
                                            {
                                                CollidedWith = OtherEntity;
                                                CollidingShape = Shape;
                                                OtherCollidingShape = OtherShape;
                                            }
                                        } break;

                                        case CollisionShapePair_TriangleCircle:
                                        {
                                            r32 dYaw;
                                            entity *CircleEntity;
                                            entity *TriangleEntity;
                                            collision_shape *CircleShape;
                                            collision_shape *TriangleShape;
                                            r32 RotationRadius;
                                            v2 CenterOffset;
                        
                                            if(Shape->Type == CollisionShapeType_Triangle)
                                            {
                                                TriangleEntity = Entity;
                                                TriangleShape = Shape;
                                                CircleEntity = OtherEntity;
                                                CircleShape = OtherShape;
                                                dYaw = OldYaw-NewYaw;
                                                CenterOffset = RotateZ(CircleShape->Center, CircleEntity->Yaw);
                                                RotationRadius = Length(CircleEntity->P.xy + CenterOffset - TriangleEntity->P.xy);
                                            }
                                            else
                                            {
                                                TriangleEntity = OtherEntity;
                                                TriangleShape = OtherShape;
                                                CircleEntity = Entity;
                                                CircleShape = Shape;
                                                dYaw = NewYaw-OldYaw;
                                                CenterOffset = RotateZ(CircleShape->Center, CircleEntity->Yaw);
                                                RotationRadius = Length(CircleShape->Center);
                                            }
                                            r32 Radius = CircleShape->Radius;
                                            v2 StartP = CircleEntity->P.xy + CenterOffset;
                                            v2 A = TriangleEntity->P.xy + RotateZ(TriangleShape->A, TriangleEntity->Yaw);
                                            v2 B = TriangleEntity->P.xy + RotateZ(TriangleShape->B, TriangleEntity->Yaw);
                                            v2 C = TriangleEntity->P.xy + RotateZ(TriangleShape->C, TriangleEntity->Yaw);
                        
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

                                            arc_segment_intersection_result IntersectionAB =
                                                ArcSegmentIntersection(Entity->P.xy, RotationRadius, StartP, dYaw, ABHitA, ABHitB);
                                            arc_segment_intersection_result IntersectionBC =
                                                ArcSegmentIntersection(Entity->P.xy, RotationRadius, StartP, dYaw, BCHitB, BCHitC);
                                            arc_segment_intersection_result IntersectionCA =
                                                ArcSegmentIntersection(Entity->P.xy, RotationRadius, StartP, dYaw, CAHitC, CAHitA);

                                            arc_circle_intersection_result IntersectionA =
                                                ArcCircleIntersection(Entity->P.xy, RotationRadius, StartP, dYaw, A, Radius);
                                            arc_circle_intersection_result IntersectionB =
                                                ArcCircleIntersection(Entity->P.xy, RotationRadius, StartP, dYaw, B, Radius);
                                            arc_circle_intersection_result IntersectionC =
                                                ArcCircleIntersection(Entity->P.xy, RotationRadius, StartP, dYaw, C, Radius);

                                            b32 IntersectionABUpdated = ProcessIntersection(IntersectionAB, RotationRadius, &tMove, tYawMax, dYaw);
                                            b32 IntersectionBCUpdated = ProcessIntersection(IntersectionBC, RotationRadius, &tMove, tYawMax, dYaw);
                                            b32 IntersectionCAUpdated = ProcessIntersection(IntersectionCA, RotationRadius, &tMove, tYawMax, dYaw);

                                            b32 IntersectionAUpdated = ProcessIntersection(IntersectionA, RotationRadius, &tMove, tYawMax, dYaw);
                                            b32 IntersectionBUpdated = ProcessIntersection(IntersectionB, RotationRadius, &tMove, tYawMax, dYaw);
                                            b32 IntersectionCUpdated = ProcessIntersection(IntersectionC, RotationRadius, &tMove, tYawMax, dYaw);

                                            if(IntersectionABUpdated || IntersectionBCUpdated || IntersectionCAUpdated ||
                                               IntersectionAUpdated || IntersectionBUpdated || IntersectionCUpdated)
                                            {
                                                CollidedWith = OtherEntity;
                                                CollidingShape = Shape;
                                                OtherCollidingShape = OtherShape;
                                            }
                                        } break;

                                        default:
                                        {
                                            NotImplemented;
                                        } break;
                                    }
                                }
                            }
                        }
                    }
                    // TODO(chris): Detect continuous collisions through rotation. Yikes.
                    Entity->Yaw += Entity->dYaw*Input->dtForFrame*tMove;

                    if(CollidedWith)
                    {
#if COLLISION_DEBUG_ANGULAR
                        CollidingShape->Collided = true;
                        OtherCollidingShape->Collided = true;
#endif
#if 0
                        Entity->dP = V3(0, 0, 0);
                        CollidedWith->dP = V3(0, 0, 0);
#endif
                        Entity->dYaw = -0.1f*Entity->dYaw;
//                        CollidedWith->dYaw -= Entity->dYaw*2.0f;
                        
                        // TODO(chris): Collision resolution
                    }
                }

                {
#if COLLISION_DEBUG
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
                                v3 A = V3(Shape->A, 0);
                                v3 B = V3(Shape->B, 0);
                                v3 C = V3(Shape->C, 0);
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
