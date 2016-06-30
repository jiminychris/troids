#if !defined(TROIDS_PLATFORM_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include <stdint.h>

#define global_variable static
#define internal static

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

struct v2i
{
    s32 x;
    s32 y;
};

struct v2
{
    r32 x;
    r32 y;
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

#define Assert(Expr) {if(!(Expr)) int A = *((int *)0);}
#define InvalidDefaultCase default: Assert(!"Invalid default case"); break;
#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))
#define OffsetOf(type, Member) ((size_t)&(((type *)0)->Member))

#define Kilobytes(Value) ((u64)(Value)*1024)
#define Megabytes(Value) (Kilobytes(Value)*1024)
#define Gigabytes(Value) (Megabytes(Value)*1024)
#define Terabytes(Value) (Gigabytes(Value)*1024)

struct read_file_result
{
    u64 ContentsSize;
    void *Contents;
};
#define PLATFORM_READ_FILE(Name) read_file_result Name(char *FileName)
typedef PLATFORM_READ_FILE(platform_read_file);

struct game_memory
{
    u64 PermanentMemorySize;
    void *PermanentMemory;

    u64 TemporaryMemorySize;
    void *TemporaryMemory;

    platform_read_file *PlatformReadFile;
};

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

struct loaded_bitmap
{
    s32 Height;
    s32 Width;
    v2 Align;
    s32 Pitch;
    void *Memory;
};

struct game_button
{
    b32 EndedDown;
    u32 HalfTransitionCount;
};

struct game_controller
{
    r32 LeftStickX;
    r32 LeftStickY;
    r32 RightStickX;
    r32 RightStickY;
    r32 LeftTrigger;
    r32 RightTrigger;
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

    game_controller Keyboard;
    game_controller Controllers[4];
};

struct game_backbuffer
{
    s32 Width;
    s32 Height;
    s32 Pitch;
    void *Memory;
};

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

#define GAME_UPDATE_AND_RENDER(Name) void Name(game_memory *GameMemory, game_input *Input, game_backbuffer *BackBuffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(Name) void Name(game_memory *GameMemory, game_input *Input, game_sound_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

#define TROIDS_PLATFORM_H
#endif
