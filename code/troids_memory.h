#if !defined(TROIDS_MEMORY_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

inline void
InitializeArena(memory_arena *Arena, memory_size SizeInit, void *MemoryInit)
{
    Arena->Size = SizeInit;
    Arena->Used = 0;
    Arena->Memory = MemoryInit;
}

enum push_flags
{
    None = 0,
    PushFlag_Zero = 1 << 0,
};

inline b32
HasRoom(memory_arena *Arena, memory_size Size, u32 Flags = 0)
{
    b32 Result = ((Arena->Used + Size) <= Arena->Size);
    return(Result);
}

#define HasRoomForStruct(Arena, type, ...) HasRoom(Arena, sizeof(type), ##__VA_ARGS__)
#define HasRoomForArray(Arena, Count, type, ...) HasRoom(Arena, Count*sizeof(type), ##__VA_ARGS__)

inline void *
PushSize(memory_arena *Arena, memory_size Size, u32 Flags = 0)
{
    void *Result = 0;
    if((Arena->Used + Size) > Arena->Size)
    {
        Assert(!"Allocated too much memory!");
    }
    else
    {
        Result = (u8 *)Arena->Memory + Arena->Used;
        Arena->Used += Size;
        if(IsSet(Flags, PushFlag_Zero))
        {
            u8 *End = (u8 *)Arena->Memory + Arena->Used;
            for(u8 *At = (u8 *)Result;
                At < End;
                ++At)
            {
                *At = 0;
            }
        }
    }

    return(Result);
}

#define PushStruct(Arena, type, ...) ((type *)PushSize(Arena, sizeof(type), ##__VA_ARGS__))
#define PushArray(Arena, Count, type, ...) ((type *)PushSize(Arena, Count*sizeof(type), ##__VA_ARGS__))

inline memory_arena
SubArena(memory_arena *Arena, memory_size Size)
{
    void *Memory = PushSize(Arena, Size);
    memory_arena Result;
    InitializeArena(&Result, Size, Memory);
    return(Result);
}

typedef struct temporary_memory
{
    memory_size Used;
    memory_arena *Arena;
} temporary_memory;

inline temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    temporary_memory Result;
    Result.Used = Arena->Used;
    Result.Arena = Arena;
    return(Result);
}

inline void
EndTemporaryMemory(temporary_memory Temp)
{
    Temp.Arena->Used = Temp.Used;
}

#define TROIDS_MEMORY_H
#endif
