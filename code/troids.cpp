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

internal rectangle2
CalculateBoundingBox(entity *Entity)
{
    rectangle2 Result;
    v2 Center = Entity->P.xy;
    v2 HalfDim = 0.5f*Entity->Dim;
    v2 Facing = V2(Cos(Entity->Yaw), Sin(Entity->Yaw));
    v2 PerpFacing = Perp(Facing);
    v2 Corners[4];
    Corners[0] = Center + HalfDim.y*Facing + HalfDim.x*PerpFacing;
    Corners[1] = Center - HalfDim.y*Facing + HalfDim.x*PerpFacing;
    Corners[2] = Center + HalfDim.y*Facing - HalfDim.x*PerpFacing;
    Corners[3] = Center - HalfDim.y*Facing - HalfDim.x*PerpFacing;
    Result.Min = Result.Max = Corners[0];
    for(u32 CornerIndex = 1;
        CornerIndex < ArrayCount(Corners);
        ++CornerIndex)
    {
        v2 Corner = Corners[CornerIndex];
        if(Corner.x < Result.Min.x)
        {
            Result.Min.x = Corner.x;
        }
        if(Corner.x > Result.Max.x)
        {
            Result.Max.x = Corner.x;
        }
        if(Corner.y < Result.Min.y)
        {
            Result.Min.y = Corner.y;
        }
        if(Corner.y > Result.Max.y)
        {
            Result.Max.y = Corner.y;
        }
    }
    return(Result);
}

