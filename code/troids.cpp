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
Clear(game_backbuffer *BackBuffer, v4 Color)
{
    u8 *PixelRow = (u8 *)BackBuffer->Memory;
    for(s32 Y = 0;
        Y < BackBuffer->Height;
        ++Y)
    {
        u32 *Dest = (u32 *)PixelRow;
        for(s32 X = 0;
            X < BackBuffer->Width;
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

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    game_state *State = (game_state *)GameMemory->PermanentMemory;
    if(!State->IsInitialized)
    {
        PlatformReadFile = GameMemory->PlatformReadFile;
        
        State->IsInitialized = true;

        State->P = V2(400.0f, 400.0f);
        State->Ship = LoadBitmap("ship.bmp");
    }

    transient_state *TranState = (transient_state *)GameMemory->PermanentMemory;
    if(!TranState->IsInitialized)
    {
        TranState->IsInitialized = true;
    }

    game_controller *Controller = Input->Controllers + 0;
    game_controller *Keyboard = &Input->Keyboard;

    r32 LeftStickX = Controller->LeftStickX;
    if(Keyboard->LeftStickX)
    {
        LeftStickX = Keyboard->LeftStickX;
    }
    r32 LeftStickY = Controller->LeftStickY;
    if(Keyboard->LeftStickY)
    {
        LeftStickY = Keyboard->LeftStickY;
    }

    State->P += 1.0f*V2(LeftStickX, LeftStickY);

    Clear(BackBuffer, V4(1.0f, 0.0f, 1.0f, 1.0f));

    // TODO(chris): DrawBitmap Function
    rectangle2i ClipRect;
    v2 BitmapP = V2(State->P.x - State->Ship.Width*State->Ship.Align.x,
                    State->P.y - State->Ship.Height*State->Ship.Align.y);
    ClipRect.Min.x = Clamp(0, Floor(BitmapP.x), BackBuffer->Width);
    ClipRect.Min.y = Clamp(0, Floor(BitmapP.y), BackBuffer->Height);
    ClipRect.Max.x = Clamp(0, Ceiling(BitmapP.x + State->Ship.Width - 2.0f), BackBuffer->Width);
    ClipRect.Max.y = Clamp(0, Ceiling(BitmapP.y + State->Ship.Height - 2.0f), BackBuffer->Height);
    r32 U = BitmapP.x - Floor(BitmapP.x);
    r32 V = BitmapP.y - Floor(BitmapP.y);
    r32 InvU = 1.0f - U;
    r32 InvV = 1.0f - V;
    r32 Inv255 = 1.0f / 255.0f;
    u8 *DestRow = (u8 *)BackBuffer->Memory + (BackBuffer->Pitch*ClipRect.Min.y);
    u8 *SourceRow = (u8 *)State->Ship.Memory + (State->Ship.Pitch*(ClipRect.Min.y - Floor(BitmapP.y)));
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
            u32 *SourceY = Source + State->Ship.Height;
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
        SourceRow += State->Ship.Pitch;
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
