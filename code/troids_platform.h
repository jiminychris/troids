#if !defined(TROIDS_PLATFORM_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include <stdint.h>
#include <float.h>

#define GAMMA_CORRECT 1
#define SAMPLE_COUNT 4

#define global_variable static
#define local_persist static
#define internal static
#define introspect

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float r32;
typedef double r64;

typedef int32_t b32;

typedef size_t memory_size;

#define U8_MIN 0
#define U8_MAX 0xFF

#define S32_MIN -32768
#define S32_MAX 32767

#define REAL32_MIN FLT_MIN
#define REAL32_MAX FLT_MAX

struct v2i
{
    s32 x;
    s32 y;
};

union v2
{
    struct
    {
        r32 x;
        r32 y;
    };
    struct
    {
        r32 u;
        r32 v;
    };
};

union v3
{
    struct
    {
        r32 x;
        r32 y;
        r32 z;
    };
    struct
    {
        r32 u;
        r32 v;
        r32 w;
    };
    struct
    {
        r32 r;
        r32 g;
        r32 b;
    };
    struct
    {
        v2 xy;
        r32 z;
    };
    struct
    {
        v2 uv;
        r32 w;
    };
};

union v4
{
    struct
    {
        r32 x;
        r32 y;
        r32 z;
        r32 w;
    };
    struct
    {
        r32 r;
        r32 g;
        r32 b;
        r32 a;
    };
    struct
    {
        v3 rgb;
        r32 a;
    };
    struct
    {
        v3 xyz;
        r32 w;
    };
    struct
    {
        v2 xy;
        r32 z;
        r32 w;
    };
};

union m22
{
    v2 Rows[2];
    r32 E[4];
};

union m33
{
    v3 Rows[3];
    r32 E[9];
};

struct rectangle2
{
    v2 Min;
    v2 Max;
};

struct rectangle2i
{
    v2i Min;
    v2i Max;
};

inline s32
Minimum(s32 A, s32 B)
{
    if(B < A)
    {
        A = B;
    }
    return(A);
}

inline u32
Maximum(u32 A, u32 B)
{
    if(B > A)
    {
        A = B;
    }
    return(A);
}

inline s32
Maximum(s32 A, s32 B)
{
    if(B > A)
    {
        A = B;
    }
    return(A);
}

inline r32
Minimum(r32 A, r32 B)
{
    if(B < A)
    {
        A = B;
    }
    return(A);
}

inline r32
Maximum(r32 A, r32 B)
{
    if(B > A)
    {
        A = B;
    }
    return(A);
}

inline r64
Minimum(r64 A, r64 B)
{
    if(B < A)
    {
        A = B;
    }
    return(A);
}

inline r64
Maximum(r64 A, r64 B)
{
    if(B > A)
    {
        A = B;
    }
    return(A);
}

inline s32
RoundS32(r32 A)
{
    s32 Result = (s32)(A + 0.5f);
    return(Result);
}

inline u32
RoundU32(r32 A)
{
    u32 Result = (u32)(A + 0.5f);
    return(Result);
}

inline s32
Clamp(s32 Min, s32 A, s32 Max)
{
    s32 Result = Maximum(Min, Minimum(Max, A));
    return(Result);
}

inline r32
Clamp(r32 Min, r32 A, r32 Max)
{
    r32 Result = Maximum(Min, Minimum(Max, A));
    return(Result);
}

inline r64
Clamp(r64 Min, r64 A, r64 Max)
{
    r64 Result = Maximum(Min, Minimum(Max, A));
    return(Result);
}

inline r32
Clamp01(r32 A)
{
    r32 Result = Clamp(0.0f, A, 1.0f);
    return(Result);
}

inline r64
Clamp01(r64 A)
{
    r64 Result = Clamp(0.0, A, 1.0);
    return(Result);
}

#if TROIDS_SLOW
#define Assert(Expr) {if(!(Expr)) int A = *((int *)0);}
#else
#define Assert(Expr)
#endif
#define InvalidCodePath Assert(!"Invalid code path");
#define InvalidDefaultCase default: Assert(!"Invalid default case"); break;
#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))
#define OffsetOf(type, Member) ((size_t)&(((type *)0)->Member))
#define NotImplemented Assert(!"Not implemented");

