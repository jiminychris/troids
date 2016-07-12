#if !defined(TROIDS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

struct live_bullet
{
    v2 P;
    r32 Direction;
    r32 Timer;
};

struct game_state
{
    b32 IsInitialized;
    
    u32 RunningSampleCount;
    
    v2 P;
    v2 dP;
    r32 dYaw;
    r32 Yaw;
    r32 Pitch;
    r32 Roll;
    r32 Cooldown;

    r32 Scale;
    
    u32 LiveBulletCount;
    live_bullet LiveBullets[64];

    loaded_bitmap Ship;
    loaded_bitmap Bullet;
    loaded_obj HeadMesh;
};

struct transient_state
{
    b32 IsInitialized;

    memory_arena TranArena;
};

#define TROIDS_H
#endif
