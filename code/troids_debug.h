#if !defined(TROIDS_DEBUG_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#define MAX_DEBUG_EVENTS 64

enum debug_event_type
{
    DebugEventType_CodeBegin,
    DebugEventType_CodeEnd,
};

struct debug_event
{
    u32 Line;
    debug_event_type Type;
    union
    {
        u64 Value_u64;
    };
    char *File;
    char *Name;
};

struct debug_state
{
    memory_arena Arena;
    u32 EventCount;
    debug_event Events[MAX_DEBUG_EVENTS];
};

global_variable debug_state *GlobalDebugState;

struct debug_timer
{
    debug_timer(char *FileInit, u32 LineInit, char *NameInit)
    {
        debug_event *Event = GlobalDebugState->Events + GlobalDebugState->EventCount++;
        Event->Type = DebugEventType_CodeBegin;
        Event->Value_u64 = rdtsc();
        Event->File = FileInit;
        Event->Line = LineInit;
        Event->Name = NameInit;
    }
    ~debug_timer(void)
    {
        debug_event *Event = GlobalDebugState->Events + GlobalDebugState->EventCount++;
        Event->Type = DebugEventType_CodeEnd;
        Event->Value_u64 = __rdtsc();
    }
};

#define TIMED_BLOCK__(Name, File, Line, Counter) debug_timer Timer_##Counter(File, Line, #Name)
#define TIMED_BLOCK_(Name, File, Line, Counter) TIMED_BLOCK__(Name, File, Line, Counter)
#define TIMED_BLOCK(Name) TIMED_BLOCK_(Name, __FILE__, __LINE__, __COUNTER__)

#define TROIDS_DEBUG_H
#endif