#define Kilobytes(Value) ((u64)(Value)*1024)
#define Megabytes(Value) (Kilobytes(Value)*1024)
#define Gigabytes(Value) (Megabytes(Value)*1024)
#define Terabytes(Value) (Gigabytes(Value)*1024)

#define PackedBufferAdvance(Name, type, Cursor) type *Name = (type *)Cursor; Cursor += sizeof(type);

struct read_file_result
{
    u64 ContentsSize;
    void *Contents;
};
#define PLATFORM_READ_FILE(Name) read_file_result Name(char *FileName)
typedef PLATFORM_READ_FILE(platform_read_file);

struct thread_progress
{
    b32 Finished;
};

#define THREAD_CALLBACK(Name) void Name(void *Params)
typedef THREAD_CALLBACK(thread_callback);

struct thread_work
{
    thread_callback *Callback;
    void *Params;
    thread_progress *Progress;
    b32 Free;
};

struct loaded_bitmap
{
    s32 Height;
    s32 Width;
    v2 Align;
    s32 Pitch;
    void *Memory;
};

struct render_chunk
{
    b32 Cleared;
    b32 Used;
    loaded_bitmap BackBuffer;
    loaded_bitmap CoverageBuffer;
    loaded_bitmap SampleBuffer;
    s32 OffsetX;
    s32 OffsetY;
};

// TODO(chris): Optimize this for the number of logical cores.
#define RENDER_CHUNK_COUNT 64
struct renderer_state
{
    loaded_bitmap BackBuffer;
    loaded_bitmap CoverageBuffer;
    loaded_bitmap SampleBuffer;

    render_chunk RenderChunks[RENDER_CHUNK_COUNT];
};

inline void
SplitWorkIntoSquares(render_chunk *RenderChunks, u32 CoreCount, s32 Width, s32 Height,
                     s32 OffsetX, s32 OffsetY)
{
    if(CoreCount == 1)
    {
        RenderChunks->OffsetX = OffsetX;
        RenderChunks->OffsetY = OffsetY;
        RenderChunks->BackBuffer.Height = Height;
        RenderChunks->BackBuffer.Width = Width;
        RenderChunks->CoverageBuffer.Height = RenderChunks->BackBuffer.Height;
        RenderChunks->CoverageBuffer.Width = RenderChunks->BackBuffer.Width;
        RenderChunks->SampleBuffer.Height = RenderChunks->BackBuffer.Height;
        RenderChunks->SampleBuffer.Width = SAMPLE_COUNT*RenderChunks->BackBuffer.Width;
    }
    else if(CoreCount & 1)
    {
        Assert(!"Odd core count not supported");
    }
    else
    {
        u32 HalfCores = CoreCount/2;
        if(Width >= Height)
        {
            s32 HalfWidth = Width/2;
            SplitWorkIntoSquares(RenderChunks, HalfCores, HalfWidth, Height, OffsetX, OffsetY);
            SplitWorkIntoSquares(RenderChunks+HalfCores, HalfCores, HalfWidth, Height, OffsetX+HalfWidth, OffsetY);
        }
        else
        {
            s32 HalfHeight = Height/2;
            SplitWorkIntoSquares(RenderChunks, HalfCores, Width, HalfHeight, OffsetX, OffsetY);
            SplitWorkIntoSquares(RenderChunks+HalfCores, HalfCores, Width, HalfHeight, OffsetX, OffsetY+HalfHeight);
        }
    }
}

inline void
SplitWorkIntoHorizontalStrips(render_chunk *RenderChunks, u32 CoreCount, s32 Width, s32 Height)
{
    s32 OffsetY = 0;
    r32 InverseCoreCount = 1.0f / CoreCount;
    for(u32 CoreIndex = 0;
        CoreIndex < CoreCount;
        ++CoreIndex)
    {
        render_chunk *RenderChunk = RenderChunks + CoreIndex;

        s32 NextOffsetY = RoundS32(Height*(CoreIndex+1)*InverseCoreCount);
        
        RenderChunk->OffsetX = 0;
        RenderChunk->OffsetY = OffsetY;
        RenderChunk->BackBuffer.Height = NextOffsetY-OffsetY;
        RenderChunk->BackBuffer.Width = Width;
        RenderChunks->CoverageBuffer.Height = RenderChunk->BackBuffer.Height;
        RenderChunks->CoverageBuffer.Width = RenderChunks->BackBuffer.Width;
        RenderChunks->SampleBuffer.Height = RenderChunk->BackBuffer.Height;
        RenderChunks->SampleBuffer.Width = SAMPLE_COUNT*RenderChunks->BackBuffer.Width;

        OffsetY = NextOffsetY;
    }
}

