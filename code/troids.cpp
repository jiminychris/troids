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
    for(s32 Y = 0;
        Y < BackBuffer->Height;
        ++Y)
    {
        for(s32 X = 0;
            X < BackBuffer->Width;
            ++X)
        {
            u32 *Pixel = ((u32 *)PixelRow) + X;
            r32 Blend = (r32)X / (r32)(BackBuffer->Width - 1);
            u8 R = (u8)(((Sin(19*State->tSin) + 1.0f)*127.5f) + 0.5f);
            u8 G = (u8)(((Sin(19*State->tSin) + 1.0f)*127.5f) + 0.5f);
            u8 B = (u8)(((Sin(19*State->tSin) + 1.0f)*127.5f) + 0.5f);
            *Pixel = ((0xFF << 24) | // A
                      (R    << 16) | // R
                      (G    << 8)  | // G
                      (B    << 0));  // B

        }
        PixelRow += BackBuffer->Pitch;
    }

    State->tSin += Input->dtForFrame;
}
