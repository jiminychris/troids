/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#define DEBUG_COLLISION 1

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
    r32 Result = 0.5f*Length(Entity->Dim);
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

inline entity *
CreateShip(game_state *State, v3 P, r32 Yaw)
{
    entity *Result = CreateEntity(State);
    Result->Type = EntityType_Ship;
    Result->Dim = V2(10.0f, 10.0f);
    Result->P = P;
    Result->Yaw = Yaw;
    Result->Collides = true;
    return(Result);
}

inline entity *
CreateFloatingHead(game_state *State)
{
    entity *Result = CreateEntity(State);
    Result->Type = EntityType_FloatingHead;
    Result->P = V3(0.0f, 0.0f, 0.0f);
    Result->Dim = V2(100.0f, 100.0f);
    Result->RotationMatrix =
    {
        1, 0, 0,
        0, 1, 0,
        0, 0, 1,
    };
    Result->Collides = false;
    return(Result);
}

inline entity *
CreateAsteroid(game_state *State, v3 P, r32 Radius)
{
    entity *Result = CreateEntity(State);
    Result->Type = EntityType_Asteroid;
    Result->P = P;
    Result->Dim = V2(Radius, Radius);
    Result->CollisionShapeCount = 1;
    Result->CollisionShapes[0].Type = CollisionShapeType_Circle;
    Result->CollisionShapes[0].Radius = Radius;
    Result->Collides = true;
    return(Result);
}

