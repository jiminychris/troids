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
    union
    {
        r32 ElapsedSeconds;
        u32 Iterations;
        u32 FrameIndex;
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
    char *GUID;
};

struct profiler_element
{
    u32 Iterations;
    u64 BeginTicks;
    u64 EndTicks;
    char *GUID;

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

struct hashed_string
{
    hashed_string *NextInHash;
    // NOTE(chris): Stored after this is a null-terminated string
};

struct debug_thread
{
    u32 ID;
    
    profiler_element ProfilerSentinel;
    profiler_element *CurrentElement;
    profiler_element *NextElement;

    profiler_element *CollatingElement;
};

struct debug_thread_storage
{
    u32 ID;
    
    memory_arena StringArena;
    // TODO(chris): These never get freed!
    hashed_string *StringHash[256];
};

#define MAX_DEBUG_FRAMES 128
#define MAX_DEBUG_THREADS 8
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
    u32 EventStart;
    // TODO(chris): This has the unintended effect of ignoring everything. e.g., I want to ignore
    // only the events raised when drawing the debug display, but this would also ignore background
    // loading stuff.
    b32 Ignored;
    volatile u32 EventWriteCount;
    volatile u32 EventReadCount;
    u32 CollatingFrameIndex;
    u32 ViewingFrameIndex;
    debug_event NullEvent;
    debug_event Events[MAX_DEBUG_EVENTS];
    debug_frame Frames[MAX_DEBUG_FRAMES];

    debug_node NodeSentinel;
    debug_node *HotNode;

    debug_node *NodeHash[256];
    debug_node *FirstFreeNode;

    u32 ThreadCount;
    debug_thread_storage ThreadStorage[MAX_DEBUG_THREADS];
    
    profiler_element *FirstFreeProfilerElement;

    loaded_font Font;
};

global_variable debug_state *GlobalDebugState;

inline debug_thread_storage *
GetThreadStorage(u32 ThreadID)
{
    debug_thread_storage *Thread = 0;
    b32 Found = false;
    for(u32 ThreadIndex = 0;
        ThreadIndex < GlobalDebugState->ThreadCount;
        ++ThreadIndex)
    {
        Thread = GlobalDebugState->ThreadStorage + ThreadIndex;
        if(Thread->ID == ThreadID)
        {
            Found = true;
            break;
        }
    }
    if(!Found)
    {
        Assert((GlobalDebugState->ThreadCount == 0) ||
               (GetCurrentThreadID() == GlobalDebugState->ThreadStorage[0].ID));
        Assert(GlobalDebugState->ThreadCount < ArrayCount(GlobalDebugState->ThreadStorage));
        Thread = GlobalDebugState->ThreadStorage + GlobalDebugState->ThreadCount++;
        Thread->ID = ThreadID;
        Thread->StringArena = SubArena(&GlobalDebugState->Arena, Megabytes(16));
        for(u32 HashIndex = 0;
            HashIndex < ArrayCount(Thread->StringHash);
            ++HashIndex)
        {
            Assert(Thread->StringHash[HashIndex] == 0);
        }
    }
    return(Thread);
}

inline u32
HashString(char *String)
{
    Assert(String);
    u32 Result = 0;
    for(char *At = String;
        *At;
        ++At)
    {
        Result += Result*65521 + *At;
    }
    return(Result);
}

inline char *
CopyString(memory_arena *Arena, char *String)
{
    char *Result = (char *)PushSize(Arena, 0);
    do
    {
        *PushStruct(Arena, char) = *String;
    } while(*String++);
    return(Result);
}

inline char *
GetStringFromHash(char *String, debug_thread_storage *Thread = GlobalDebugState->ThreadStorage)
{
    char *Result = 0;
    
    u32 HashValue = HashString(String);
    u32 Index = HashValue & (ArrayCount(Thread->StringHash) - 1);
    Assert(Index < ArrayCount(Thread->StringHash));
    hashed_string *FirstInHash = Thread->StringHash[Index];

    for(hashed_string *Chain = FirstInHash;
        Chain;
        Chain = Chain->NextInHash)
    {
        char *HashedString = (char *)(Chain + 1);
        if(StringsMatch(String, HashedString))
        {
            Result = HashedString;
            break;
        }
    }

    if(!Result)
    {
        hashed_string *Header = PushStruct(&Thread->StringArena, hashed_string);
        Result = CopyString(&Thread->StringArena, String);
        Header->NextInHash = FirstInHash;
        Thread->StringHash[Index] = Header;
    }

    return(Result);
}

