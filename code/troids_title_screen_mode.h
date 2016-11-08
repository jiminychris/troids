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
    r32 FadeInTimer;
    r32 FlickerTimer;
    b32 PressedStart;
    title_screen_selection Selection;
    b32 Debounce;
};

#define TROIDS_TITLE_SCREEN_MODE_H
#endif
