/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include <windows.h>
#include <stdio.h>
#include <dsound.h>

#include "troids_platform.h"

struct win32_backbuffer
{
    s32 Width;
    s32 Height;
    s32 Pitch;
    BITMAPINFO Info;
    void *Memory;
};

global_variable win32_backbuffer GlobalBackBuffer;
global_variable b32 GlobalRunning;

inline void
CopyBackBufferToWindow(HDC DeviceContext, win32_backbuffer *BackBuffer)
{
    StretchDIBits(DeviceContext,
                  0,
                  0,
                  GlobalBackBuffer.Width,
                  GlobalBackBuffer.Height,
                  0,
                  0,
                  GlobalBackBuffer.Width,
                  GlobalBackBuffer.Height,
                  GlobalBackBuffer.Memory,
                  &GlobalBackBuffer.Info,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}

LRESULT WindowProc(HWND Window,
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam)
{
    LRESULT Result = 0;
    switch(Message) 
    { 
        case WM_ACTIVATEAPP:
        {
        } break;

        case WM_CLOSE:
        case WM_QUIT:
        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT PaintStruct;
            HDC DeviceContext = BeginPaint(Window, &PaintStruct);
            CopyBackBufferToWindow(DeviceContext, &GlobalBackBuffer);
            EndPaint(Window, &PaintStruct);
        } break;

        case WM_MOUSEMOVE:
        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam); 
        } break;
    }
    return(Result);
}