inline entity *
CreateBullet(game_state *State, v3 P, v3 dP, r32 Yaw)
{
    entity *Result = CreateEntity(State);
    Result->Type = EntityType_Bullet;
    Result->P = P;
    Result->dP = dP;
    Result->Yaw = Yaw;
    Result->Dim = V2(1.5f, 3.0f);
    Result->Timer = 2.0f;
    Result->Collides = true;
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
    v3 ShipStartingP = V3(0.0f, 0.0f, 0.0f);
    r32 ShipStartingYaw = 0.25f*Tau;
    v3 AsteroidStartingP = V3(50.0f, 0.0f, 0.0f);

    if(!State->IsInitialized)
    {
        State->IsInitialized = true;

        State->MetersToPixels = 3674.9418959066769192359305459154f;

        State->ShipBitmap = LoadBitmap("ship_opaque.bmp");
        State->AsteroidBitmap = LoadBitmap("asteroid_opaque.bmp");
        State->BulletBitmap = LoadBitmap("bullet.bmp");

        entity *Ship = CreateShip(State, ShipStartingP, ShipStartingYaw);
        entity *FloatingHead = CreateFloatingHead(State);

        entity *SmallAsteroid = CreateAsteroid(State, AsteroidStartingP, 10.0f);
        entity *SmallAsteroid2 = CreateAsteroid(State, V3(Perp(AsteroidStartingP.xy), 0), 10.0f);
        entity *LargeAsteroid = CreateAsteroid(State, AsteroidStartingP + V3(30.0f, 30.0f, 0.0f), 20.0f);

        State->CameraP = V3(Ship->P.xy, 100.0f);
        State->CameraRot = Ship->Yaw;
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
                    Entity->P = ShipStartingP;
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

                Acceleration += 100.0f*Facing*Thrust;
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
                    b32 IsFirstAsteroid = false;
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
                        Entity->P = AsteroidStartingP;
                    }
                }

                v3 Acceleration = (0.25f*V3(AsteroidController->LeftStick.x,
                                            AsteroidController->LeftStick.y,
                                            0.0f));
                Entity->dP += Acceleration*Input->dtForFrame;
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
            Entity->Collided = false;
        }
        EntityIndex = NextIndex;
    }

    // NOTE(chris): Collision pass
    for(u32 EntityIndex = 0;
        EntityIndex < State->EntityCount;
        ++EntityIndex)
    {
        entity *Entity = State->Entities + EntityIndex;
        if(Entity->Collides)
        {
            v3 OldP = Entity->P;
            v3 NewP = OldP + Input->dtForFrame*Entity->dP;
            v3 dP = NewP - OldP;
            for(u32 OtherEntityIndex = EntityIndex + 1;
                OtherEntityIndex < State->EntityCount;
                ++OtherEntityIndex)
            {
                entity *OtherEntity = State->Entities + OtherEntityIndex;
                if(!OtherEntity->Collides) continue;

                v3 RelativeOldP = OldP - OtherEntity->P;
                v3 RelativeNewP = NewP - OtherEntity->P;
                r32 RelativeOldPLengthSq = LengthSq(RelativeOldP.xy);
                r32 RelativeNewPLengthSq = LengthSq(RelativeNewP.xy);
                r32 HitRadius = OtherEntity->BoundingRadius + Entity->BoundingRadius;
                r32 HitRadiusSq = Square(HitRadius);
                if(RelativeOldPLengthSq <= HitRadiusSq || RelativeNewPLengthSq <= HitRadiusSq)
                {
                    Entity->Collided = OtherEntity->Collided = true;
                }
                else
                {
                    rectangle2 dPRect = MinMax(V2(Minimum(RelativeOldP.x, RelativeNewP.x), Minimum(RelativeOldP.y, RelativeNewP.y)),
                                               V2(Maximum(RelativeOldP.x, RelativeNewP.x), Maximum(RelativeOldP.y, RelativeNewP.y)));
                    if(dP.x == 0)
                    {
                        if(dPRect.Min.y < -HitRadius && dPRect.Max.y > HitRadius)
                        {
                            r32 YSq = HitRadiusSq - RelativeOldP.x*RelativeOldP.x;
                            if(YSq >= 0)
                            {
                                Entity->Collided = OtherEntity->Collided = true;
                            }
                        }
                    }
                    else if(dP.y == 0)
                    {
                        if(dPRect.Min.x < -HitRadius && dPRect.Max.x > HitRadius)
                        {
                            r32 XSq = HitRadiusSq - RelativeOldP.y*RelativeOldP.y;
                            if(XSq >= 0)
                            {
                                Entity->Collided = OtherEntity->Collided = true;
                            }
                        }
                    }
                    else
                    {
                        r32 dPLengthSq = LengthSq(dP.xy);
                        r32 D = RelativeOldP.x*RelativeNewP.y - RelativeNewP.x*RelativeOldP.y;
                        r32 Discriminant = HitRadiusSq*dPLengthSq - D*D;
                        if(Discriminant >= 0)
                        {
                            r32 Ddx = -D*dP.x;
                            r32 Root = AbsoluteValue(dP.y)*SquareRoot(Discriminant);
                            r32 InvdPLengthSq = 1.0f / dPLengthSq;
                            r32 Y1 = (Ddx + Root)*InvdPLengthSq;
                            r32 Y2 = (Ddx - Root)*InvdPLengthSq;

                            if(InsideY(dPRect, Y1) || InsideY(dPRect, Y2))
                            {
                                Entity->Collided = OtherEntity->Collided = true;
                            }
                        }
                    }
#if 0
                    r32 Y1 = 0.0f;
                    r32 Y2 = 0.0f;
                    r32 X1 = 0.0f;
                    r32 X2 = 0.0f;
                    if(dP.x == 0)
                    {
                        if(InsideX(HitBox, RelativeOldP.x) &&
                           (InsideY(dPRect, HitBox.Min.y) ||
                            InsideY(dPRect,  HitBox.Max.y)))
                        {
                            Entity->Collided = OtherEntity->Collided = true;
                        }
                    }
                    else if(dP.y == 0)
                    {
                        if(InsideY(HitBox, RelativeOldP.y) &&
                           (InsideX(dPRect, HitBox.Min.x) ||
                            InsideX(dPRect,  HitBox.Max.x)))
                        {
                            Entity->Collided = OtherEntity->Collided = true;
                        }
                    }
                    else
                    {
                        r32 m = dP.y / dP.x;
                        r32 mInv = 1.0f / m;
                        r32 b = RelativeOldP.y - m*RelativeOldP.x;
                        Y1 = m*HitBox.Min.x + b;
                        Y2 = m*HitBox.Max.x + b;
                        X1 = (HitBox.Min.y - b)*mInv;
                        X2 = (HitBox.Max.y - b)*mInv;
                        if((InsideY(HitBox, Y1) && InsideY(dPRect, Y1)) ||
                           (InsideY(HitBox, Y2) && InsideY(dPRect, Y2)) ||
                           (InsideX(HitBox, X1) && InsideX(dPRect, X1)) ||
                           (InsideX(HitBox, X2) && InsideX(dPRect, X2)))
                        {
                            Entity->Collided = OtherEntity->Collided = true;
                        }
                    }
#endif
                }
            }
            Entity->P += Entity->dP*Input->dtForFrame;
            // TODO(chris): Detect continuous collisions through rotation. Yikes.
            Entity->Yaw += Entity->dYaw*Input->dtForFrame;
#if DEBUG_COLLISION
            v3 BoundingCircleOffset = V3(0, 0, -0.0001f);
            v3 ExtrusionOffset = V3(0, 0, -0.0002f);
            v4 Color = Entity->Collided ? V4(1, 0, 0, 1) : V4(1, 0, 1, 1);
            PushCircle(RenderBuffer, OldP + BoundingCircleOffset, Entity->BoundingRadius, Color);
            PushCircle(RenderBuffer, NewP + BoundingCircleOffset, Entity->BoundingRadius, Color);

            r32 dPLength = Length(dP.xy);
            r32 InvdPLength = 1.0f / dPLength;
            v2 XAxis = InvdPLength*-Perp(dP.xy);
            v2 YAxis = Perp(XAxis);

            v2 Dim = V2(2.0f*Entity->BoundingRadius, dPLength);
            PushRotatedRectangle(RenderBuffer, 0.5f*(OldP + NewP) + ExtrusionOffset,
                                 XAxis, YAxis, Dim, (Entity->Collided ?
                                                     V4(0.8f, 0, 0, 1) :
                                                     V4(1, 0, 0.8f, 1)));
#if 0
            for(u32 ShapeIndex = 0;
                ShapeIndex < State->Ship.CollisionShapeCount;
                ++ShapeIndex)
            {
                collision_shape *Shape = State->Ship.CollisionBoxes + ShapeIndex;
                PushBitmap(RenderBuffer, &State->DebugBitmap,
                           V3(State->Ship.P.xy + GetCenter(*Box), State->Ship.P.z+0.01f),
                           XAxis, YAxis, GetDim(*Box));
            }
#endif
#endif
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
                State->CameraP.xy += 0.2f*(Entity->P.xy - State->CameraP.xy);
                State->CameraRot += 0.05f*((Entity->Yaw - 0.25f*Tau) - State->CameraRot);
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
