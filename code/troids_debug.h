#if !defined(TROIDS_DEBUG_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#if TROIDS_INTERNAL
#define MAX_DEBUG_EVENTS 65536

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
    DebugEventType_DebugEvents,
    DebugEventType_FillBar,
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
    u32 ThreadID;
    b32 Ignored;
    u32 Line;
    union
    {
        r32 ElapsedSeconds;
        u32 Iterations;
    };
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
    u32 Iterations;
    u64 BeginTicks;
    u64 EndTicks;
    char *File;
    char *Name;

    union
    {
        profiler_element *Next;
        profiler_element *NextFree;
    };
    profiler_element *Parent;
    profiler_element *Child;
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

struct debug_thread
{
    u32 ID;
    
    profiler_element ProfilerSentinel;
    profiler_element *CurrentElement;
    profiler_element *NextElement;
};

#define MAX_DEBUG_FRAMES 128
#define MAX_DEBUG_THREADS 32
struct debug_frame
{
    r32 ElapsedSeconds;
    u64 BeginTicks;
    u64 EndTicks;

    // NOTE(chris): Per-frame hash for nodes.
    debug_node *NodeHash[256];
    
    u32 ThreadCount;
    debug_thread Threads[MAX_DEBUG_THREADS];
};

struct debug_state
{
    b32 IsInitialized;

    b32 Paused;
    memory_arena Arena;
    memory_arena StringArena;
    u32 EventStart;
    // TODO(chris): This has the unintended effect of ignoring everything. e.g., I want to ignore
    // only the events raised when drawing the debug display, but this would also ignore background
    // loading stuff.
    b32 Ignored;
    volatile u32 EventCount;
    u32 CollatingFrameIndex;
    u32 ViewingFrameIndex;
    debug_event NullEvent;
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
    u32 Mask = ArrayCount(GlobalDebugState->Events)-1;
    u32 EventIndex = ((AtomicIncrement(&GlobalDebugState->EventCount) - 1) & Mask);
                      
    Assert(((EventIndex+1)&Mask) != GlobalDebugState->EventStart);
    debug_event *Result = GlobalDebugState->Events + EventIndex;
    Result->ThreadID = GetCurrentThreadID();
    Result->Ignored = GlobalDebugState->Ignored;
    return(Result);
}

inline void
BeginTimedBlock(char *NameInit, u32 Iterations, char *FileInit, u32 LineInit)
{
    debug_event *Event = NextDebugEvent();
    Event->Type = DebugEventType_CodeBegin;
    Event->Value_u64 = __rdtsc();
    Event->Iterations = Iterations;
    Event->File = FileInit;
    Event->Line = LineInit;
    Event->Name = NameInit;
}
#define BEGIN_TIMED_BLOCK(Name) BeginTimedBlock(Name, 1, __FILE__, __LINE__)

inline void
EndTimedBlock()
{
    debug_event *Event = NextDebugEvent();
    Event->Type = DebugEventType_CodeEnd;
    Event->Value_u64 = __rdtsc();
}
#define END_TIMED_BLOCK() EndTimedBlock()

struct debug_timer
{
    debug_timer(char *NameInit, u32 Iterations, char *FileInit, u32 LineInit)
    {
        BeginTimedBlock(NameInit, Iterations, FileInit, LineInit);
    }
    ~debug_timer(void)
    {
        EndTimedBlock();
    }
};

struct debug_group
{
    debug_event *BeginEvent;
    
    debug_group(char *GUID, debug_event_type Type = DebugEventType_GroupBegin)
    {
        BeginEvent = NextDebugEvent();
        BeginEvent->Type = Type;
        BeginEvent->Name = GUID;
    }
    ~debug_group(void)
    {
        debug_event *Event = NextDebugEvent();
        Event->Type = DebugEventType_GroupEnd;
    }
};

#define TIMED_BLOCK__(Name, Iterations, File, Line, Counter) debug_timer Timer_##Counter(Name, Iterations, File, Line)
#define TIMED_BLOCK_(Name, Iterations, File, Line, Counter) TIMED_BLOCK__(Name, Iterations, File, Line, Counter)
#define TIMED_BLOCK(Name) TIMED_BLOCK_(Name, 1, __FILE__, __LINE__, __COUNTER__)
#define TIMED_LOOP(Name, Iterations) TIMED_BLOCK_(Name, Iterations, __FILE__, __LINE__, __COUNTER__)
#define TIMED_LOOP_FUNCTION(Iterations) TIMED_BLOCK_(__FUNCTION__, Iterations, __FILE__, __LINE__, __COUNTER__)
#define TIMED_FUNCTION() TIMED_BLOCK_(__FUNCTION__, 1, __FILE__, __LINE__, __COUNTER__)
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

