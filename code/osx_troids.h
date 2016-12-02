#if !defined(OSX_TROIDS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#define THREAD_QUEUE_SIZE 256
#define MAX_THREAD_COUNT 256

#include <semaphore.h>
#include <stdio.h>
#import <Cocoa/Cocoa.h>

#include "troids_platform.h"
#include "troids_memory.h"
#include "troids_intrinsics.h"
#include "troids_debug.h"
#include "troids_opengl.h"

#include "troids.cpp"

#if 0
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
#endif

enum {
    VK_A                    = 0x00,
    VK_S                    = 0x01,
    VK_D                    = 0x02,
    VK_F                    = 0x03,
    VK_H                    = 0x04,
    VK_G                    = 0x05,
    VK_Z                    = 0x06,
    VK_X                    = 0x07,
    VK_C                    = 0x08,
    VK_V                    = 0x09,
    VK_B                    = 0x0B,
    VK_Q                    = 0x0C,
    VK_W                    = 0x0D,
    VK_E                    = 0x0E,
    VK_R                    = 0x0F,
    VK_Y                    = 0x10,
    VK_T                    = 0x11,
    VK_1                    = 0x12,
    VK_2                    = 0x13,
    VK_3                    = 0x14,
    VK_4                    = 0x15,
    VK_6                    = 0x16,
    VK_5                    = 0x17,
    VK_EQUAL                = 0x18,
    VK_9                    = 0x19,
    VK_7                    = 0x1A,
    VK_MINUS                = 0x1B,
    VK_8                    = 0x1C,
    VK_0                    = 0x1D,
    VK_RIGHTBRACKET         = 0x1E,
    VK_O                    = 0x1F,
    VK_U                    = 0x20,
    VK_LEFTBRACKET          = 0x21,
    VK_I                    = 0x22,
    VK_P                    = 0x23,
    VK_L                    = 0x25,
    VK_J                    = 0x26,
    VK_QUOTE                = 0x27,
    VK_K                    = 0x28,
    VK_SEMICOLON            = 0x29,
    VK_BACKSLASH            = 0x2A,
    VK_COMMA                = 0x2B,
    VK_SLASH                = 0x2C,
    VK_N                    = 0x2D,
    VK_M                    = 0x2E,
    VK_PERIOD               = 0x2F,
    VK_GRAVE                = 0x32,
    VK_KEYPADDECIMAL        = 0x41,
    VK_KEYPADMULTIPLY       = 0x43,
    VK_KEYPADPLUS           = 0x45,
    VK_KEYPADCLEAR          = 0x47,
    VK_KEYPADDIVIDE         = 0x4B,
    VK_KEYPADENTER          = 0x4C,
    VK_KEYPADMINUS          = 0x4E,
    VK_KEYPADEQUALS         = 0x51,
    VK_KEYPAD0              = 0x52,
    VK_KEYPAD1              = 0x53,
    VK_KEYPAD2              = 0x54,
    VK_KEYPAD3              = 0x55,
    VK_KEYPAD4              = 0x56,
    VK_KEYPAD5              = 0x57,
    VK_KEYPAD6              = 0x58,
    VK_KEYPAD7              = 0x59,
    VK_KEYPAD8              = 0x5B,
    VK_KEYPAD9              = 0x5C
};

/* keycodes for keys that are independent of keyboard layout*/
enum {
    VK_RETURN                    = 0x24,
    VK_TAB                       = 0x30,
    VK_SPACE                     = 0x31,
    VK_DELETE                    = 0x33,
    VK_ESCAPE                    = 0x35,
    VK_COMMAND                   = 0x37,
    VK_SHIFT                     = 0x38,
    VK_CAPSLOCK                  = 0x39,
    VK_OPTION                    = 0x3A,
    VK_CONTROL                   = 0x3B,
    VK_RIGHTSHIFT                = 0x3C,
    VK_RIGHTOPTION               = 0x3D,
    VK_RIGHTCONTROL              = 0x3E,
    VK_FUNCTION                  = 0x3F,
    VK_F17                       = 0x40,
    VK_VOLUMEUP                  = 0x48,
    VK_VOLUMEDOWN                = 0x49,
    VK_MUTE                      = 0x4A,
    VK_F18                       = 0x4F,
    VK_F19                       = 0x50,
    VK_F20                       = 0x5A,
    VK_F5                        = 0x60,
    VK_F6                        = 0x61,
    VK_F7                        = 0x62,
    VK_F3                        = 0x63,
    VK_F8                        = 0x64,
    VK_F9                        = 0x65,
    VK_F11                       = 0x67,
    VK_F13                       = 0x69,
    VK_F16                       = 0x6A,
    VK_F14                       = 0x6B,
    VK_F10                       = 0x6D,
    VK_F12                       = 0x6F,
    VK_F15                       = 0x71,
    VK_HELP                      = 0x72,
    VK_HOME                      = 0x73,
    VK_PAGEUP                    = 0x74,
    VK_FORWARDDELETE             = 0x75,
    VK_F4                        = 0x76,
    VK_END                       = 0x77,
    VK_F2                        = 0x78,
    VK_PAGEDOWN                  = 0x79,
    VK_F1                        = 0x7A,
    VK_LEFT                      = 0x7B,
    VK_RIGHT                     = 0x7C,
    VK_DOWN                      = 0x7D,
    VK_UP                        = 0x7E
};

typedef struct osx_backbuffer
{
    s32 Width;
    s32 Height;
    s32 MonitorWidth;
    s32 MonitorHeight;
    s32 Pitch;
    void *Memory;
} osx_backbuffer;

typedef enum recording_state
{
    RecordingState_None,
    RecordingState_Recording,
    RecordingState_PlayingRecord,
} recording_state;

typedef struct thread_context
{
    b32 Active;
    pthread_t Thread;
} thread_context;

global_variable v2 GlobalMousePosition;
global_variable game_button GlobalLeftMouse;
global_variable osx_backbuffer GlobalBackBuffer;
global_variable b32 GlobalRunning;
global_variable thread_work GlobalThreadQueue[THREAD_QUEUE_SIZE];
global_variable u32 GlobalThreadCount;
global_variable thread_context GlobalThreadContext[MAX_THREAD_COUNT];
global_variable volatile u32 GlobalThreadQueueStartIndex = 0;
global_variable u32 GlobalThreadQueueNextIndex = 0;
global_variable sem_t *GlobalThreadQueueSemaphore;
global_variable NSRect PreviousWindowRect;
global_variable NSWindow *GlobalWindow;
//global_variable HANDLE GlobalEXEFile;

//#include "osx_troids_font.h"

#define OSX_TROIDS_H
#endif