#define DEBUG_GUID__(Name, File, Line) Name##"|"##File##"|"###Line
#define DEBUG_GUID_(Name, File, Line) DEBUG_GUID__(Name, File, Line)
#define DEBUG_GUID(Name) DEBUG_GUID_(Name, __FILE__, __LINE__)

struct next_debug_event_result
{
    u32 EventCount;
    debug_event *Event;
};

inline next_debug_event_result
BeginNextDebugEvent(char *GUID = "")
{
    next_debug_event_result Result;
    u32 Mask = ArrayCount(GlobalDebugState->Events)-1;
    Result.EventCount = AtomicIncrement(&GlobalDebugState->EventWriteCount);
    u32 EventIndex = ((Result.EventCount - 1) & Mask);
                      
    Assert(((EventIndex+1)&Mask) != GlobalDebugState->EventStart);
    Result.Event = GlobalDebugState->Events + EventIndex;
    Result.Event->ThreadID = GetCurrentThreadID();
    // TODO(chris): Does this impact the calling code too much?
    Result.Event->GUID = GetStringFromHash(GUID, GetThreadStorage(Result.Event->ThreadID));
    Result.Event->Ignored = GlobalDebugState->Ignored;

    return(Result);
}

inline void
EndNextDebugEvent(u32 EventCount)
{
    b32 Exchanged;
    do
    {
        Exchanged = AtomicCompareExchange(&GlobalDebugState->EventReadCount, EventCount, EventCount-1);
    } while(!Exchanged);
}

inline char *
BeginTimedBlock(char *GUID, u32 Iterations)
{
    next_debug_event_result Next = BeginNextDebugEvent(GUID);
    Next.Event->Type = DebugEventType_CodeBegin;
    Next.Event->Value_u64 = __rdtsc();
    Next.Event->Iterations = Iterations;
    EndNextDebugEvent(Next.EventCount);
    return(Next.Event->GUID);
}
#define BEGIN_TIMED_BLOCK(GUIDName, Name) char *GUIDName = BeginTimedBlock(DEBUG_GUID(Name), 1);

inline void
EndTimedBlock(char *GUID)
{
    next_debug_event_result Next = BeginNextDebugEvent(GUID);
    Next.Event->Type = DebugEventType_CodeEnd;
    Next.Event->Value_u64 = __rdtsc();
    EndNextDebugEvent(Next.EventCount);
}
#define END_TIMED_BLOCK(GUID) EndTimedBlock(GUID)

struct debug_timer
{
    char *GUID;
    debug_timer(char *GUIDInit, u32 Iterations)
    {
        GUID = BeginTimedBlock(GUIDInit, Iterations);
    }
    ~debug_timer(void)
    {
        EndTimedBlock(GUID);
    }
};

struct debug_group
{
    debug_event *BeginEvent;
    
    debug_group(char *GUID, debug_event_type Type = DebugEventType_GroupBegin)
    {
        next_debug_event_result Next = BeginNextDebugEvent(GUID);
        BeginEvent = Next.Event;
        BeginEvent->Type = Type;
        EndNextDebugEvent(Next.EventCount);
    }
    ~debug_group(void)
    {
        next_debug_event_result Next = BeginNextDebugEvent();
        Next.Event->GUID = BeginEvent->GUID;
        Next.Event->Type = DebugEventType_GroupEnd;
        EndNextDebugEvent(Next.EventCount);
    }
};

#define TIMED_BLOCK__(Name, Iterations, Counter) debug_timer Timer_##Counter(DEBUG_GUID(Name), Iterations)
#define TIMED_BLOCK_(Name, Iterations, Counter) TIMED_BLOCK__(Name, Iterations, Counter)
#define TIMED_BLOCK(Name) TIMED_BLOCK_(Name, 1, __COUNTER__)
#define TIMED_LOOP(Name, Iterations) TIMED_BLOCK_(Name, Iterations, __COUNTER__)
#define TIMED_LOOP_FUNCTION(Iterations) TIMED_LOOP(__FUNCTION__, Iterations)
#define TIMED_FUNCTION() TIMED_BLOCK_(__FUNCTION__, 1, __COUNTER__)
#define FRAME_MARKER(FrameSeconds)                              \
    {                                                           \
        next_debug_event_result Next = BeginNextDebugEvent();   \
        Next.Event->Type = DebugEventType_FrameMarker;          \
        Next.Event->ElapsedSeconds = FrameSeconds;              \
        Next.Event->Value_u64 = __rdtsc();                      \
        EndNextDebugEvent(Next.EventCount);                     \
    }

