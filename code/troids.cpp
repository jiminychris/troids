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

platform_read_file *PlatformReadFile;

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

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
#if TROIDS_INTERNAL
    GlobalDebugState = (debug_state *)GameMemory->DebugMemory;
#endif
    TIMED_FUNCTION();
    game_state *State = (game_state *)GameMemory->PermanentMemory;
    if(!State->IsInitialized)
    {
        PlatformReadFile = GameMemory->PlatformReadFile;
        
        State->IsInitialized = true;

        State->P = V3(BackBuffer->Width / 2.0f, BackBuffer->Height / 2.0f, 0.0f);
        State->Ship = LoadBitmap("ship_opaque.bmp");
        State->Asteroid = LoadBitmap("asteroid_opaque.bmp");
        State->Bullet = LoadBitmap("bullet.bmp");

        State->Scale = 100.0f;

        State->RotationMatrix =
        {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
        };

        State->AsteroidCount = 1;
        State->Asteroids[0].P = V3(BackBuffer->Width / 4.0f, BackBuffer->Height / 4.0f, 0.0f);
        State->Asteroids[0].dP = 50.0f*V3(0.7f, 0.3f, 0.0f);
        State->Asteroids[0].Scale = 0.15f;
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
        
        State->HeadMesh = LoadObj("head.obj", &TranState->TranArena);

        TranState->IsInitialized = true;
    }
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
        asteroid *NewAsteroid = State->Asteroids + State->AsteroidCount;
        asteroid *OldAsteroid = State->Asteroids;
        // NOTE(chris): Halving volume results in dividing radius by cuberoot(2)
        OldAsteroid->Scale *= 0.79370052598f;
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
        State->P = V3(BackBuffer->Width / 2.0f, BackBuffer->Height / 2.0f, 0.0f);
    }

    if(AsteroidController->Start.EndedDown)
    {
        State->Asteroids[0].P = V3(BackBuffer->Width / 2.0f, BackBuffer->Height / 2.0f, 0.0f);
    }

    if(Keyboard->ActionUp.EndedDown || ShipController->ActionUp.EndedDown)
    {
        State->Scale += Input->dtForFrame*100.0f;
    }

    if(Keyboard->ActionDown.EndedDown || ShipController->ActionDown.EndedDown)
    {
        State->Scale -= Input->dtForFrame*100.0f;
    }
    if(State->Scale < 0.0f)
    {
        State->Scale = 0.0f;
    }

    r32 ShipScale = 0.4f;
    r32 BulletScale = 0.5f;

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
    r32 dYaw = State->dYaw + Input->dtForFrame*-LeftStickX;
    State->Yaw += (dYaw + State->dYaw)*Halfdt;
    State->dYaw = dYaw;
    v3 Facing = V3(Cos(State->Yaw), Sin(State->Yaw), 0.0f);

    v3 Acceleration = {};
    if((WentDown(ShipController->RightShoulder1) || WentDown(Keyboard->RightShoulder1)) &&
       State->LiveBulletCount < ArrayCount(State->LiveBullets))
    {
        if(State->Cooldown <= 0.0f)
        {
            live_bullet *LiveBullet = State->LiveBullets + State->LiveBulletCount++;
            r32 BulletOffset = 0.5f*(ShipScale*State->Ship.Height + BulletScale*State->Bullet.Height);
            LiveBullet->P = State->P + Facing*(BulletOffset);
            LiveBullet->Direction = State->Yaw;
            LiveBullet->Timer = 2.0f;
            State->Cooldown = 0.1f;
            Acceleration += -Facing*200.0f;
        }
    }

    r32 MetersPerPixel = 0.00027211314f;
    r32 PixelsPerMeter = 1.0f/MetersPerPixel;

    Acceleration += Facing*0.075f*PixelsPerMeter*Thrust;
    v3 dP = State->dP + Acceleration*Input->dtForFrame;
    State->P += (dP + State->dP)*Halfdt;
    State->dP = dP;

    v3 AsteroidAcceleration = (0.25f*PixelsPerMeter*
                               V3(AsteroidController->LeftStick.x, AsteroidController->LeftStick.y, 0.0f));

    State->Asteroids[0].dP += AsteroidAcceleration*Input->dtForFrame;
    
    for(u32 AsteroidIndex = 0;
        AsteroidIndex < State->AsteroidCount;
        ++AsteroidIndex)
    {
        asteroid *Asteroid = State->Asteroids + AsteroidIndex;
        Asteroid->P += Asteroid->dP*Input->dtForFrame;
    }
    
    if(State->Cooldown > 0.0f)
    {
        State->Cooldown -= Input->dtForFrame;
        ShipController->HighFrequencyMotor = 1.0f;
    }
    else
    {
        ShipController->HighFrequencyMotor = 0.0f;
    }

    {
        v2 YAxis = Facing.xy;
        v2 XAxis = -Perp(YAxis);
        PushBitmap(&TranState->RenderBuffer, &State->Ship, State->P, XAxis, YAxis, ShipScale);
    }

    {
        v2 YAxis = V2(0, 1);
        v2 XAxis = -Perp(YAxis);
        for(u32 AsteroidIndex = 0;
            AsteroidIndex < State->AsteroidCount;
            ++AsteroidIndex)
        {
            asteroid *Asteroid = State->Asteroids + AsteroidIndex;
            PushBitmap(&TranState->RenderBuffer, &State->Asteroid, Asteroid->P, XAxis, YAxis, Asteroid->Scale);
        }
    }
    
    live_bullet *LiveBullet = State->LiveBullets;
    live_bullet *End = State->LiveBullets + State->LiveBulletCount;
    while(LiveBullet != End)
    {
        if(LiveBullet->Timer <= 0.0f)
        {
            *LiveBullet = State->LiveBullets[--State->LiveBulletCount];
            --End;
        }
        else
        {
            v2 YAxis = V2(Cos(LiveBullet->Direction), Sin(LiveBullet->Direction));
            v2 XAxis = -Perp(YAxis);
            LiveBullet->P += V3(YAxis, 0)*Input->dtForFrame*PixelsPerMeter*0.1f;
            PushBitmap(&TranState->RenderBuffer, &State->Bullet, LiveBullet->P, XAxis, YAxis, BulletScale,
                       V4(1.0f, 1.0f, 1.0f, Unlerp(0.0f, LiveBullet->Timer, 2.0f)));
//            DrawLine(BackBuffer, State->P, LiveBullet->P, V4(0.0f, 0.0f, 1.0f, 1.0f));
            LiveBullet->Timer -= Input->dtForFrame;
            ++LiveBullet;
        }
    }

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

    State->RotationMatrix = ZRotationMatrix*YRotationMatrix*XRotationMatrix*State->RotationMatrix;

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