inline void
DrawCollision(render_buffer *RenderBuffer, entity *Entity)
{
    v3 BoundingBoxOffset = V3(0, 0, -0.1f);
    PushRotatedRectangle(RenderBuffer, Entity->P + BoundingBoxOffset,
                         GetDim(Entity->BoundingBox), Entity->Collided ? V4(1, 0, 0, 1) : V4(1, 0, 1, 1));
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
    r32 ShipStartingYaw = 0.0f;
    v3 AsteroidStartingP = V3(50.0f, 0.0f, 0.0f);
    if(!State->IsInitialized)
    {
        State->IsInitialized = true;

        State->MetersToPixels = 3674.9418959066769192359305459154f;

        State->Ship.Dim = V2(10.0f, 10.0f);
        State->Ship.P = ShipStartingP;
        State->Ship.Yaw = ShipStartingYaw;
        State->ShipBitmap = LoadBitmap("ship_opaque.bmp");
        State->AsteroidBitmap = LoadBitmap("asteroid_opaque.bmp");
        State->BulletBitmap = LoadBitmap("bullet.bmp");

        State->CameraP = V3(State->Ship.P.xy, 100.0f);
        State->CameraRot = State->Ship.Yaw;

        State->FloatingHead.Scale = 100.0f;

        State->FloatingHead.RotationMatrix =
        {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
        };

        State->AsteroidCount = 2;
        State->Asteroids[0].P = AsteroidStartingP;
        State->Asteroids[1].P = AsteroidStartingP + V3(30.0f, 30.0f, 0.0f);
#if 0
        State->Asteroids[0].dP = 50.0f*V3(0.7f, 0.3f, 0.0f);
#endif
        State->Asteroids[0].Dim = V2(10.0f, 10.0f);
        State->Asteroids[1].Dim = V2(20.0f, 20.0f);
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
    TranState->RenderBuffer.CameraP = State->CameraP;
    TranState->RenderBuffer.CameraRot = State->CameraRot;
    temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->RenderBuffer.Arena);
    PushClear(&TranState->RenderBuffer, V4(0.1f, 0.1f, 0.1f, 1.0f));

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

    if(WentDown(ShipController->Select) && (State->AsteroidCount < ArrayCount(State->Asteroids)))
    {
        entity *NewAsteroid = State->Asteroids + State->AsteroidCount;
        entity *OldAsteroid = State->Asteroids;
        // NOTE(chris): Halving volume results in dividing radius by cuberoot(2)
        OldAsteroid->Dim *= 0.79370052598f;
        *NewAsteroid = *OldAsteroid;

        r32 Angle = Tau/12.0f;
        r32 CosRot = Cos(Angle);
        r32 SinRot = Sin(Angle);
        {
            m33 RotMat =
            {
                CosRot, SinRot, 0,
                -SinRot, CosRot, 0,
                0, 0, 1,
            };
            OldAsteroid->dP = RotMat*OldAsteroid->dP;
        }

        {
            m33 RotMat =
            {
                CosRot, -SinRot, 0,
                SinRot, CosRot, 0,
                0, 0, 1,
            };
            NewAsteroid->dP = RotMat*NewAsteroid->dP;
        }

        ++State->AsteroidCount;
    }

    if(Keyboard->Start.EndedDown || ShipController->Start.EndedDown)
    {
        State->Ship.P = ShipStartingP;
    }

    if(AsteroidController->Start.EndedDown)
    {
        State->Asteroids[0].P = AsteroidStartingP;
    }

    if(Keyboard->ActionUp.EndedDown || ShipController->ActionUp.EndedDown)
    {
        State->CameraP.z += 100.0f*Input->dtForFrame;
    }

    if(Keyboard->ActionDown.EndedDown || ShipController->ActionDown.EndedDown)
    {
        State->CameraP.z -= 100.0f*Input->dtForFrame;
    }
    if(State->FloatingHead.Scale < 0.0f)
    {
        State->FloatingHead.Scale = 0.0f;
    }

    r32 LeftStickX = Keyboard->LeftStick.x ? Keyboard->LeftStick.x : ShipController->LeftStick.x;
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
    
    r32 Halfdt = Input->dtForFrame*0.5f;
    r32 MaxDYaw = 0.2*Tau;
    r32 dYaw = Clamp(-MaxDYaw, State->Ship.dYaw + Input->dtForFrame*-LeftStickX, MaxDYaw);
    State->Ship.Yaw += (dYaw + State->Ship.dYaw)*Halfdt;
    State->Ship.dYaw = dYaw;
    v3 Facing = V3(Cos(State->Ship.Yaw), Sin(State->Ship.Yaw), 0.0f);

    r32 PixelsToMeters = 1.0f / State->MetersToPixels;
    
    v3 Acceleration = {};
    if((WentDown(ShipController->RightShoulder1) || WentDown(Keyboard->RightShoulder1)) &&
       State->BulletCount < ArrayCount(State->Bullets))
    {
        if(State->Ship.Timer <= 0.0f)
        {
            entity *Bullet = State->Bullets + State->BulletCount++;
            Bullet->Dim = V2(1.5f, 3.0f);
            r32 BulletOffset = 0.5f*(State->Ship.Dim.y + Bullet->Dim.y);
            Bullet->P = State->Ship.P + Facing*(BulletOffset);
            Bullet->dP = State->Ship.dP + Facing*100.0f;
            Bullet->Yaw = State->Ship.Yaw;
            Bullet->Timer = 2.0f;
            State->Ship.Timer = 0.1f;
            Acceleration += -Facing*100.0f;
        }
    }

    Acceleration += 50.0f*Facing*Thrust;
    v3 dP = State->Ship.dP + Acceleration*Input->dtForFrame;
    State->Ship.P += (dP + State->Ship.dP)*Halfdt;
    State->Ship.dP = dP;

    v3 AsteroidAcceleration = (0.25f*
                               V3(AsteroidController->LeftStick.x, AsteroidController->LeftStick.y, 0.0f));

    State->Asteroids[0].dP += AsteroidAcceleration*Input->dtForFrame;
    
    State->Ship.BoundingBox = CalculateBoundingBox(&State->Ship);
    State->Ship.Collided = false;
    for(u32 AsteroidIndex = 0;
        AsteroidIndex < State->AsteroidCount;
        ++AsteroidIndex)
    {
        entity *Asteroid = State->Asteroids + AsteroidIndex;
        Asteroid->Collided = false;
        Asteroid->P += Asteroid->dP*Input->dtForFrame;
        Asteroid->BoundingBox = CalculateBoundingBox(Asteroid);

        rectangle2 HitBox = AddRadius(Asteroid->BoundingBox, 0.5f*GetDim(State->Ship.BoundingBox));
        if(Inside(HitBox, State->Ship.P.xy))
        {
            Asteroid->Collided = State->Ship.Collided = true;
        }
    }
    
    if(State->Ship.Timer > 0.0f)
    {
        State->Ship.Timer -= Input->dtForFrame;
        ShipController->HighFrequencyMotor = 1.0f;
    }
    else
    {
        ShipController->HighFrequencyMotor = 0.0f;
    }

    {
        v2 YAxis = Facing.xy;
        v2 XAxis = -Perp(YAxis);
        PushBitmap(&TranState->RenderBuffer, &State->ShipBitmap, State->Ship.P,
                   XAxis, YAxis, State->Ship.Dim);
    }

    {
        v2 YAxis = V2(0, 1);
        v2 XAxis = -Perp(YAxis);
        for(u32 AsteroidIndex = 0;
            AsteroidIndex < State->AsteroidCount;
            ++AsteroidIndex)
        {
            entity *Asteroid = State->Asteroids + AsteroidIndex;
            PushBitmap(&TranState->RenderBuffer, &State->AsteroidBitmap, Asteroid->P,
                       XAxis, YAxis, Asteroid->Dim);
        }
    }
    
    entity *Bullet = State->Bullets;
    entity *End = State->Bullets + State->BulletCount;
    while(Bullet != End)
    {
        if(Bullet->Timer <= 0.0f)
        {
            *Bullet = State->Bullets[--State->BulletCount];
            --End;
        }
        else
        {
            v2 YAxis = V2(Cos(Bullet->Yaw), Sin(Bullet->Yaw));
            v2 XAxis = -Perp(YAxis);
            v3 OldP = Bullet->P;
            v3 NewP = OldP + Input->dtForFrame*Bullet->dP;
            Bullet->Collided = false;
            Bullet->BoundingBox = CalculateBoundingBox(Bullet);

            for(u32 AsteroidIndex = 0;
                AsteroidIndex < State->AsteroidCount;
                ++AsteroidIndex)
            {
                entity *Asteroid = State->Asteroids + AsteroidIndex;
                rectangle2 HitBox = AddRadius(Asteroid->BoundingBox, 0.5f*GetDim(Bullet->BoundingBox));
                if(Inside(HitBox, OldP.xy) || Inside(HitBox, NewP.xy))
                {
                        Bullet->Collided = Asteroid->Collided = true;
                }
                else
                {
                    v3 dP = NewP - OldP;
                    rectangle2 dPRect = MinMax(V2(Minimum(OldP.x, NewP.x), Minimum(OldP.y, NewP.y)),
                                               V2(Maximum(OldP.x, NewP.x), Maximum(OldP.y, NewP.y)));
                    r32 Y1 = 0.0f;
                    r32 Y2 = 0.0f;
                    r32 X1 = 0.0f;
                    r32 X2 = 0.0f;
                    if(dP.x == 0)
                    {
                        if(InsideX(HitBox, OldP.x) &&
                           (InsideY(dPRect, HitBox.Min.y) ||
                            InsideY(dPRect,  HitBox.Max.y)))
                        {
                            Bullet->Collided = Asteroid->Collided = true;
                        }
                    }
                    else if(dP.y == 0)
                    {
                        if(InsideY(HitBox, OldP.y) &&
                           (InsideX(dPRect, HitBox.Min.x) ||
                            InsideX(dPRect,  HitBox.Max.x)))
                        {
                            Bullet->Collided = Asteroid->Collided = true;
                        }
                    }
                    else
                    {
                        r32 m = dP.y / dP.x;
                        r32 mInv = 1.0f / m;
                        r32 b = OldP.y - m*OldP.x;
                        Y1 = m*HitBox.Min.x + b;
                        Y2 = m*HitBox.Max.x + b;
                        X1 = (HitBox.Min.y - b)*mInv;
                        X2 = (HitBox.Max.y - b)*mInv;
                        if((InsideY(HitBox, Y1) && InsideY(dPRect, Y1)) ||
                           (InsideY(HitBox, Y2) && InsideY(dPRect, Y2)) ||
                           (InsideX(HitBox, X1) && InsideX(dPRect, X1)) ||
                           (InsideX(HitBox, X2) && InsideX(dPRect, X2)))
                        {
                            Bullet->Collided = Asteroid->Collided = true;
                        }
                    }
                }
            }
            Bullet->P = NewP;
            PushBitmap(&TranState->RenderBuffer, &State->BulletBitmap, Bullet->P, XAxis, YAxis,
                       Bullet->Dim,
                       V4(1.0f, 1.0f, 1.0f, Unlerp(0.0f, Bullet->Timer, 2.0f)));
//            DrawLine(BackBuffer, State->P, Bullet->P, V4(0.0f, 0.0f, 1.0f, 1.0f));
            Bullet->Timer -= Input->dtForFrame;
            ++Bullet;
        }
    }

#if DEBUG_COLLISION
        DrawCollision(&TranState->RenderBuffer, &State->Ship);

        for(u32 AsteroidIndex = 0;
            AsteroidIndex < State->AsteroidCount;
            ++AsteroidIndex)
        {
            entity *Asteroid = State->Asteroids + AsteroidIndex;
            DrawCollision(&TranState->RenderBuffer, Asteroid);
        }
            
        for(u32 BulletIndex = 0;
            BulletIndex < State->BulletCount;
            ++BulletIndex)
        {
            entity *Bullet = State->Bullets + BulletIndex;
            DrawCollision(&TranState->RenderBuffer, Bullet);
        }

#if 0
        for(u32 ShapeIndex = 0;
            ShapeIndex < State->Ship.CollisionShapeCount;
            ++ShapeIndex)
        {
            collision_shape *Shape = State->Ship.CollisionBoxes + ShapeIndex;
            PushBitmap(&TranState->RenderBuffer, &State->DebugBitmap,
                       V3(State->Ship.P.xy + GetCenter(*Box), State->Ship.P.z+0.01f),
                       XAxis, YAxis, GetDim(*Box));
        }
#endif
#endif

    State->CameraP.xy += 0.2f*(State->Ship.P.xy - State->CameraP.xy);
    State->CameraRot += 0.05f*((State->Ship.Yaw - 0.25f*Tau) - State->CameraRot);

    v4 White = V4(1.0f, 1.0f, 1.0f, 1.0f);

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

    State->FloatingHead.RotationMatrix =
        ZRotationMatrix*YRotationMatrix*XRotationMatrix*State->FloatingHead.RotationMatrix;

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
                PushBitmap(&TranState->RenderBuffer, Glyph, P, XAxis, YAxis, Scale);
                P.x += XAxis.x*Scale;
            }
        }
    }
