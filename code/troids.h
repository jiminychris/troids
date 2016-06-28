#if !defined(TROIDS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

struct game_state
{
    b32 IsInitialized;
    
    u32 RunningSampleCount;
    v2 P;

    loaded_bitmap Ship;
};

struct transient_state
{
    b32 IsInitialized;
};

#define TROIDS_H
#endif