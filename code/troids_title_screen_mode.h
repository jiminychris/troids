#if !defined(TROIDS_TITLE_SCREEN_MODE_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

struct title_screen_state
{
    u32 FadeInTicks;
    r32 FlickerTicks;
    b32 PressedStart;
    title_screen_selection Selection;
    b32 Debounce;
};

#define TROIDS_TITLE_SCREEN_MODE_H
#endif
