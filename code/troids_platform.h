#if !defined(TROIDS_PLATFORM_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */
    
#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif
    
#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

// TODO(chris): Intrinsics for different platforms
#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#define snprintf _snprintf_s
#include <windows.h>
#include <intrin.h>
#define ZERO(type) {}
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#include <pthread.h>
#include <mach/mach_time.h>
#define ZERO(type) (const type){}
#endif
#endif

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

typedef union {
    __m128i v;
    u32 a[4];
} u__m128i;

#define U8_MIN 0
#define U8_MAX 0xFF

#define S32_MIN -32768
#define S32_MAX 32767

#define REAL32_MIN FLT_MIN
#define REAL32_MAX FLT_MAX

typedef struct v2i
{
    s32 x;
    s32 y;
} v2i;

typedef union v2
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
} v2;

typedef union v3
{
    struct
    {
        union
        {
            struct
            {
                r32 x;
                r32 y;
            };
            v2 xy;
        };
        r32 z;
    };
    struct
    {
        union
        {
            struct
            {
                r32 u;
                r32 v;
            };
            v2 uv;
        };
        r32 w;
    };
    struct
    {
        r32 r;
        r32 g;
        r32 b;
    };
} v3;

typedef union v4
{
    struct
    {
        union
        {
            struct
            {
                union
                {
                    struct
                    {
                        r32 x;
                        r32 y;
                    };
                    v2 xy;
                };
                r32 z;
            };
            v3 xyz;
        };
        r32 w;
    };
    struct
    {
        union
        {
            struct
            {
                r32 r;
                r32 g;
                r32 b;
            };
            v3 rgb;
        };
        r32 a;
    };
} v4;

inline v2
V2(r32 X, r32 Y)
{
    v2 Result;
    Result.x = X;
    Result.y = Y;
    return(Result);
}

inline v2
V2i(s32 X, s32 Y)
{
    v2 Result;
    Result.x = (r32)X;
    Result.y = (r32)Y;
    return(Result);
}

inline v3
V3(r32 X, r32 Y, r32 Z)
{
    v3 Result;
    Result.x = X;
    Result.y = Y;
    Result.z = Z;
    return(Result);
}

inline v3
V3i(s32 X, s32 Y, s32 Z)
{
    v3 Result;
    Result.x = (r32)X;
    Result.y = (r32)Y;
    Result.z = (r32)Z;
    return(Result);
}

inline v3
V3(v2 XY, r32 Z)
{
    v3 Result;
    Result.x = XY.x;
    Result.y = XY.y;
    Result.z = Z;
    return(Result);
}

inline v4
V4(r32 X, r32 Y, r32 Z, r32 W)
{
    v4 Result;
    Result.x = X;
    Result.y = Y;
    Result.z = Z;
    Result.w = W;
    return(Result);
}

inline v4
V4(v3 XYZ, r32 W)
{
    v4 Result;
    Result.x = XYZ.x;
    Result.y = XYZ.y;
    Result.z = XYZ.z;
    Result.w = W;
    return(Result);
}

inline v4
V4i(u32 X, u32 Y, u32 Z, u32 W)
{
    v4 Result;
    Result.x = (r32)X;
    Result.y = (r32)Y;
    Result.z = (r32)Z;
    Result.w = (r32)W;
    return(Result);
}

typedef union m22
{
    v2 Rows[2];
    r32 E[4];
} m22;

typedef union m33
{
    v3 Rows[3];
    r32 E[9];
} m33;

typedef struct rectangle2
{
    v2 Min;
    v2 Max;
} rectangle2;

typedef struct rectangle2i
{
    v2i Min;
    v2i Max;
} rectangle2i;

