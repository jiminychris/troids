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

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    game_state *State = (game_state *)GameMemory->PermanentMemory;
    if(!State->IsInitialized)
    {
        PlatformReadFile = GameMemory->PlatformReadFile;
        
        State->IsInitialized = true;

        State->P = V2(BackBuffer->Width / 2.0f, BackBuffer->Height / 2.0f);
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
        State->Asteroids[0].P = V2(BackBuffer->Width / 4.0f, BackBuffer->Height / 4.0f);
        State->Asteroids[0].dP = 50.0f*V2(0.7f, 0.3f);
        State->Asteroids[0].Scale = 0.15f;
    }

    transient_state *TranState = (transient_state *)GameMemory->TemporaryMemory;
    if(!TranState->IsInitialized)
    {
        InitializeArena(&TranState->TranArena,
                        GameMemory->TemporaryMemorySize - sizeof(transient_state),
                        (u8 *)GameMemory->TemporaryMemory + sizeof(transient_state));

        TranState->RenderBuffer.Arena = SubArena(&TranState->TranArena, Megabytes(1));
        
        TranState->IsInitialized = true;
        
        State->HeadMesh = LoadObj("head.obj", &TranState->TranArena);
    }
    temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->RenderBuffer.Arena);
    PushClear(&TranState->RenderBuffer, V4(0.1f, 0.1f, 0.1f, 1.0f));


#if 1
    r32 FontScale = 0.3f;
    u32 TextLength;
    char Text[256];
    TextLength = _snprintf_s(Text, sizeof(Text), "Keyboard");
        
    for(u32 ControllerIndex = 0;
        ControllerIndex < ArrayCount(Input->Controllers);
        ++ControllerIndex)
    {
        v2 At = V2(BackBuffer->Width*((r32)ControllerIndex/(r32)ArrayCount(Input->Controllers)),
                   BackBuffer->Height - GameMemory->DebugFont.Height*FontScale);
        game_controller *Controller = Input->Controllers + ControllerIndex;

        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);

        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %f", "LeftStickX", Controller->LeftStickX);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %f", "LeftStickY", Controller->LeftStickY);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %f", "RightStickX", Controller->RightStickX);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %f", "RightStickY", Controller->RightStickY);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %f", "LeftTrigger", Controller->LeftTrigger);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %f", "RightTrigger", Controller->RightTrigger);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %d", "ActionUp", Controller->ActionUp);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %d", "ActionLeft", Controller->ActionLeft);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %d", "ActionDown", Controller->ActionDown);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %d", "ActionRight", Controller->ActionRight);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %d", "LeftShoulder1", Controller->LeftShoulder1);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %d", "RightShoulder1", Controller->RightShoulder1);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %d", "LeftShoulder2", Controller->LeftShoulder2);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %d", "RightShoulder2", Controller->RightShoulder2);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %d", "Select", Controller->Select);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %d", "Start", Controller->Start);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %d", "LeftClick", Controller->LeftClick);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %d", "RightClick", Controller->RightClick);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %d", "Power", Controller->Power);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);
        TextLength = _snprintf_s(Text, sizeof(Text), "%-15s %d", "CenterClick", Controller->CenterClick);
        PushText(&TranState->RenderBuffer, &GameMemory->DebugFont,
                 TextLength, Text, &At, FontScale);

        TextLength = _snprintf_s(Text, sizeof(Text), "Game Pad %d", ControllerIndex);
    }
#endif

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
            m22 RotMat =
            {
                CosRot, SinRot,
                -SinRot, CosRot,
            };
            OldAsteroid->dP = RotMat*OldAsteroid->dP;
        }

        {
            m22 RotMat =
            {
                CosRot, -SinRot,
                SinRot, CosRot,
            };
            NewAsteroid->dP = RotMat*NewAsteroid->dP;
        }

        ++State->AsteroidCount;
    }

    if(Keyboard->Start.EndedDown || ShipController->Start.EndedDown)
    {
        State->P = V2(BackBuffer->Width / 2.0f, BackBuffer->Height / 2.0f);
    }

    if(AsteroidController->Start.EndedDown)
    {
        State->Asteroids[0].P = V2(BackBuffer->Width / 2.0f, BackBuffer->Height / 2.0f);
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

    r32 LeftStickX = Keyboard->LeftStickX ? Keyboard->LeftStickX : ShipController->LeftStickX;
    r32 Throttle = Keyboard->LeftStickY ? Keyboard->LeftStickY : ShipController->LeftStickY;

    r32 YRotation = Input->dtForFrame*(Keyboard->RightStickX ?
                                       Keyboard->RightStickX :
                                       ShipController->RightStickX);
    r32 XRotation = Input->dtForFrame*(Keyboard->RightStickY ?
                                       Keyboard->RightStickY :
                                       ShipController->RightStickY);
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
    v2 Facing = V2(Cos(State->Yaw), Sin(State->Yaw));

    v2 Acceleration = {};
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

    Acceleration += Facing*0.075f*PixelsPerMeter*Clamp01(Throttle);
    v2 dP = State->dP + Acceleration*Input->dtForFrame;
    State->P += (dP + State->dP)*Halfdt;
    State->dP = dP;

    v2 AsteroidAcceleration = (0.25f*PixelsPerMeter*
                               V2(AsteroidController->LeftStickX, AsteroidController->LeftStickY));

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
    }

    {
        v2 YAxis = Facing;
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
            LiveBullet->P += YAxis*Input->dtForFrame*PixelsPerMeter*0.1f;
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

#if 1
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

        PushLine(&TranState->RenderBuffer, VertexA.xy, VertexB.xy, White);
        PushLine(&TranState->RenderBuffer, VertexB.xy, VertexC.xy, White);
        PushLine(&TranState->RenderBuffer, VertexC.xy, VertexA.xy, White);
    }
#endif

    RenderBufferToBackBuffer(&TranState->RenderBuffer, BackBuffer);
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
