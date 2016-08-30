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
    DebugEventType_Profiler,
    DebugEventType_Group,
    DebugEventType_Name,
    DebugEventType_b32,
    DebugEventType_u8,
    DebugEventType_u16,
    DebugEventType_u32,
    DebugEventType_u64,
    DebugEventType_s8,
    DebugEventType_s16,
    DebugEventType_s32,
    DebugEventType_s64,
    DebugEventType_r32,
    DebugEventType_r64,
    DebugEventType_v2,
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
        v2 Value_v2;
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

inline debug_event *
NextDebugEvent(debug_state *DebugState)
{
    Assert(GlobalDebugState->EventCount < ArrayCount(GlobalDebugState->Events));
    debug_event *Result = GlobalDebugState->Events + GlobalDebugState->EventCount++;
    return(Result);
}

struct debug_timer
{
    debug_timer(char *FileInit, u32 LineInit, char *NameInit)
    {
        debug_event *Event = NextDebugEvent(GlobalDebugState);
        Event->Type = DebugEventType_CodeBegin;
        Event->Value_u64 = __rdtsc();
        Event->File = FileInit;
        Event->Line = LineInit;
        Event->Name = NameInit;
    }
    ~debug_timer(void)
    {
        debug_event *Event = NextDebugEvent(GlobalDebugState);
        Event->Type = DebugEventType_CodeEnd;
        Event->Value_u64 = __rdtsc();
    }
};

#define TIMED_BLOCK__(Name, File, Line, Counter) debug_timer Timer_##Counter(File, Line, #Name)
#define TIMED_BLOCK_(Name, File, Line, Counter) TIMED_BLOCK__(Name, File, Line, Counter)
#define TIMED_BLOCK(Name) TIMED_BLOCK_(Name, __FILE__, __LINE__, __COUNTER__)
#define FRAME_MARKER(FrameSeconds)                                      \
    {                                                                   \
        debug_event *Event = NextDebugEvent(GlobalDebugState);          \
        Event->Type = DebugEventType_FrameMarker;                       \
        Event->ElapsedSeconds = FrameSeconds;                           \
        Event->Value_u64 = __rdtsc();                                   \
    }
inline void
LogDebugValue(char *Name, v2 Value)
{
    debug_event *Event = NextDebugEvent(GlobalDebugState);
    Event->Type = DebugEventType_v2;
    Event->Name = Name;
    Event->Value_v2 = Value;
}
#define DEBUG_GROUP(NameInit)                               \
    local_persist b32 NameInit = false;                     \
    debug_event *Event = NextDebugEvent(GlobalDebugState);  \
    Event->Type = DebugEventType_Group;                     \
    Event->Name = #NameInit;                                \
    Event->Value_b32 = NameInit;
#define DEBUG_NAME(NameInit)                                            \
    debug_event *Event = NextDebugEvent(GlobalDebugState);              \
    Event->Type = DebugEventType_Name;                                  \
    Event->Name = #NameInit;                                                 
#define DEBUG_VALUE(NameInit, Value) LogDebugValue(#NameInit, Value)
#else
#define TIMED_BLOCK__(...)
#define TIMED_BLOCK_(...)
#define TIMED_BLOCK(...)
#define FRAME_MARKER(...)
#define DEBUG_GROUP(...)
#define DEBUG_NAME(...)
#endif

#define TROIDS_DEBUG_H
#endif
