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

internal void
RenderBitmap(game_backbuffer *BackBuffer, loaded_bitmap Bitmap, v2 Origin, v2 XAxis, v2 YAxis,
             r32 Scale, v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
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
            ++X)
        {
            v2 TestPoint = V2(X, Y) - Origin;
            r32 EdgeTestA = Inner(TestPoint, Perp(XAxis));
            r32 EdgeTestB = Inner(TestPoint - XAxis, Perp(YAxis));
            r32 EdgeTestC = Inner(TestPoint - XAxis - YAxis, Perp(-XAxis));
            r32 EdgeTestD = Inner(TestPoint - YAxis, Perp(-YAxis));

#if 1
            if(EdgeTestA > 0.0f &&
               EdgeTestB > 0.0f &&
               EdgeTestC > 0.0f &&
               EdgeTestD > 0.0f)
            {

                r32 U = Clamp01(Inner(TestPoint, XAxis)*InvXAxisLengthSq);
                r32 V = Clamp01(Inner(TestPoint, YAxis)*InvYAxisLengthSq);

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

                r32 DA = 1.0f - (Inv255*Texel.a);
                u32 DR = (*Pixel >> 16) & 0xFF;
                u32 DG = (*Pixel >> 8) & 0xFF;
                u32 DB = (*Pixel >> 0) & 0xFF;
                
                *Pixel = (((RoundU32(Color.r*Texel.r + DA*DR)) << 16) |
                          ((RoundU32(Color.g*Texel.g + DA*DG)) << 8) |
                          ((RoundU32(Color.b*Texel.b + DA*DB)) << 0));
            }
#else
            *Pixel = 0xFF0000FF;

#endif
            *Pixel++;
        }
        PixelRow += BackBuffer->Pitch;
    }

#if 0
    rectangle2i ClipRect;
    v2 BitmapP = V2(P.x - Bitmap.Width*Bitmap.Align.x,
                    P.y - Bitmap.Height*Bitmap.Align.y);
    ClipRect.Min.x = Clamp(0, Floor(BitmapP.x), BackBuffer->Width);
    ClipRect.Min.y = Clamp(0, Floor(BitmapP.y), BackBuffer->Height);
    ClipRect.Max.x = Clamp(0, Ceiling(BitmapP.x + Bitmap.Width - 2.0f), BackBuffer->Width);
    ClipRect.Max.y = Clamp(0, Ceiling(BitmapP.y + Bitmap.Height - 2.0f), BackBuffer->Height);
    r32 U = BitmapP.x - Floor(BitmapP.x);
    r32 V = BitmapP.y - Floor(BitmapP.y);
    r32 InvU = 1.0f - U;
    r32 InvV = 1.0f - V;
    r32 Inv255 = 1.0f / 255.0f;
    u8 *DestRow = (u8 *)BackBuffer->Memory + (BackBuffer->Pitch*ClipRect.Min.y);
    u8 *SourceRow = (u8 *)Bitmap.Memory + (Bitmap.Pitch*(ClipRect.Min.y - Floor(BitmapP.y)));
    for(s32 Y = ClipRect.Min.y;
        Y < ClipRect.Max.y;
        ++Y)
    {
        u32 *Dest = ((u32 *)DestRow) + ClipRect.Min.x;
        u32 *Source = ((u32 *)SourceRow) + 1 + ClipRect.Min.x - Floor(BitmapP.x);
        for(s32 X = ClipRect.Min.x;
            X < ClipRect.Max.x;
            ++X)
        {
            u32 *SourceW = Source;
            u32 *SourceX = SourceW + 1;
            u32 *SourceY = Source + Bitmap.Height;
            u32 *SourceZ = SourceY + 1;
            v4 SW = V4((r32)((*SourceW >> 16) & 0xFF),
                       (r32)((*SourceW >> 8) & 0xFF),
                       (r32)((*SourceW >> 0) & 0xFF),
                       (r32)((*SourceW >> 24) & 0xFF));
            v4 SX = V4((r32)((*SourceX >> 16) & 0xFF),
                       (r32)((*SourceX >> 8) & 0xFF),
                       (r32)((*SourceX >> 0) & 0xFF),
                       (r32)((*SourceX >> 24) & 0xFF));
            v4 SY = V4((r32)((*SourceY >> 16) & 0xFF),
                       (r32)((*SourceY >> 8) & 0xFF),
                       (r32)((*SourceY >> 0) & 0xFF),
                       (r32)((*SourceY >> 24) & 0xFF));
            v4 SZ = V4((r32)((*SourceZ >> 16) & 0xFF),
                       (r32)((*SourceZ >> 8) & 0xFF),
                       (r32)((*SourceZ >> 0) & 0xFF),
                       (r32)((*SourceZ >> 24) & 0xFF));

            // TODO(chris): Intuition says the Us and Vs should be swapped with Invs, but this works?
            v4 SJ = U*SW + InvU*SX;
            v4 SK = U*SY + InvU*SZ;

            v4 S = V*SJ + InvV*SK;
            u32 SA = (u32)(S.a + 0.5f);
            u32 SR = (u32)(S.r + 0.5f);
            u32 SG = (u32)(S.g + 0.5f);
            u32 SB = (u32)(S.b + 0.5f);

            r32 InvAlpha = 1.0f - (SA*Inv255);
            r32 DRed = InvAlpha*((*Dest >> 16) & 0xFF);
            r32 DGreen = InvAlpha*((*Dest >> 8) & 0xFF);
            r32 DBlue = InvAlpha*((*Dest >> 0) & 0xFF);

            u32 DR = SR + (u32)(DRed + 0.5f);
            u32 DG = SG + (u32)(DGreen + 0.5f);
            u32 DB = SB + (u32)(DBlue + 0.5f);

            *Dest = ((0xFF << 24) | // A
                     (DR   << 16) | // R
                     (DG   << 8)  | // G
                     (DB   << 0));  // B

            ++Dest;
            ++Source;
        }
        DestRow += BackBuffer->Pitch;
        SourceRow += Bitmap.Pitch;
    }
