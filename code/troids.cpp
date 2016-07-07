/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include "troids_platform.h"
#include "troids_intrinsics.h"
#include "troids_math.h"
#include "troids.h"

platform_read_file *PlatformReadFile;

internal loaded_bitmap
LoadBitmap(char *FileName)
{
    loaded_bitmap Result = {};
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

#pragma optimize("gts", on)
internal void
RenderBitmap(game_backbuffer *BackBuffer, loaded_bitmap Bitmap, v2 Origin, v2 XAxis, v2 YAxis,
             r32 Scale, v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    Color.rgb *= Color.a;
    XAxis *= Scale;
    YAxis *= Scale;
    Origin -= Hadamard(Bitmap.Align, XAxis + YAxis);
    s32 XMin = Clamp(0, Floor(Minimum(Minimum(Origin.x, Origin.x + XAxis.x),
                                      Minimum(Origin.x + YAxis.x, Origin.x + XAxis.x + YAxis.x))),
                     BackBuffer->Width);
    s32 YMin = Clamp(0, Floor(Minimum(Minimum(Origin.y, Origin.y + XAxis.y),
                                      Minimum(Origin.y + YAxis.y, Origin.y + XAxis.y + YAxis.y))),
                     BackBuffer->Height);

    s32 XMax = Clamp(0, Ceiling(Maximum(Maximum(Origin.x, Origin.x + XAxis.x),
                                        Maximum(Origin.x + YAxis.x, Origin.x + XAxis.x + YAxis.x))),
                     BackBuffer->Width);
    s32 YMax = Clamp(0, Ceiling(Maximum(Maximum(Origin.y, Origin.y + XAxis.y),
                                        Maximum(Origin.y + YAxis.y, Origin.y + XAxis.y + YAxis.y))),
                     BackBuffer->Height);

    r32 Inv255 = 1.0f / 255.0f;
    r32 InvXAxisLengthSq = 1.0f / LengthSq(XAxis);
    r32 InvYAxisLengthSq = 1.0f / LengthSq(YAxis);
    u8 *PixelRow = (u8 *)BackBuffer->Memory + (BackBuffer->Pitch*YMin);
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Pixel = (u32 *)PixelRow + XMin;
        for(s32 X = XMin;
            X < XMax;
            X += 1)
        {
            // NOTE(chris): Test the next four pixels and mask the ones we're using
            v2 TestPoint = V2(X, Y) - Origin;

            r32 U = Inner(TestPoint, XAxis)*InvXAxisLengthSq;
            r32 V = Inner(TestPoint, YAxis)*InvYAxisLengthSq;

            if(U >= 0.0f &&
               U <= 1.0f &&
               V >= 0.0f &&
               V <= 1.0f)
            {
                r32 tX = U*(Bitmap.Width - 2) + 0.5f;
                r32 tY = V*(Bitmap.Height - 2) + 0.5f;

                u32 *TexelAPtr = (u32 *)Bitmap.Memory + (u32)tY*Bitmap.Width + (u32)tX;
                u32 *TexelBPtr = TexelAPtr + 1;
                u32 *TexelCPtr = TexelAPtr + Bitmap.Width;
                u32 *TexelDPtr = TexelCPtr + 1;
                v4 TexelA = V4((*TexelAPtr >> 16) & 0xFF,
                               (*TexelAPtr >> 8) & 0xFF,
                               (*TexelAPtr >> 0) & 0xFF,
                               (*TexelAPtr >> 24) & 0xFF);
                v4 TexelB = V4((*TexelBPtr >> 16) & 0xFF,
                               (*TexelBPtr >> 8) & 0xFF,
                               (*TexelBPtr >> 0) & 0xFF,
                               (*TexelBPtr >> 24) & 0xFF);
                v4 TexelC = V4((*TexelCPtr >> 16) & 0xFF,
                               (*TexelCPtr >> 8) & 0xFF,
                               (*TexelCPtr >> 0) & 0xFF,
                               (*TexelCPtr >> 24) & 0xFF);
                v4 TexelD = V4((*TexelDPtr >> 16) & 0xFF,
                               (*TexelDPtr >> 8) & 0xFF,
                               (*TexelDPtr >> 0) & 0xFF,
                               (*TexelDPtr >> 24) & 0xFF);

                r32 tU = 1.0f - (tX - (u32)tX);
                r32 tV = 1.0f - (tY - (u32)tY);
                v4 Texel = Lerp(Lerp(TexelA, tU, TexelB), tV, Lerp(TexelC, tU, TexelD));

                r32 DA = 1.0f - (Inv255*Texel.a*Color.a);
                u32 DR = (*Pixel >> 16) & 0xFF;
                u32 DG = (*Pixel >> 8) & 0xFF;
                u32 DB = (*Pixel >> 0) & 0xFF;
                
                *Pixel = (((RoundU32(Color.r*Texel.r + DA*DR)) << 16) |
                          ((RoundU32(Color.g*Texel.g + DA*DG)) << 8) |
                          ((RoundU32(Color.b*Texel.b + DA*DB)) << 0));
            }
            Pixel += 1;
        }
        PixelRow += BackBuffer->Pitch;
    }
}