inline r32
RealMask(b32 Value)
{
    u32 IntResult = Value ? 0xFFFFFFFF : 0;
    r32 Result = *((r32 *)(&IntResult));
    return(Result);
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

typedef struct read_file_result
{
    u64 ContentsSize;
    void *Contents;
} read_file_result;
#define PLATFORM_READ_FILE(Name) read_file_result Name(char *FileName, u32 Offset)
typedef PLATFORM_READ_FILE(platform_read_file);

typedef struct thread_progress
{
    b32 Finished;
} thread_progress;

#define THREAD_CALLBACK(Name) void Name(void *Params)
typedef THREAD_CALLBACK(thread_callback);

typedef struct thread_work
{
    thread_callback *Callback;
    void *Params;
    thread_progress *Progress;
    b32 Free;
} thread_work;

typedef struct loaded_bitmap
{
    s32 Height;
    s32 Width;
    v2 Align;
    s32 Pitch;
    void *Memory;
} loaded_bitmap;

typedef enum font_weight
{
    FontWeight_Normal,
    FontWeight_Bold,
} font_weight;

struct bitmap_id
{
    u32 Value;
};

typedef struct loaded_font
{
    r32 Height;
    r32 Ascent;
    r32 LineAdvance;
    bitmap_id Glyphs[128];
    r32 KerningTable[128][128];
} loaded_font;

global_variable thread_progress GlobalNullProgress;

#define PLATFORM_PUSH_THREAD_WORK(Name) void Name(thread_callback *Callback, void *Params, thread_progress *Progress)
typedef PLATFORM_PUSH_THREAD_WORK(platform_push_thread_work);

#define PLATFORM_WAIT_FOR_ALL_THREAD_WORK(Name) void Name()
typedef PLATFORM_WAIT_FOR_ALL_THREAD_WORK(platform_wait_for_all_thread_work);

typedef struct game_memory
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
    platform_wait_for_all_thread_work *PlatformWaitForAllThreadWork;
    loaded_font Font;
    u32 AssetOffset;
    char *EXEFileName;
} game_memory;

typedef struct memory_arena
{
    memory_size Size;
    memory_size Used;
    void *Memory;
} memory_arena;

inline b32 IsSet(u32 Flags, u32 Bit)
{
    b32 Result = (Flags & Bit);
    return(Result);
}

#pragma pack(push, 1)
typedef struct bitmap_header
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
} bitmap_header;
#pragma pack(pop)

// TODO(chris): More polygons besides triangles?
typedef struct obj_face
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
} obj_face;

// TODO(chris): Groups and smoothing?
typedef struct loaded_obj
{
    u32 VertexCount;
    u32 TextureVertexCount;
    u32 VertexNormalCount;
    u32 FaceCount;

    v4 *Vertices;
    v3 *TextureVertices;
    v3 *VertexNormals;
    obj_face *Faces;
} loaded_obj;

introspect typedef struct game_button
{
    b32 EndedDown;
    u32 HalfTransitionCount;
} game_button;

typedef enum controller_type
{
    ControllerType_None,
    
    ControllerType_Keyboard,
    
    ControllerType_Dualshock4,
    ControllerType_XboxOne,
    ControllerType_Xbox360,
    ControllerType_N64,
} controller_type;

typedef struct game_controller
{
    controller_type Type;
    
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
} game_controller;

typedef struct game_input
{
    r32 dtForFrame;
    u64 SeedValue;
    b32 Quit;

    v2 MousePosition;
    game_button LeftMouse;
    u32 MostRecentlyUsedController;
    union
    {
        game_controller Controllers[5];
        struct
        {
            game_controller Keyboard;
            game_controller GamePads[4];
        };
    };
} game_input;

inline b32
WentDown(game_button Button)
{
    b32 Result = Button.EndedDown && (Button.HalfTransitionCount > 0);

    return(Result);
}

typedef struct game_sound_buffer
{
    u32 SamplesPerSecond;
    u16 Channels;
    u16 BitsPerSample;
    u32 Size;
    u32 Region1Size;
    u32 Region2Size;
    void *Region1;
    void *Region2;
} game_sound_buffer;

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

typedef struct render_chunk
{
    b32 Used;
    loaded_bitmap BackBuffer;
    loaded_bitmap CoverageBuffer;
    loaded_bitmap SampleBuffer;
    s32 OffsetX;
    s32 OffsetY;
    v3 ClearColor;
} render_chunk;

// TODO(chris): Optimize this for the number of logical cores.
#define MAX_RENDER_CHUNKS 64
typedef struct renderer_state
{
    v3 ClearColor;
    loaded_bitmap BackBuffer;
    loaded_bitmap CoverageBuffer;
    loaded_bitmap SampleBuffer;

    u32 RenderChunkCount;
    render_chunk RenderChunks[MAX_RENDER_CHUNKS];
}renderer_state;

#define GAME_UPDATE_AND_RENDER(Name) void Name(game_memory *GameMemory, game_input *Input, renderer_state *RendererState)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(Name) void Name(game_memory *GameMemory, game_input *Input, game_sound_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

#define DEBUG_COLLATE(Name) void Name(game_memory *GameMemory, game_input *Input, renderer_state *RendererState)
typedef DEBUG_COLLATE(debug_collate);

#define TROIDS_PLATFORM_H
#endif
