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
    game_controller *Controller = Input->Controllers + 0;
    game_controller *Keyboard = &Input->Keyboard;
#else
    game_controller *Controller = &Input->Keyboard;
#endif

    r32 LeftStickX;
    r32 LeftStickY;
    if(Keyboard->LeftStickX)
    {
        LeftStickX = Keyboard->LeftStickX;
    }
    else
    {
        LeftStickX = Controller->LeftStickX;
    }
    if(Keyboard->LeftStickY)
    {
        LeftStickY = Keyboard->LeftStickY;
    }
    else
    {
        LeftStickY = Controller->LeftStickY;
    }
    State->RBase += Input->dtForFrame*LeftStickX*0.1f;
    State->GBase += Input->dtForFrame*LeftStickY*0.1f;
    State->BBase -= Input->dtForFrame*Controller->LeftTrigger*0.1f;
    State->BBase += Input->dtForFrame*Controller->RightTrigger*0.1f;

    State->RBase = Clamp01(State->RBase);
    State->GBase = Clamp01(State->GBase);
    State->BBase = Clamp01(State->BBase);

    u8 R = (u8)(State->RBase*255.0f + 0.5f);
    u8 G = (u8)(State->GBase*255.0f + 0.5f);
    u8 B = (u8)(State->BBase*255.0f + 0.5f);

    for(s32 Y = 0;
        Y < BackBuffer->Height;
        ++Y)
    {
        for(s32 X = 0;
            X < BackBuffer->Width;
            ++X)
        {
            u32 *Pixel = ((u32 *)PixelRow) + X;
            *Pixel = ((0xFF << 24) | // A
                      (R    << 16) | // R
                      (G    << 8)  | // G
                      (B    << 0));  // B

        }
        PixelRow += BackBuffer->Pitch;
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