internal void
DrawRectangle(game_backbuffer *BackBuffer, rectangle2 Rect, v4 Color)
{
    s32 XMin = Clamp(0, RoundS32(Rect.Min.x), BackBuffer->Width);
    s32 YMin = Clamp(0, RoundS32(Rect.Min.y), BackBuffer->Height);
    s32 XMax = Clamp(0, RoundS32(Rect.Max.x), BackBuffer->Width);
    s32 YMax = Clamp(0, RoundS32(Rect.Max.y), BackBuffer->Height);
    
    u32 A = (u32)(Color.a*255.0f + 0.5f);
    u32 R = (u32)(Color.r*255.0f + 0.5f);
    u32 G = (u32)(Color.g*255.0f + 0.5f);
    u32 B = (u32)(Color.b*255.0f + 0.5f);

    u32 DestColor = ((A << 24) | // A
                     (R << 16) | // R
                     (G << 8)  | // G
                     (B << 0));  // B

    u8 *PixelRow = (u8 *)BackBuffer->Memory;
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Dest = (u32 *)PixelRow;
        for(s32 X = XMin;
            X < XMax;
            ++X)
        {
            *Dest++ = DestColor;
        }
        PixelRow += BackBuffer->Pitch;
    }
}
#pragma optimize("", on)

inline void
Clear(game_backbuffer *BackBuffer, v4 Color)
{
    rectangle2 Rect;
    Rect.Min = V2(0, 0);
    Rect.Max = V2(BackBuffer->Width, BackBuffer->Height);
    DrawRectangle(BackBuffer, Rect, Color);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    game_state *State = (game_state *)GameMemory->PermanentMemory;
    if(!State->IsInitialized)
    {
        PlatformReadFile = GameMemory->PlatformReadFile;
        
        State->IsInitialized = true;

        State->P = V2(BackBuffer->Width / 2, BackBuffer->Height / 2);
        State->Ship = LoadBitmap("ship_opaque.bmp");
        State->Bullet = LoadBitmap("bullet.bmp");
    }

    transient_state *TranState = (transient_state *)GameMemory->PermanentMemory;
    if(!TranState->IsInitialized)
    {
        TranState->IsInitialized = true;
    }

    game_controller *Controller = Input->Controllers + 0;
    game_controller *Keyboard = &Input->Keyboard;

    if(Keyboard->Start.EndedDown || Controller->Start.EndedDown)
    {
        State->P = V2(BackBuffer->Width / 2, BackBuffer->Height / 2);
    }

    r32 ShipScale = 100.0f;
    r32 BulletScale = 30.0f;
    r32 LeftStickX = Controller->LeftStickX;
    if(Keyboard->LeftStickX)
    {
        LeftStickX = Keyboard->LeftStickX;
    }
    r32 Throttle = Controller->RightTrigger;
    if(Keyboard->LeftStickY)
    {
        Throttle = Keyboard->LeftStickY;
    }
    r32 Halfdt = Input->dtForFrame*0.5f;
    r32 dYaw = State->dYaw + Input->dtForFrame*-LeftStickX;
    State->Yaw += (dYaw + State->dYaw)*Halfdt;
    State->dYaw = dYaw;
    v2 Facing = V2(Cos(State->Yaw), Sin(State->Yaw));

    v2 Acceleration = {};
    if((WentDown(Controller->RightShoulder1) || WentDown(Keyboard->RightShoulder1)) &&
       State->LiveBulletCount < ArrayCount(State->LiveBullets))
    {
        if(State->Cooldown <= 0.0f)
        {
            live_bullet *LiveBullet = State->LiveBullets + State->LiveBulletCount++;
            LiveBullet->P = State->P + 0.5f*Facing*(ShipScale + BulletScale);
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
    if(State->Cooldown > 0.0f)
    {
        State->Cooldown -= Input->dtForFrame;
    }

    Clear(BackBuffer, V4(0.1f, 0.1f, 0.1f, 1.0f));

    {
        v2 YAxis = Facing;
        v2 XAxis = -Perp(YAxis);
        RenderBitmap(BackBuffer, State->Ship, State->P, XAxis, YAxis, ShipScale);
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
            v2 XAxis = -0.5f*Perp(YAxis);
            LiveBullet->P += YAxis*Input->dtForFrame*PixelsPerMeter*0.1f;
            RenderBitmap(BackBuffer, State->Bullet, LiveBullet->P, XAxis, YAxis, BulletScale,
                         V4(1.0f, 1.0f, 1.0f, Unlerp(0.0f, LiveBullet->Timer, 2.0f)));
            LiveBullet->Timer -= Input->dtForFrame;
            ++LiveBullet;
        }
    }
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