#define DEBUG_GROUP__(Name, Type, Counter) debug_group Group_##Counter(DEBUG_GUID(Name), Type)
#define DEBUG_GROUP_(Name, Type, Counter) DEBUG_GROUP__(Name, Type, Counter)
#define DEBUG_GROUP(Name) DEBUG_GROUP_(Name, DebugEventType_GroupBegin, __COUNTER__)
#define DEBUG_SPECIAL_GROUP(Name, Type) DEBUG_GROUP_(Name, Type, __COUNTER__)

#define DEBUG_SUMMARY() DEBUG_SPECIAL_GROUP("DEBUG Summary", DebugEventType_Summary);

#define DEBUG_NAME(NameInit)                    \
    debug_event *Event = NextDebugEvent(DEBUG_GUID(NameInit));  \
    Event->Type = DebugEventType_Name;
     
#define DEBUG_VALUE(Name, Value) LogDebugValue(DEBUG_GUID(Name), Value);
#define DEBUG_FILL_BAR(NameInit, Used, Max)         \
    {                                               \
        next_debug_event_result Next = BeginNextDebugEvent(DEBUG_GUID(NameInit));  \
        Next.Event->Type = DebugEventType_FillBar;       \
        Next.Event->Value_v2 = V2((r32)Used, (r32)Max);  \
        EndNextDebugEvent(Next.EventCount);                        \
    }

#define LOG_DEBUG_FEATURE(TypeInit)             \
    {                                           \
        next_debug_event_result Next = BeginNextDebugEvent(DEBUG_GUID(#TypeInit)); \
        Next.Event->Type = TypeInit;                 \
        EndNextDebugEvent(Next.EventCount);                    \
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
#define LOG_DEBUG_TYPE(TypeInit)                                \
    inline void                                                 \
    LogDebugValue(char *GUID, TypeInit Value)                   \
    {                                                               \
        next_debug_event_result Next = BeginNextDebugEvent(GUID);   \
        Next.Event->Type = DebugEventType_##TypeInit;               \
        Next.Event->Value_##TypeInit = Value;                       \
        EndNextDebugEvent(Next.EventCount);                         \
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

#define LOG_CONTROLLER(Controller)                                      \
    {                                                                   \
        DEBUG_VALUE("LeftStick", (Controller)->LeftStick);              \
        DEBUG_VALUE("RightStick", (Controller)->RightStick);            \
        DEBUG_VALUE("LeftTrigger", (Controller)->LeftTrigger);          \
        DEBUG_VALUE("RightTrigger", (Controller)->RightTrigger);        \
        DEBUG_VALUE("ActionUp", (Controller)->ActionUp.EndedDown);      \
        DEBUG_VALUE("ActionLeft", (Controller)->ActionLeft.EndedDown);  \
        DEBUG_VALUE("ActionDown", (Controller)->ActionDown.EndedDown);  \
        DEBUG_VALUE("ActionRight", (Controller)->ActionRight.EndedDown); \
        DEBUG_VALUE("LeftShoulder1", (Controller)->LeftShoulder1.EndedDown); \
        DEBUG_VALUE("RightShoulder1", (Controller)->RightShoulder1.EndedDown); \
        DEBUG_VALUE("LeftShoulder2", (Controller)->LeftShoulder2.EndedDown); \
        DEBUG_VALUE("RightShoulder2", (Controller)->RightShoulder2.EndedDown); \
        DEBUG_VALUE("Select", (Controller)->Select.EndedDown);          \
        DEBUG_VALUE("Start", (Controller)->Start.EndedDown);            \
        DEBUG_VALUE("LeftClick", (Controller)->LeftClick.EndedDown);    \
        DEBUG_VALUE("RightClick", (Controller)->RightClick.EndedDown);  \
        DEBUG_VALUE("Power", (Controller)->Power.EndedDown);            \
        DEBUG_VALUE("CenterClick", (Controller)->CenterClick.EndedDown); \
    }


#else
#define LOG_CONTROLLER(...)
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