#endif
}

internal void
DrawRectangle(game_backbuffer *BackBuffer, rectangle2 Rect, v4 Color)
{
    s32 XMin = Clamp(0, RoundS32(Rect.Min.x), BackBuffer->Width);
    s32 YMin = Clamp(0, RoundS32(Rect.Min.y), BackBuffer->Height);
    s32 XMax = Clamp(0, RoundS32(Rect.Max.x), BackBuffer->Width);
    s32 YMax = Clamp(0, RoundS32(Rect.Max.y), BackBuffer->Height);
    
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
            u32 A = (u32)(Color.a*255.0f + 0.5f);
            u32 R = (u32)(Color.r*255.0f + 0.5f);
            u32 G = (u32)(Color.g*255.0f + 0.5f);
            u32 B = (u32)(Color.b*255.0f + 0.5f);
            *Dest++ = ((A << 24) | // A
                       (R << 16) | // R
                       (G << 8)  | // G
                       (B << 0));  // B
        }
        PixelRow += BackBuffer->Pitch;
    }
}

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
        State->Ship = LoadBitmap("ship.bmp");
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
    State->Yaw += 4.0f*Input->dtForFrame*-LeftStickX;
    v2 Facing = V2(Cos(State->Yaw), Sin(State->Yaw));
    if(WentDown(Controller->RightShoulder1) &&
       State->LiveBulletCount < ArrayCount(State->LiveBullets))
    {
        if(State->Cooldown <= 0.0f)
        {
            live_bullet *LiveBullet = State->LiveBullets + State->LiveBulletCount++;
            LiveBullet->P = State->P + 0.5f*Facing*(ShipScale + BulletScale);
            LiveBullet->Direction = State->Yaw;
            LiveBullet->Timer = 2.0f;
            State->Cooldown = 0.1f;
        }
    }

    r32 MetersPerPixel = 0.00027211314f;
    r32 PixelsPerMeter = 1.0f/MetersPerPixel;

    State->P += Facing*0.075f*PixelsPerMeter*Input->dtForFrame*Clamp01(Throttle);
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
            RenderBitmap(BackBuffer, State->Bullet, LiveBullet->P, XAxis, YAxis, BulletScale);
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