#endif

#if 0
    for(u32 FaceIndex = 1;
        FaceIndex < State->HeadMesh.FaceCount;
        ++FaceIndex)
    {
        v3 Translation = V3(0.5f*BackBuffer->Width, 0.5f*BackBuffer->Height, 0.0f);
        v3 Scale = State->Scale*V3(1.0f, 1.0f, 1.0f);
        obj_face *Face = State->HeadMesh.Faces + FaceIndex;
        v3 VertexA = (Hadamard(State->RotationMatrix*(State->HeadMesh.Vertices + Face->VertexIndexA)->xyz, Scale) +
                      Translation);
        v3 VertexB = (Hadamard(State->RotationMatrix*(State->HeadMesh.Vertices + Face->VertexIndexB)->xyz, Scale) +
                      Translation);
        v3 VertexC = (Hadamard(State->RotationMatrix*(State->HeadMesh.Vertices + Face->VertexIndexC)->xyz, Scale) +
                      Translation);

        PushLine(&TranState->RenderBuffer, VertexA, VertexB, White);
        PushLine(&TranState->RenderBuffer, VertexB, VertexC, White);
        PushLine(&TranState->RenderBuffer, VertexC, VertexA, White);
    }
#endif


    {
        TIMED_BLOCK("RenderGame");
        RenderBufferToBackBuffer(&TranState->RenderBuffer, BackBuffer);
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