inline void
SplitWorkIntoVerticalStrips(render_chunk *RenderChunks, u32 CoreCount, s32 Width, s32 Height)
{
    s32 OffsetX = 0;
    r32 InverseCoreCount = 1.0f / CoreCount;
    for(u32 CoreIndex = 0;
        CoreIndex < CoreCount;
        ++CoreIndex)
    {
        render_chunk *RenderChunk = RenderChunks + CoreIndex;

        s32 NextOffsetX = RoundS32(Width*(CoreIndex+1)*InverseCoreCount);
        
        RenderChunk->OffsetX = OffsetX;
        RenderChunk->OffsetY = 0;
        RenderChunk->BackBuffer.Height = Height;
        RenderChunk->BackBuffer.Width = NextOffsetX-OffsetX;
        RenderChunks->CoverageBuffer.Height = RenderChunk->BackBuffer.Height;
        RenderChunks->CoverageBuffer.Width = RenderChunks->BackBuffer.Width;
        RenderChunks->SampleBuffer.Height = RenderChunk->BackBuffer.Height;
        RenderChunks->SampleBuffer.Width = SAMPLE_COUNT*RenderChunks->BackBuffer.Width;

        OffsetX = NextOffsetX;
    }
}

enum font_weight
{
    FontWeight_Normal,
    FontWeight_Bold,
};

struct loaded_font
{
    r32 Height;
    r32 Ascent;
    r32 LineAdvance;
    loaded_bitmap Glyphs[128];
    r32 KerningTable[128][128];
};

#define PLATFORM_PUSH_THREAD_WORK(Name) void Name(thread_callback *Callback, void *Params, thread_progress *Progress)
typedef PLATFORM_PUSH_THREAD_WORK(platform_push_thread_work);

struct game_memory
{
    u64 PermanentMemorySize;
    void *PermanentMemory;

    u64 TemporaryMemorySize;
    void *TemporaryMemory;

#if TROIDS_INTERNAL
    u64 DebugMemorySize;
    void *DebugMemory;
#endif

    platform_read_file *PlatformReadFile;
    platform_push_thread_work *PlatformPushThreadWork;
    loaded_font Font;
};

struct memory_arena
{
    memory_size Size;
    memory_size Used;
    void *Memory;
};

inline void
InitializeArena(memory_arena *Arena, memory_size SizeInit, void *MemoryInit)
{
    Arena->Size = SizeInit;
    Arena->Used = 0;
    Arena->Memory = MemoryInit;
}

