#if !defined(TROIDS_DEBUG_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#if TROIDS_INTERNAL
#define MAX_DEBUG_EVENTS 64

enum debug_event_type
{
    DebugEventType_CodeBegin,
    DebugEventType_CodeEnd,
    DebugEventType_FrameMarker,
};

struct debug_event
{
    u32 Line;
    debug_event_type Type;
    union
    {
        u32 Value_u32;
        u64 Value_u64;
        r32 Value_r32;
    };
    char *File;
    char *Name;
};

struct debug_state
{
    b32 IsInitialized;
    
    memory_arena Arena;
    u32 EventCount;
    debug_event Events[MAX_DEBUG_EVENTS];
};

global_variable debug_state *GlobalDebugState;

struct debug_timer
{
    debug_timer(char *FileInit, u32 LineInit, char *NameInit)
    {
        if(GlobalDebugState)
        {
            debug_event *Event = GlobalDebugState->Events + GlobalDebugState->EventCount++;
            Event->Type = DebugEventType_CodeBegin;
            Event->Value_u64 = __rdtsc();
            Event->File = FileInit;
            Event->Line = LineInit;
            Event->Name = NameInit;
        }
    }
    ~debug_timer(void)
    {
        if(GlobalDebugState)
        {
            debug_event *Event = GlobalDebugState->Events + GlobalDebugState->EventCount++;
            Event->Type = DebugEventType_CodeEnd;
            Event->Value_u64 = __rdtsc();
        }
    }
};

#define TIMED_BLOCK__(Name, File, Line, Counter) debug_timer Timer_##Counter(File, Line, #Name)
#define TIMED_BLOCK_(Name, File, Line, Counter) TIMED_BLOCK__(Name, File, Line, Counter)
#define TIMED_BLOCK(Name) TIMED_BLOCK_(Name, __FILE__, __LINE__, __COUNTER__)
#define FRAME_MARKER(FrameSeconds)                                      \
    {                                                                   \
        if(GlobalDebugState)                                            \
        {                                                               \
            debug_event *Event = GlobalDebugState->Events + GlobalDebugState->EventCount++; \
            Event->Type = DebugEventType_FrameMarker;                   \
            Event->Value_r32 = FrameSeconds;                            \
        }                                                               \
    }
#else
#define TIMED_BLOCK__(...)
#define TIMED_BLOCK_(...)
#define TIMED_BLOCK(...)
#define FRAME_MARKER(...)
#endif

#define TROIDS_DEBUG_H
#endif