#define DEBUG_GROUP__(Name, Type, Counter) debug_group Group_##Counter(DEBUG_GUID(Name), Type)
#define DEBUG_GROUP_(Name, Type, Counter) DEBUG_GROUP__(Name, Type, Counter)
#define DEBUG_GROUP(Name) DEBUG_GROUP_(Name, DebugEventType_GroupBegin, __COUNTER__)
#define DEBUG_SPECIAL_GROUP(Name, Type) DEBUG_GROUP_(Name, Type, __COUNTER__)

#define DEBUG_SUMMARY() DEBUG_SPECIAL_GROUP("DEBUG Summary", DebugEventType_Summary);

#define DEBUG_NAME(NameInit)                    \
    debug_event *Event = NextDebugEvent();      \
    Event->Type = DebugEventType_Name;          \
    Event->Name = DEBUG_GUID(NameInit);                                                 
     
#define DEBUG_VALUE(Name, Value) LogDebugValue(DEBUG_GUID(Name), Value);
#define DEBUG_FILL_BAR(NameInit, Used, Max)         \
    {                                               \
        debug_event *Event = NextDebugEvent();      \
        Event->Name = DEBUG_GUID(NameInit);         \
        Event->Type = DebugEventType_FillBar;       \
        Event->Value_v2 = V2((r32)Used, (r32)Max);  \
    }

#define LOG_DEBUG_FEATURE(TypeInit)             \
    {                                           \
        debug_event *Event = NextDebugEvent();  \
        Event->Type = TypeInit;                 \
        Event->Name = DEBUG_GUID(#TypeInit);     \
    }
#define DEBUG_PROFILER() LOG_DEBUG_FEATURE(DebugEventType_Profiler)
#define DEBUG_FRAME_TIMELINE() LOG_DEBUG_FEATURE(DebugEventType_FrameTimeline)
#define DEBUG_MEMORY() LOG_DEBUG_FEATURE(DebugEventType_DebugMemory)
#define DEBUG_EVENTS() LOG_DEBUG_FEATURE(DebugEventType_DebugEvents)

#define COLLATE_BLANK_TYPES(Event, Prev, GroupBeginStackCount, GroupBeginStack) \
    case(DebugEventType_Name):                                          \
    case(DebugEventType_Profiler):                                      \
    case(DebugEventType_FrameTimeline):                                 \
    case(DebugEventType_DebugMemory):                                   \
    case(DebugEventType_DebugEvents):                                   \
    {                                                                   \
        debug_node *Node = GetNode(Event);                              \
        LinkNode(Node, &Prev, GroupBeginStack, GroupBeginStackCount);   \
    } break;
#define COLLATE_VALUE_TYPE(TypeInit, Frame, Event, Prev, GroupBeginStackCount, GroupBeginStack) \
    case(DebugEventType_##TypeInit):                                    \
    {                                                                   \
        debug_node *Node = GetNode(Event);                              \
        LinkNode(Node, &Prev, GroupBeginStack, GroupBeginStackCount);   \
        debug_node *FrameNode = GetNode(Event, Frame);                  \
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
#define COLLATE_VALUE_TYPES(Frame, Event, Prev, GroupBeginStackCount, GroupBeginStack) \
    COLLATE_VALUE_TYPE(v2, Frame, Event, Prev, GroupBeginStackCount, GroupBeginStack) \
    COLLATE_VALUE_TYPE(r32, Frame, Event, Prev, GroupBeginStackCount, GroupBeginStack) \
    COLLATE_VALUE_TYPE(b32, Frame, Event, Prev, GroupBeginStackCount, GroupBeginStack) \
    COLLATE_VALUE_TYPE(u32, Frame, Event, Prev, GroupBeginStackCount, GroupBeginStack) \
    COLLATE_VALUE_TYPE(memory_arena, Frame, Event, Prev, GroupBeginStackCount, GroupBeginStack)
LOG_DEBUG_TYPE(v2)
LOG_DEBUG_TYPE(r32)
LOG_DEBUG_TYPE(b32)
LOG_DEBUG_TYPE(u32)
LOG_DEBUG_TYPE(memory_arena)

#else
#define TIMED_LOOP(...)
#define TIMED_BLOCK(...)
#define TIMED_FUNCTION(...)
#define BEGIN_TIMED_BLOCK(...)
#define END_TIMED_BLOCK(...)
#define FRAME_MARKER(...)
#define DEBUG_GROUP(...)
#define DEBUG_NAME(...)
#define DEBUG_VALUE(...)
#define DEBUG_PROFILER(...)
#define DEBUG_FRAME_TIMELINE(...)
#define DEBUG_SUMMARY(...)
#define DEBUG_MEMORY(...)
#define DEBUG_EVENTS(...)
#define DEBUG_FILL_BAR(...)
#endif
#define IGNORED_TIMED_LOOP_FUNCTION(...)
#define IGNORED_TIMED_BLOCK(...)
#define IGNORED_TIMED_FUNCTION(...)

#define TROIDS_DEBUG_H
#endif