inline b32 IsSet(u32 Flags, u32 Bit)
{
    b32 Result = (Flags & Bit);
    return(Result);
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

#define HasRoomForStruct(Arena, type, ...) HasRoom(Arena, sizeof(type), __VA_ARGS__)
#define HasRoomForArray(Arena, Count, type, ...) HasRoom(Arena, Count*sizeof(type), __VA_ARGS__)

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

#define PushStruct(Arena, type, ...) ((type *)PushSize(Arena, sizeof(type), __VA_ARGS__))
#define PushArray(Arena, Count, type, ...) ((type *)PushSize(Arena, Count*sizeof(type), __VA_ARGS__))

inline memory_arena
SubArena(memory_arena *Arena, memory_size Size)
{
    void *Memory = PushSize(Arena, Size);
    memory_arena Result;
    InitializeArena(&Result, Size, Memory);
    return(Result);
}

struct temporary_memory
{
    memory_size Used;
    memory_arena *Arena;
};

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

#pragma pack(push, 1)
struct bitmap_header
{
    u16 FileType;     /* File type, always 4D42h ("BM") */
    u32 FileSize;     /* Size of the file in bytes */
    u16 Reserved1;    /* Always 0 */
    u16 Reserved2;    /* Always 0 */
    u32 BitmapOffset; /* Starting position of image data in bytes */

    u32 Size;            /* Size of this header in bytes */
	s32 Width;           /* Image width in pixels */
	s32 Height;          /* Image height in pixels */
	u16 Planes;          /* Number of color planes */
	u16 BitsPerPixel;    /* Number of bits per pixel */
	u32 Compression;     /* Compression methods used */
	u32 SizeOfBitmap;    /* Size of bitmap in bytes */
	s32 HorzResolution;  /* Horizontal resolution in pixels per meter */
	s32 VertResolution;  /* Vertical resolution in pixels per meter */
	u32 ColorsUsed;      /* Number of colors in the image */
	u32 ColorsImportant; /* Minimum number of important colors */
    
    u32 RedMask;       /* Mask identifying bits of red component */
	u32 GreenMask;     /* Mask identifying bits of green component */
	u32 BlueMask;      /* Mask identifying bits of blue component */
	u32 AlphaMask;      /* Mask identifying bits of blue component */
};
#pragma pack(pop)

// TODO(chris): More polygons besides triangles?
struct obj_face
{
    u32 VertexIndexA;
    u32 TextureVertexIndexA;
    u32 VertexNormalIndexA;

    u32 VertexIndexB;
    u32 TextureVertexIndexB;
    u32 VertexNormalIndexB;

    u32 VertexIndexC;
    u32 TextureVertexIndexC;
    u32 VertexNormalIndexC;
};

// TODO(chris): Groups and smoothing?
struct loaded_obj
{
    u32 VertexCount;
    u32 TextureVertexCount;
    u32 VertexNormalCount;
    u32 FaceCount;

    v4 *Vertices;
    v3 *TextureVertices;
    v3 *VertexNormals;
    obj_face *Faces;
};

introspect struct game_button
{
    b32 EndedDown;
    u32 HalfTransitionCount;
};

struct game_controller
{
    v2 LeftStick;
    v2 RightStick;
    r32 LeftTrigger;
    r32 RightTrigger;

    r32 LowFrequencyMotor;
    r32 HighFrequencyMotor;

    union
    {
        game_button Buttons[14];
        struct
        {
            game_button ActionUp;
            game_button ActionLeft;
            game_button ActionDown;
            game_button ActionRight;
            game_button LeftShoulder1;
            game_button RightShoulder1;
            game_button LeftShoulder2;
            game_button RightShoulder2;
            game_button Select;
            game_button Start;
            game_button LeftClick;
            game_button RightClick;
            game_button Power;
            game_button CenterClick;
        };
    };
};

struct game_input
{
    r32 dtForFrame;

    v2 MousePosition;
    game_button LeftMouse;
    union
    {
        game_controller Controllers[5];
        struct
        {
            game_controller Keyboard;
            game_controller GamePads[4];
        };
    };
};

inline b32
WentDown(game_button Button)
{
    b32 Result = Button.EndedDown && (Button.HalfTransitionCount > 0);

    return(Result);
}

struct game_sound_buffer
{
    u32 SamplesPerSecond;
    u16 Channels;
    u16 BitsPerSample;
    u32 Size;
    u32 Region1Size;
    u32 Region2Size;
    void *Region1;
    void *Region2;
};

inline u32
CatStrings(u32 DestSize, char *DestInit,
           u32 SourceALength, char *SourceA,
           char *SourceB)
{
    char *Dest = DestInit;
    char *DestEnd = Dest + DestSize - 1;
    char *SourceAEnd = SourceA + SourceALength;
    
    while((Dest < DestEnd) && (SourceA < SourceAEnd))
    {
        *Dest++ = *SourceA++;
    }
    while((Dest < DestEnd) && *SourceB)
    {
        *Dest++ = *SourceB++;
    }
    *Dest = 0;

    u32 Result = (u32)(Dest - DestInit);
    return Result;
}

inline b32
StringsMatch(char *A, char *B)
{
    b32 Result = false;
    if(A && B)
    {
        Result = true;
        while(*A || *B)
        {
            if(*A++ != *B++)
            {
                Result = false;
                break;
            }
        } 
    }
    return Result;
}

#define GAME_UPDATE_AND_RENDER(Name) void Name(game_memory *GameMemory, game_input *Input, renderer_state *RendererState)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(Name) void Name(game_memory *GameMemory, game_input *Input, game_sound_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

#define DEBUG_COLLATE(Name) void Name(game_memory *GameMemory, game_input *Input, renderer_state *RendererState)
typedef DEBUG_COLLATE(debug_collate);

#define TROIDS_PLATFORM_H
#endif
