/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include "troids_platform.h"
#include "troids_math.h"

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    u8 *PixelRow = (u8 *)BackBuffer->Memory;
    r32 BlendMultiplier = 1.0f / (r32)(BackBuffer->Width - 1);
#if 1
    r32 RBase = (Sin(3*State->tSin) + 1.0f)*127.5f;
    r32 GBase = (Sin(7 + 2*State->tSin) + 1.0f)*127.5f;
    r32 BBase = (Sin(5 + State->tSin) + 1.0f)*127.5f;
#else
    r32 RBase = (Sin(State->tSin) + 1.0f)*127.5f;
    r32 GBase = (Sin(State->tSin) + 1.0f)*127.5f;
    r32 BBase = (Sin(State->tSin) + 1.0f)*127.5f;
#endif
    for(s32 Y = 0;
        Y < BackBuffer->Height;
        ++Y)
    {
        for(s32 X = 0;
            X < BackBuffer->Width;
            ++X)
        {
            u32 *Pixel = ((u32 *)PixelRow) + X;
            r32 Blend = (r32)X*BlendMultiplier;
            u8 R = (u8)(RBase*Blend + 0.5f);
            u8 G = (u8)(GBase*Blend + 0.5f);
            u8 B = (u8)(BBase*Blend + 0.5f);
            *Pixel = ((0xFF << 24) | // A
                      (R    << 16) | // R
                      (G    << 8)  | // G
                      (B    << 0));  // B

        }
        PixelRow += BackBuffer->Pitch;
    }

    State->tSin += Input->dtForFrame;
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    r32 tSample = State->tSample;
    u32 SampleSize = SoundBuffer->Channels*SoundBuffer->BitsPerSample / 8;
    r32 SecondsPerSample = 1.0f / SoundBuffer->SamplesPerSecond;
    u32 TotalSampleCount = SoundBuffer->Size / SampleSize;

    // NOTE(chris): Assumes sample size
    u16 *Sample = (u16 *)SoundBuffer->Region1;
    for(u32 SampleIndex = 0;
        SampleIndex < SoundBuffer->Region1Size;
        SampleIndex += SampleSize)
    {
        s16 Value = (s16)((400.0f*Sin(Tau*440.0f*tSample)) + 0.5f);
        tSample += SecondsPerSample;
        for(u8 Channel = 0;
            Channel < SoundBuffer->Channels;
            ++Channel)
        {
            *Sample++ = Value;
        }
        ++SoundBuffer->RunningSampleCount;
    }
    Sample = (u16 *)SoundBuffer->Region2;
    for(u32 SampleIndex = 0;
        SampleIndex < SoundBuffer->Region2Size;
        SampleIndex += SampleSize)
    {
        s16 Value = (s16)((400.0f*Sin(Tau*440.0f*tSample)) + 0.5f);
        tSample += SecondsPerSample;
        for(u8 Channel = 0;
            Channel < SoundBuffer->Channels;
            ++Channel)
        {
            *Sample++ = Value;
        }
        ++SoundBuffer->RunningSampleCount;
    }
    if(SoundBuffer->RunningSampleCount >= TotalSampleCount)
    {
        SoundBuffer->RunningSampleCount -= TotalSampleCount;
    }
}
