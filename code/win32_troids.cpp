/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include <windows.h>
#include <stdint.h>

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

typedef uint32_t b32;

LRESULT WindowProc(HWND   Window,
                   UINT   Message,
                   WPARAM wParam,
                   LPARAM lParam)
{
    LRESULT Result;
    switch(Message) 
    { 
        default:
        {
            Result = DefWindowProcA(Window, Message, wParam, lParam); 
        } break;
    }
    return(Result);
}

int WinMain(HINSTANCE Handle,
            HINSTANCE PreviousInstance,
            LPSTR     CommandLine,
            int       Show)
{
    // TODO(chris): CreateMutex?
    
    MSG Message;
    Message.wParam = 0;
    WNDCLASS WindowClass;

    WindowClass.style = CS_OWNDC;
    WindowClass.lpfnWndProc = WindowProc;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = Handle;
    WindowClass.hIcon = 0;
    WindowClass.hCursor = 0;
    WindowClass.hbrBackground = 0;
    WindowClass.lpszMenuName = 0;
    WindowClass.lpszClassName = "TroidsWindowClass";

    if(RegisterClassA(&WindowClass))
    {
        s32 WindowWidth = 1920/2;
        s32 WindowHeight = 1080/2;
        HWND Window = CreateWindowA("TroidsWindowClass",
                                    "TROIDS",
                                    WS_OVERLAPPEDWINDOW,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    WindowWidth,
                                    WindowHeight,
                                    0,
                                    0,
                                    Handle,
                                    0);
        if(Window)
        {
            ShowWindow(Window, Show);
            UpdateWindow(Window);
            b32 Running = true;
            while(Running)
            {
                while(PeekMessageA(&Message, Window, 0, 0, PM_REMOVE))
                {
                    switch(Message.message)
                    {
                        case WM_KEYDOWN:
                        {
                            Running = false;
                        } break;

                        default:
                        {
                            TranslateMessage(&Message);
                            DispatchMessageA(&Message);
                        }
                    }
                }
            }
        }
        else
        {
            // TODO(chris): Logging
        }
    }
    else
    {
        // TODO(chris): Logging
    }
    return((int)Message.wParam);
}
