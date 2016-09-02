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
    DebugEventType_Summary,
    DebugEventType_Profiler,
    DebugEventType_FrameTimeline,
    DebugEventType_DebugMemory,
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
    DebugEventType_memory_arena,
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
        memory_arena Value_memory_arena;
    };
    char *File;
    char *Name;
};

// TODO(chris): All node info should be persistent.
#if 0
struct debug_persistent_event
{
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

    union
    {
        debug_persistent_event *NextInHash;
        debug_persistent_event *NextFree;
    };
};
#endif

struct profiler_element
{
    u32 Line;
    u64 BeginTicks;
    u64 EndTicks;
    char *File;
    char *Name;

    union
    {
        profiler_element *Next;
        profiler_element *NextFree;
    };
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
        memory_arena Value_memory_arena;
    };
    union
    {
        char *Name;
        char *GUID;
    };
    
    debug_node *Next;
    debug_node *Child;

    union
    {
        debug_node *NextInHash;
        debug_node *NextFree;
    };
};

#define MAX_DEBUG_FRAMES 128
struct debug_frame
{
    r32 ElapsedSeconds;
    u64 BeginTicks;
    u64 EndTicks;

    // NOTE(chris): Per-frame hash for nodes.
    debug_node *NodeHash[256];
    
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
    u32 ViewingFrameIndex;
    debug_event Events[MAX_DEBUG_EVENTS];
    debug_frame Frames[MAX_DEBUG_FRAMES];

    debug_node NodeSentinel;
    debug_node *HotNode;

    debug_node *NodeHash[256];
    debug_node *FirstFreeNode;

    profiler_element *FirstFreeProfilerElement;

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

#define DEBUG_GUID__(Name, File, Line) Name##"|"##File##"|"###Line
#define DEBUG_GUID_(Name, File, Line) DEBUG_GUID__(Name, File, Line)
#define DEBUG_GUID(Name) DEBUG_GUID_(Name, __FILE__, __LINE__)

#define DEBUG_GROUP__(Name, Counter) debug_group Group_##Counter(DEBUG_GUID(Name))
#define DEBUG_GROUP_(Name, Counter) DEBUG_GROUP__(Name, Counter)
#define DEBUG_GROUP(Name) DEBUG_GROUP_(Name, __COUNTER__)

#define DEBUG_NAME(NameInit)                    \
    debug_event *Event = NextDebugEvent();      \
    Event->Type = DebugEventType_Name;          \
    Event->Name = DEBUG_GUID(NameInit);                                                 
     
#define DEBUG_VALUE(Name, Value) LogDebugValue(DEBUG_GUID(Name), Value);

#define LOG_DEBUG_FEATURE(TypeInit)             \
    {                                           \
        debug_event *Event = NextDebugEvent();  \
        Event->Type = TypeInit;                 \
        Event->Name = DEBUG_GUID(#TypeInit);     \
    }
#define DEBUG_PROFILER() LOG_DEBUG_FEATURE(DebugEventType_Profiler)
#define DEBUG_FRAME_TIMELINE() LOG_DEBUG_FEATURE(DebugEventType_FrameTimeline)
#define DEBUG_SUMMARY() LOG_DEBUG_FEATURE(DebugEventType_Summary)
#define DEBUG_MEMORY() LOG_DEBUG_FEATURE(DebugEventType_DebugMemory)

#define COLLATE_BLANK_TYPES                                             \
    case(DebugEventType_Name):                                          \
    case(DebugEventType_Profiler):                                      \
    case(DebugEventType_FrameTimeline):                                 \
    case(DebugEventType_DebugMemory):                                   \
    {                                                                   \
        debug_node *Node = GetNode(Event);                              \
        LinkNode(Node, &Prev, GroupBeginStack, GroupBeginStackCount);   \
    } break;
#define COLLATE_VALUE_TYPE(TypeInit)                                    \
    case(DebugEventType_##TypeInit):                                    \
    {                                                                   \
        debug_node *Node = GetNode(Event);                              \
        LinkNode(Node, &Prev, GroupBeginStack, GroupBeginStackCount);   \
        debug_node *FrameNode = GetNode(Frame, Event);                  \
        FrameNode->Value_##TypeInit = Event->Value_##TypeInit;          \
    }
#define LOG_DEBUG_TYPE(TypeInit)                    \
    inline void                                     \
    LogDebugValue(char *Name, TypeInit Value)       \
    {                                               \
        debug_event *Event = NextDebugEvent();      \
        Event->Name = Name;                         \
        Event->Type = DebugEventType_##TypeInit;    \
        Event->Value_##TypeInit = Value;            \
    }
#define COLLATE_VALUE_TYPES                     \
    COLLATE_VALUE_TYPE(v2)                      \
    COLLATE_VALUE_TYPE(r32)                     \
    COLLATE_VALUE_TYPE(b32)                     \
    COLLATE_VALUE_TYPE(memory_arena)
LOG_DEBUG_TYPE(v2)
LOG_DEBUG_TYPE(r32)
LOG_DEBUG_TYPE(b32)
LOG_DEBUG_TYPE(memory_arena)

#else
#define TIMED_BLOCK__(...)
#define TIMED_BLOCK_(...)
#define TIMED_BLOCK(...)
#define FRAME_MARKER(...)
#define DEBUG_GROUP(...)
#define DEBUG_NAME(...)
#define DEBUG_VALUE(...)
#define DEBUG_PROFILER(...)
#define DEBUG_FRAME_TIMELINE(...)
#endif

#define TROIDS_DEBUG_H
#endif
