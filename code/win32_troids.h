#if !defined(WIN32_TROIDS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#define THREAD_QUEUE_SIZE 256
#define MAX_THREAD_COUNT 256

#include <windows.h>
#include <stdio.h>
#include <dbt.h>
#include <gl/gl.h>

#include "troids_platform.h"
#include "troids_math.h"
#include "troids_intrinsics.h"
#include "troids_debug.h"
#include "troids_opengl.h"

#if ONE_FILE
#include "troids.cpp"
#endif

typedef BOOL WINAPI wgl_swap_interval_ext(int);
    
#include <dsound.h>
#define DIRECT_SOUND_CREATE(Name) HRESULT WINAPI Name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

#define CREATE_GUID(Name, x, y, z, a, b, c, d, e, f, g, h) const GUID Name = {x, y, z, {a, b, c, d, e, f, g, h}}
#include "troids_input.h"
#if 1
#include "troids_dxinput.cpp"
#else
#include "troids_hid.cpp"
#endif

struct win32_backbuffer
{
    s32 Width;
    s32 Height;
    s32 MonitorWidth;
    s32 MonitorHeight;
    s32 Pitch;
    BITMAPINFO Info;
    void *Memory;
};

enum recording_state
{
    RecordingState_None,
    RecordingState_Recording,
    RecordingState_PlayingRecord,
};

struct thread_context
{
    b32 Active;
    DWORD ID;
};

global_variable v2 GlobalMousePosition;
global_variable game_button GlobalLeftMouse;
global_variable win32_backbuffer GlobalBackBuffer;
global_variable b32 GlobalRunning;
global_variable thread_work GlobalThreadQueue[THREAD_QUEUE_SIZE];
global_variable u32 GlobalThreadCount;
global_variable thread_context GlobalThreadContext[MAX_THREAD_COUNT];
global_variable volatile u32 GlobalThreadQueueStartIndex = 0;
global_variable u32 GlobalThreadQueueNextIndex = 0;
global_variable HANDLE GlobalThreadQueueSemaphore;
global_variable WINDOWPLACEMENT PreviousWindowPlacement = { sizeof(PreviousWindowPlacement) };
global_variable HWND GlobalWindow;
global_variable HANDLE GlobalEXEFile;

#include "win32_troids_font.h"

#define WIN32_TROIDS_H
#endif
