#if !defined(TROIDS_PLATFORM_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include <stdint.h>

#define global_variable static
#define internal static

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float r32;
typedef double r64;

typedef int32_t b32;

#define Assert(Expr) {if(!(Expr)) *((int *)0);}

struct game_state
{
    r32 tSin;
};

struct game_input
{
    r32 dtForFrame;
};

struct game_backbuffer
{
    s32 Width;
    s32 Height;
    s32 Pitch;
    void *Memory;
};

#define GAME_UPDATE_AND_RENDER(Name) void Name(game_state *State, game_input *Input, game_backbuffer *BackBuffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define TROIDS_PLATFORM_H
#endif
