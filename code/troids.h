#if !defined(TROIDS_H)
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
#include "troids_render.h"
#include "troids_debug.h"
#include "troids_random.h"
#include "troids_physics.h"
#include "troids_play_mode.h"
#include "troids_title_screen_mode.h"

platform_read_file *PlatformReadFile;
platform_push_thread_work *PlatformPushThreadWork;

enum game_mode
{
    GameMode_TitleScreen,
    GameMode_Play,
};

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

    loaded_bitmap ShipBitmap;
    loaded_bitmap AsteroidBitmap;
    loaded_bitmap BulletBitmap;
    loaded_obj HeadMesh;

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
