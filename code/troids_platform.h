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

typedef size_t memory_size;

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

// TODO(chris): Understand and implement CRC32
static unsigned int stbiw__crc32(unsigned char *buffer, int len)
{
   static unsigned int crc_table[256] =
   {
      0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
      0x0eDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
      0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
      0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
      0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
      0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
      0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
      0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
      0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
      0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
      0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
      0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
      0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
      0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
      0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
      0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
      0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
      0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
      0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
      0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
      0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
      0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
      0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
      0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
      0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
      0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
      0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
      0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
      0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
      0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
      0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
      0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
   };

   unsigned int crc = ~0u;
   int i;
   for (i=0; i < len; ++i)
      crc = (crc >> 8) ^ crc_table[buffer[i] ^ (crc & 0xff)];
   return ~crc;
}

#define Assert(Expr) {if(!(Expr)) int A = *((int *)0);}
#define InvalidCodePath Assert(!"Invalid code path");
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

    u64 DebugMemorySize;
    void *DebugMemory;

    platform_read_file *PlatformReadFile;
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

inline void *
PushSize(memory_arena *Arena, memory_size Size)
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
    }

    return(Result);
}

#define PushStruct(Arena, type) (type *)PushSize(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize(Arena, Count*sizeof(type))

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

inline b32
WentDown(game_button Button)
{
    b32 Result = Button.EndedDown && (Button.HalfTransitionCount > 0);

    return(Result);
}

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