int WinMain(HINSTANCE Handle,
            HINSTANCE PrevInstance,
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
        HWND Window = CreateWindowExA(0,
                                      WindowClass.lpszClassName,
                                      "TROIDS",
                                      WS_OVERLAPPEDWINDOW|WS_VISIBLE,
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
            GlobalBackBuffer.Width = WindowWidth;
            GlobalBackBuffer.Height = WindowHeight;
            GlobalBackBuffer.Pitch = GlobalBackBuffer.Width*sizeof(u32);
            GlobalBackBuffer.Info.bmiHeader.biSize = sizeof(GlobalBackBuffer.Info.bmiHeader);
            GlobalBackBuffer.Info.bmiHeader.biWidth = GlobalBackBuffer.Width;
            GlobalBackBuffer.Info.bmiHeader.biHeight = GlobalBackBuffer.Height;
            GlobalBackBuffer.Info.bmiHeader.biPlanes = 1;
            GlobalBackBuffer.Info.bmiHeader.biBitCount = 32;
            GlobalBackBuffer.Info.bmiHeader.biCompression = BI_RGB;
            GlobalBackBuffer.Info.bmiHeader.biSizeImage = 0;
            GlobalBackBuffer.Info.bmiHeader.biXPelsPerMeter = 1;
            GlobalBackBuffer.Info.bmiHeader.biYPelsPerMeter = 1;
            GlobalBackBuffer.Info.bmiHeader.biClrUsed = 0;
            GlobalBackBuffer.Info.bmiHeader.biClrImportant = 0;

            HDC DeviceContext = GetDC(Window);
            CreateDIBSection(DeviceContext, &GlobalBackBuffer.Info, DIB_RGB_COLORS,
                             &GlobalBackBuffer.Memory, 0, 0);

            game_state GameState;
            GameState.tSin = 0.0f;
            
            game_input GameInput;
            // TODO(chris): Query monitor refresh rate
            GameInput.dtForFrame = 1.0f / 60.0f;
            
            game_backbuffer GameBackBuffer;
            GameBackBuffer.Width = GlobalBackBuffer.Width;
            GameBackBuffer.Height = GlobalBackBuffer.Height;
            GameBackBuffer.Pitch = GlobalBackBuffer.Pitch;
            GameBackBuffer.Memory = GlobalBackBuffer.Memory;

            LPDIRECTSOUNDBUFFER SecondarySoundBuffer = 0;
            LPDIRECTSOUND DirectSound;
            WAVEFORMATEX WaveFormat;
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = 44100;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;
            DSBUFFERDESC BufferDescription;
            BufferDescription.dwSize = sizeof(BufferDescription); 
            BufferDescription.dwFlags = 0;
            // NOTE(chris): Secondary sound buffer is 10s long.
            BufferDescription.dwBufferBytes = WaveFormat.nAvgBytesPerSec*10;
            BufferDescription.lpwfxFormat = &WaveFormat;

            game_sound_buffer GameSoundBuffer;
            if(DS_OK == DirectSoundCreate(0, &DirectSound, 0))
            {
                if(DS_OK == DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))
                {
                    // TODO(chris): Fails with E_INVALIDARG :(
                    HRESULT Result = DirectSound->CreateSoundBuffer(&BufferDescription, &SecondarySoundBuffer, 0);
                    if(DS_OK == Result)
                    {
                        // TODO(chris): Primary buffer allocation?

                        GameSoundBuffer.SamplesPerSecond = WaveFormat.nSamplesPerSec;
                        GameSoundBuffer.Channels = WaveFormat.nChannels;
                        GameSoundBuffer.BitsPerSample = WaveFormat.wBitsPerSample;
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
            }
            else
            {
                // TODO(chris): Logging
            }

            game_update_and_render *GameUpdateAndRender = 0;
            game_get_sound_samples *GameGetSoundSamples = 0;
            // TODO(chris): Is MAX_PATH really correct?
            char CodePath[MAX_PATH];
            u32 CodePathLength = GetModuleFileName(0, CodePath, sizeof(CodePath));
            u64 CodeWriteTime = 0;
            HMODULE GameCode = 0;
            // TODO(chris): Check for truncated path
            if(CodePathLength)
            {
                char *Char;
                for(Char = CodePath + CodePathLength - 1;
                    Char > CodePath;
                    --Char)
                {
                    if(*(Char - 1) == '\\')
                    {
                        break;
                    }
                }

                char *DLLName = "troids.dll";
                while(*DLLName && Char < (CodePath + sizeof(CodePath) - 1))
                {
                    *Char++ = *DLLName++;
                }
                *Char = 0;
            }
            else
            {
                // TODO(chris): Logging
            }

            r64 ClocksToSeconds = 0.0;
            LARGE_INTEGER Freq, LastCounter, Counter;
            if(QueryPerformanceFrequency(&Freq))
            {
                ClocksToSeconds = 1.0 / Freq.QuadPart;
            }
            else
            {
                // TODO(chris): Logging
            }

            // TODO(chris): Allocate some memory for the game state.
            
            b32 Recording = false;
            GlobalRunning = true;
            QueryPerformanceCounter(&LastCounter);
            while(GlobalRunning)
            {
                while(PeekMessageA(&Message, Window, 0, 0, PM_REMOVE))
                {
                    switch(Message.message)
                    {
                        case WM_KEYDOWN:
                        {
                            if(!(Message.lParam & (1 << 30)))
                            {
                                switch(Message.wParam)
                                {
                                    case 'R':
                                    {
                                        Recording = !Recording;

                                        // TODO(chris): Recording inputs and game state.
                                    } break;
                                }
                            }
                        } break;

                        default:
                        {
                            TranslateMessage(&Message);
                            DispatchMessageA(&Message);
                        } break;
                    }
                }

                WIN32_FILE_ATTRIBUTE_DATA FileInfo;
                if(GetFileAttributesEx(CodePath, GetFileExInfoStandard, &FileInfo))
                {
                    u64 LastWriteTime = (((u64)FileInfo.ftLastWriteTime.dwHighDateTime << 32) |
                                         (FileInfo.ftLastWriteTime.dwLowDateTime << 0));
                    if(LastWriteTime > CodeWriteTime)
                    {
                        if(GameCode)
                        {
                            // TODO(chris): Is this necessary?
                            FreeLibrary(GameCode);
                        }
                        CodeWriteTime = LastWriteTime;
                        GameCode = LoadLibrary(CodePath);
                        Assert(GameCode);
                        GameUpdateAndRender =
                            (game_update_and_render *)GetProcAddress(GameCode, "GameUpdateAndRender");
                        GameGetSoundSamples =
                            (game_get_sound_samples *)GetProcAddress(GameCode, "GameGetSoundSamples");
                    }
                }

                if(GameUpdateAndRender)
                {
                    GameUpdateAndRender(&GameState, &GameInput, &GameBackBuffer);
                }
                if(GameGetSoundSamples && SecondarySoundBuffer)
                {
                    SecondarySoundBuffer->Lock(0, WaveFormat.nAvgBytesPerSec,
                                               &GameSoundBuffer.Memory1, (LPDWORD)&GameSoundBuffer.Memory1Size,
                                               &GameSoundBuffer.Memory2, (LPDWORD)&GameSoundBuffer.Memory2Size,
                                               DSBLOCK_FROMWRITECURSOR);
                    GameGetSoundSamples(&GameState, &GameInput, &GameSoundBuffer);
                    SecondarySoundBuffer->Unlock(&GameSoundBuffer.Memory1, GameSoundBuffer.Memory1Size,
                                                 &GameSoundBuffer.Memory2, GameSoundBuffer.Memory2Size);
                    SecondarySoundBuffer->Play(0, 0, 0);
                }

                if(Recording)
                {
                    u32 RecordDotMargin = 10;
                    u32 RecordDotRadius = 10;
                    u8 *PixelRow = ((u8 *)GameBackBuffer.Memory +
                                    GameBackBuffer.Pitch*(GameBackBuffer.Height -
                                                          (RecordDotMargin + 1)));
                    for(u32 Y = RecordDotMargin;
                        Y < RecordDotMargin + RecordDotRadius*2;
                        ++Y)
                    {
                        u32 *Pixel = (u32 *)PixelRow + RecordDotMargin;
                        for(u32 X = RecordDotMargin;
                            X < RecordDotMargin + RecordDotRadius*2;
                            ++X)
                        {
                            *Pixel++ = 0xFFFF0000;
                        }
                        PixelRow -= GameBackBuffer.Pitch;
                    }
                }

                HDC DeviceContext = GetDC(Window);
                CopyBackBufferToWindow(DeviceContext, &GlobalBackBuffer);
                ReleaseDC(Window, DeviceContext);
                
                QueryPerformanceCounter(&Counter);
                char Text[256];
                _snprintf_s(Text, sizeof(Text), "Update time: %fms\n",
                            (Counter.QuadPart - LastCounter.QuadPart)*ClocksToSeconds*1000);
                OutputDebugStringA(Text);
                LastCounter = Counter;
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
