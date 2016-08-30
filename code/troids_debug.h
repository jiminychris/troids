#if !defined(TROIDS_DEBUG_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#if TROIDS_INTERNAL
#define MAX_DEBUG_EVENTS 8192

enum debug_event_type
{
    DebugEventType_CodeBegin,
    DebugEventType_CodeEnd,
    DebugEventType_FrameMarker,
    DebugEventType_Data,
};

struct debug_event
{
    u32 Line;
    r32 ElapsedSeconds;
    debug_event_type Type;
    union
    {
        b32 Value_b32;
        u8 Value_u8;
        u16 Value_u16;
        u32 Value_u32;
        u64 Value_u64;
        s8 Value_s8;
        s16 Value_s16;
        s32 Value_s32;
        s64 Value_s64;
        r32 Value_r32;
        r64 Value_r64;
    };
    char *File;
    char *Name;
};

struct debug_element
{
    u32 Line;
    u64 BeginTicks;
    u64 EndTicks;
    char *File;
    char *Name;
    
    debug_element *Next;
};

#define MAX_DEBUG_FRAMES 128
struct debug_frame
{
    r32 ElapsedSeconds;
    u64 BeginTicks;
    u64 EndTicks;

    debug_element *FirstElement;
    debug_element *LastElement;
};

struct debug_state
{
    b32 IsInitialized;

    b32 Paused;
    memory_arena Arena;
    u32 EventCount;
    u32 FrameIndex;
    u32 LastFrameIndex;
    debug_event Events[MAX_DEBUG_EVENTS];
    debug_frame Frames[MAX_DEBUG_FRAMES];
};

global_variable debug_state *GlobalDebugState;

struct debug_timer
{
    debug_timer(char *FileInit, u32 LineInit, char *NameInit)
    {
        Assert(GlobalDebugState->EventCount < ArrayCount(GlobalDebugState->Events));
        debug_event *Event = GlobalDebugState->Events + GlobalDebugState->EventCount++;
        Event->Type = DebugEventType_CodeBegin;
        Event->Value_u64 = __rdtsc();
        Event->File = FileInit;
        Event->Line = LineInit;
        Event->Name = NameInit;
    }
    ~debug_timer(void)
    {
        Assert(GlobalDebugState->EventCount < ArrayCount(GlobalDebugState->Events));
        debug_event *Event = GlobalDebugState->Events + GlobalDebugState->EventCount++;
        Event->Type = DebugEventType_CodeEnd;
        Event->Value_u64 = __rdtsc();
    }
};

#define TIMED_BLOCK__(Name, File, Line, Counter) debug_timer Timer_##Counter(File, Line, #Name)
#define TIMED_BLOCK_(Name, File, Line, Counter) TIMED_BLOCK__(Name, File, Line, Counter)
#define TIMED_BLOCK(Name) TIMED_BLOCK_(Name, __FILE__, __LINE__, __COUNTER__)
#define FRAME_MARKER(FrameSeconds)                                      \
    {                                                                   \
        Assert(GlobalDebugState->EventCount < ArrayCount(GlobalDebugState->Events)); \
        debug_event *Event = GlobalDebugState->Events + GlobalDebugState->EventCount++; \
        Event->Type = DebugEventType_FrameMarker;                       \
        Event->ElapsedSeconds = FrameSeconds;                           \
        Event->Value_u64 = __rdtsc();                                   \
    }
#else
#define TIMED_BLOCK__(...)
#define TIMED_BLOCK_(...)
#define TIMED_BLOCK(...)
#define FRAME_MARKER(...)
#endif

#define TROIDS_DEBUG_H
#endif
