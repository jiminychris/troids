#if !defined(TROIDS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#define DEBUG_CAMERA 0

#include "troids_platform.h"
#include "troids_intrinsics.h"
#include "troids_math.h"
#include "troids_debug.h"
#include "troids_render.h"
#include "troids_random.h"
#include "troids_physics.h"

platform_read_file *PlatformReadFile;
platform_push_thread_work *PlatformPushThreadWork;
platform_wait_for_all_thread_work *PlatformWaitForAllThreadWork;

enum game_mode
{
    GameMode_TitleScreen,
    GameMode_Play,
};

enum play_type
{
    PlayType_Journey,
    PlayType_Arcade,

    PlayType_Terminator,
};

#include "troids_play_mode.h"
#include "troids_title_screen_mode.h"

struct game_state
{
    b32 IsInitialized;

    r32 MetersToPixels;
    u32 RunningSampleCount;

    game_mode Mode;
    game_mode NextMode;
    union
    {
        play_state PlayState;
        title_screen_state TitleScreenState;
    };

    loaded_bitmap LaserBitmap;

    memory_arena Arena;
};

struct transient_state
{
    b32 IsInitialized;

    memory_arena Arena;
    render_buffer RenderBuffer;
};

#define TROIDS_H
#endif
