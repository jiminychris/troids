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
    DebugEventType_FrameMarker,
    DebugEventType_CodeBegin,
    DebugEventType_CodeEnd,
    DebugEventType_GroupBegin,
    DebugEventType_GroupEnd,
    DebugEventType_Name,
    DebugEventType_Profiler,
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

struct debug_persistent_event
{
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
    char *GUID;
};

struct profiler_element
{
    u32 Line;
    u64 BeginTicks;
    u64 EndTicks;
    char *File;
    char *Name;
    
    profiler_element *Next;
};

struct debug_node
{
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
    union
    {
        char *Name;
        char *GUID;
    };
    
    debug_node *Next;
    debug_node *Parent;
    debug_node *Child;
};

#define MAX_DEBUG_FRAMES 128
struct debug_frame
{
    r32 ElapsedSeconds;
    u64 BeginTicks;
    u64 EndTicks;

    debug_node *FirstNode;
    debug_node *CurrentNode;
    
    profiler_element *FirstElement;
    profiler_element *LastElement;
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

    debug_persistent_event *PersistentEventHash;

    loaded_font Font;
};

global_variable debug_state *GlobalDebugState;

inline debug_event *
NextDebugEvent()
{
    Assert(GlobalDebugState->EventCount < ArrayCount(GlobalDebugState->Events));
    debug_event *Result = GlobalDebugState->Events + GlobalDebugState->EventCount++;
    return(Result);
}

struct debug_timer
{
    debug_timer(char *FileInit, u32 LineInit, char *NameInit)
    {
        debug_event *Event = NextDebugEvent();
        Event->Type = DebugEventType_CodeBegin;
        Event->Value_u64 = __rdtsc();
        Event->File = FileInit;
        Event->Line = LineInit;
        Event->Name = NameInit;
    }
    ~debug_timer(void)
    {
        debug_event *Event = NextDebugEvent();
        Event->Type = DebugEventType_CodeEnd;
        Event->Value_u64 = __rdtsc();
    }
};

struct debug_group
{
    debug_group(char *GUID)
    {
        debug_event *Event = NextDebugEvent();
        Event->Type = DebugEventType_GroupBegin;
        Event->Name = GUID;
    }
    ~debug_group(void)
    {
        debug_event *Event = NextDebugEvent();
        Event->Type = DebugEventType_GroupEnd;
    }
};

#define TIMED_BLOCK__(Name, File, Line, Counter) debug_timer Timer_##Counter(File, Line, #Name)
#define TIMED_BLOCK_(Name, File, Line, Counter) TIMED_BLOCK__(Name, File, Line, Counter)
#define TIMED_BLOCK(Name) TIMED_BLOCK_(Name, __FILE__, __LINE__, __COUNTER__)
#define FRAME_MARKER(FrameSeconds)                                      \
    {                                                                   \
        debug_event *Event = NextDebugEvent();          \
        Event->Type = DebugEventType_FrameMarker;                       \
        Event->ElapsedSeconds = FrameSeconds;                           \
        Event->Value_u64 = __rdtsc();                                   \
    }

#define DEBUG_GROUP__(Name, File, Line, Counter) debug_group Group_##Counter(Name##"|"##File##"|"###Line)
#define DEBUG_GROUP_(Name, File, Line, Counter) DEBUG_GROUP__(Name, File, Line, Counter)
#define DEBUG_GROUP(Name) DEBUG_GROUP_(Name, __FILE__, __LINE__, __COUNTER__)

#define DEBUG_NAME(NameInit)                    \
    debug_event *Event = NextDebugEvent();      \
    Event->Type = DebugEventType_Name;          \
    Event->Name = #NameInit;                                                 

#define LOG_DEBUG_TYPE(TypeInit)                    \
    inline void                                     \
    LogDebugValue(char *Name, TypeInit Value)       \
    {                                               \
        debug_event *Event = NextDebugEvent();      \
        Event->Name = Name;                         \
        Event->Type = DebugEventType_##TypeInit;    \
        Event->Value_##TypeInit = Value;            \
    }

LOG_DEBUG_TYPE(v2)
LOG_DEBUG_TYPE(r32)
LOG_DEBUG_TYPE(b32)
     
#define DEBUG_VALUE(Name, Value) LogDebugValue(#Name, Value);

#define COLLATE_DEBUG_TYPE(TypeInit)                                    \
    case(DebugEventType_##TypeInit):                                    \
    {                                                                   \
        debug_node *Node = AllocateDebugNode(Frame, Event->Type, Event->Name); \
        Node->Value_##TypeInit = Event->Value_##TypeInit;               \
    } break;
#define COLLATE_DEBUG_TYPES(Frame, Event)       \
    COLLATE_DEBUG_TYPE(v2)                      \
    COLLATE_DEBUG_TYPE(r32)                     \
    COLLATE_DEBUG_TYPE(b32)                     \

#else
#define TIMED_BLOCK__(...)
#define TIMED_BLOCK_(...)
#define TIMED_BLOCK(...)
#define FRAME_MARKER(...)
#define DEBUG_GROUP(...)
#define DEBUG_NAME(...)
#define DEBUG_VALUE(...)
#endif

#define TROIDS_DEBUG_H
#endif
