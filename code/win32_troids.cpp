/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include <windows.h>
#include <stdio.h>
#include <stddef.h>

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

#include <dsound.h>
#define DIRECT_SOUND_CREATE(Name) HRESULT WINAPI Name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

#if 1
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
const GUID IID_IDirectInput8A = {3212410928, 18490, 19874, {170, 153, 93, 100, 237, 54, 151, 0}};
#define DIRECT_INPUT_CREATE(Name) HRESULT Name(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, LPUNKNOWN punkOuter);
typedef DIRECT_INPUT_CREATE(direct_input_create);
#else
#include <hidsdi.h>
#define GET_HID_GUID(Name) void Name(LPGUID HIDGUID);
typedef GET_HID_GUID(get_hid_guid);
#include <setupapi.h>
#define GET_CLASS_DEVS(Name) HDEVINFO Name(GUID *ClassGuid, PCTSTR Enumerator, HWND hwndParent, DWORD Flags);
typedef GET_CLASS_DEVS(get_class_devs);
#define ENUM_DEVICE_INTERFACES(Name) BOOL Name(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, GUID *InterfaceClassGuid, DWORD MemberIndex, PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData);
typedef ENUM_DEVICE_INTERFACES(enum_device_interfaces);
#define GET_DEVICE_INTERFACE_DETAIL(Name) BOOL Name(HDEVINFO DeviceInfoSet, PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData, PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData, DWORD DeviceInterfaceDetailDataSize, PDWORD RequiredSize, PSP_DEVINFO_DATA DeviceInfoData);
typedef GET_DEVICE_INTERFACE_DETAIL(get_device_interface_detail);
#endif

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

enum recording_state
{
    RecordingState_None,
    RecordingState_Recording,
    RecordingState_PlayingRecord,
};

int WinMain(HINSTANCE Instance,
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
    WindowClass.hInstance = Instance;
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
                                      Instance,
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
            GameState.RBase = 0.0f;
            GameState.GBase = 0.0f;
            GameState.BBase = 0.0f;
            GameState.RunningSampleCount = 0;
            
            // TODO(chris): Query monitor refresh rate
            r32 dtForFrame = 1.0f / 60.0f;
            
            game_backbuffer GameBackBuffer;
            GameBackBuffer.Width = GlobalBackBuffer.Width;
            GameBackBuffer.Height = GlobalBackBuffer.Height;
            GameBackBuffer.Pitch = GlobalBackBuffer.Pitch;
            GameBackBuffer.Memory = GlobalBackBuffer.Memory;

            LPDIRECTSOUND DirectSound;
            LPDIRECTSOUNDBUFFER PrimarySoundBuffer = 0;
            LPDIRECTSOUNDBUFFER SecondarySoundBuffer = 0;
            game_sound_buffer GameSoundBuffer = {};

            HMODULE DSoundCode = LoadLibrary("dsound.dll");
            direct_sound_create *DirectSoundCreate = 0;
            if(DSoundCode)
            {
                DirectSoundCreate =
                    (direct_sound_create *)GetProcAddress(DSoundCode, "DirectSoundCreate");
            }
            
            if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
            {
                if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
                {
                    // NOTE(chris): Primary and secondary buffer need the same format
                    WAVEFORMATEX WaveFormat = {};
                    WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
                    WaveFormat.nChannels = 2;
                    WaveFormat.nSamplesPerSec = 44100;
                    WaveFormat.wBitsPerSample = 16;
                    WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
                    WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
                    DSBUFFERDESC PrimaryBufferDescription = {};
                    PrimaryBufferDescription.dwSize = sizeof(PrimaryBufferDescription); 
                    PrimaryBufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                    if(SUCCEEDED(DirectSound->CreateSoundBuffer(&PrimaryBufferDescription, &PrimarySoundBuffer, 0)))
                    {
                        // NOTE(chris): Set the primary buffer format.
                        PrimarySoundBuffer->SetFormat(&WaveFormat);
                    }
                    else
                    {
                        // TODO(chris): Logging
                    }
                
                    DSBUFFERDESC SecondaryBufferDescription = {};
                    SecondaryBufferDescription.dwSize = sizeof(SecondaryBufferDescription); 
                    SecondaryBufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
                    // NOTE(chris): Secondary sound buffer is 2s long.
                    SecondaryBufferDescription.dwBufferBytes = WaveFormat.nAvgBytesPerSec*2;
                    SecondaryBufferDescription.lpwfxFormat = &WaveFormat;
                    if(SUCCEEDED(DirectSound->CreateSoundBuffer(&SecondaryBufferDescription, &SecondarySoundBuffer, 0)))
                    {
                        GameSoundBuffer.SamplesPerSecond = WaveFormat.nSamplesPerSec;
                        GameSoundBuffer.Channels = WaveFormat.nChannels;
                        GameSoundBuffer.BitsPerSample = WaveFormat.wBitsPerSample;
                        GameSoundBuffer.Size = SecondaryBufferDescription.dwBufferBytes;
                        Assert(SUCCEEDED(SecondarySoundBuffer->Play(0, 0, DSBPLAY_LOOPING)));
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

#if 0
            get_hid_guid *GetHIDGUID = 0;
            get_class_devs *GetClassDevs = 0;
            enum_device_interfaces *EnumDeviceInterfaces = 0;
            get_device_interface_detail *GetDeviceInterfaceDetail = 0;

            HMODULE HIDCode = LoadLibrary("hid.dll");
            HMODULE SetupCode = LoadLibrary("setupapi.dll");

            if(HIDCode)
            {
                GetHIDGUID = (get_hid_guid *)GetProcAddress(HIDCode, "HidD_GetHidGuid");
            }
            if(SetupCode)
            {
                GetClassDevs = (get_class_devs *)GetProcAddress(SetupCode, "SetupDiGetClassDevsA");
                EnumDeviceInterfaces = (enum_device_interfaces *)GetProcAddress(SetupCode, "SetupDiEnumDeviceInterfaces");
                GetDeviceInterfaceDetail = (get_device_interface_detail *)GetProcAddress(SetupCode, "SetupDiGetDeviceInterfaceDetailA");
            }
            if(GetHIDGUID && GetClassDevs && EnumDeviceInterfaces)
            {
                GUID HIDGUID;
                GetHIDGUID(&HIDGUID);
                HDEVINFO DeviceInfo = GetClassDevs(&HIDGUID, 0, 0, DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
                DWORD InterfaceIndex = 0;
                SP_DEVICE_INTERFACE_DATA DeviceInterfaceData = {};
                DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
                while(EnumDeviceInterfaces(DeviceInfo, 0, &HIDGUID,
                                           InterfaceIndex++, &DeviceInterfaceData))
                {
                    DWORD BufferSize;
                    GetDeviceInterfaceDetail(DeviceInfo, &DeviceInterfaceData, 0, 0, &BufferSize, 0);
                    DWORD TotalBufferSize = sizeof(u32) + BufferSize;
                    SP_DEVICE_INTERFACE_DETAIL_DATA *DeviceInterfaceDetailData =
                        (SP_DEVICE_INTERFACE_DETAIL_DATA *)VirtualAlloc(0, TotalBufferSize,
                                                                        MEM_COMMIT|MEM_RESERVE,
                                                                        PAGE_READWRITE);
                    DeviceInterfaceDetailData->cbSize = sizeof(*DeviceInterfaceDetailData);
                    if(GetDeviceInterfaceDetail(DeviceInfo, &DeviceInterfaceData,
                                                DeviceInterfaceDetailData,
                                                TotalBufferSize,
                                                0, 0))
                    {
                        HANDLE Device = CreateFile(DeviceInterfaceDetailData->DevicePath,
                                                   GENERIC_READ|GENERIC_WRITE,
                                                   FILE_SHARE_READ|FILE_SHARE_WRITE,
                                                   0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
                        int A = 0;
                    }
                }
            }
#else
            direct_input_create *DirectInputCreate = 0;
            HMODULE DInputCode = LoadLibrary("dinput8.dll");
            if(DInputCode)
            {
                DirectInputCreate = (direct_input_create *)GetProcAddress(DInputCode, "DirectInput8Create");
            }
            if(DirectInputCreate)
            {
                LPDIRECTINPUT8 DirectInput;
                HRESULT DInputCreateResult = DirectInputCreate(Instance, DIRECTINPUT_VERSION,
                                                               IID_IDirectInput8A,
                                                               (void **)&DirectInput, 0);
                int A = 0;

            }
#endif

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
            
            recording_state RecordingState = RecordingState_None;
            // TODO(chris): Memory-map file? What happens when game state gets huge?
            HANDLE RecordFile = CreateFile("record.trc",
                                           GENERIC_READ|GENERIC_WRITE,
                                           0, 0,
                                           CREATE_ALWAYS,
                                           FILE_ATTRIBUTE_NORMAL, 0);
            DWORD ByteToLock = 0;
            GlobalRunning = true;
            game_input GameInput[2] = {};
            game_input *OldInput = GameInput;
            game_input *NewInput = GameInput + 1;
            QueryPerformanceCounter(&LastCounter);
            while(GlobalRunning)
            {
                game_input *Temp = NewInput;
                NewInput = OldInput;
                OldInput = Temp;
                for(u32 ButtonIndex = 0;
                    ButtonIndex < ArrayCount(NewInput->Buttons);
                    ++ButtonIndex)
                {
                    game_button *NewButton = NewInput->Buttons + ButtonIndex;
                    game_button *OldButton = OldInput->Buttons + ButtonIndex;
                    NewButton->EndedDown = OldButton->EndedDown;
                }
                NewInput->dtForFrame = dtForFrame;
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
                                        // TODO(chris): Recording inputs and game state.
                                        // 1. Dump out all game state to file
                                        // 2. Record game input every frame to same file
                                        // File format will be
                                        // struct
                                        // {
                                        //     game_state State;
                                        //     game_input *GameInputs;
                                        // }
                                        // Loop around once game_input * reaches EOF
                                        switch(RecordingState)
                                        {
                                            case RecordingState_None:
                                            {
                                                RecordingState = RecordingState_Recording;
                                                if(RecordFile)
                                                {
                                                    DWORD BytesWritten;
                                                    OVERLAPPED Overlapped = {};
                                                    BOOL Result = WriteFile(RecordFile, &GameState,
                                                                            sizeof(GameState),
                                                                            &BytesWritten, &Overlapped);
                                                    Assert(Result && (BytesWritten == sizeof(GameState)));
                                                }
                                            } break;

                                            case RecordingState_Recording:
                                            {
                                                RecordingState = RecordingState_PlayingRecord;
                                                if(RecordFile)
                                                {
                                                    DWORD BytesRead;
                                                    OVERLAPPED Overlapped = {};
                                                    BOOL Result = ReadFile(RecordFile, &GameState,
                                                                           sizeof(GameState),
                                                                           &BytesRead, &Overlapped);
                                                    Assert(Result && (BytesRead == sizeof(GameState)));
                                                }
                                            } break;

                                            case RecordingState_PlayingRecord:
                                            {
                                                RecordingState = RecordingState_None;
                                            } break;
                                        }
                                    } break;

                                    // TODO(chris): Clean up
                                    case 'W':
                                    {
                                        ++NewInput->MoveUp.HalfTransitionCount;
                                        NewInput->MoveUp.EndedDown = true;
                                    } break;

                                    case 'A':
                                    {
                                        ++NewInput->MoveLeft.HalfTransitionCount;
                                        NewInput->MoveLeft.EndedDown = true;
                                    } break;

                                    case 'S':
                                    {
                                        ++NewInput->MoveDown.HalfTransitionCount;
                                        NewInput->MoveDown.EndedDown = true;
                                    } break;

                                    case 'D':
                                    {
                                        ++NewInput->MoveRight.HalfTransitionCount;
                                        NewInput->MoveRight.EndedDown = true;
                                    } break;
                                }
                            }
                        } break;
                        
                        case WM_KEYUP:
                        {
                            if((Message.lParam & (1 << 30)))
                            {
                                switch(Message.wParam)
                                {
                                    // TODO(chris): Clean up
                                    case 'W':
                                    {
                                        ++NewInput->MoveUp.HalfTransitionCount;
                                        NewInput->MoveUp.EndedDown = false;
                                    } break;

                                    case 'A':
                                    {
                                        ++NewInput->MoveLeft.HalfTransitionCount;
                                        NewInput->MoveLeft.EndedDown = false;
                                    } break;

                                    case 'S':
                                    {
                                        ++NewInput->MoveDown.HalfTransitionCount;
                                        NewInput->MoveDown.EndedDown = false;
                                    } break;

                                    case 'D':
                                    {
                                        ++NewInput->MoveRight.HalfTransitionCount;
                                        NewInput->MoveRight.EndedDown = false;
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

                switch(RecordingState)
                {
                    case RecordingState_Recording:
                    {
                        DWORD BytesWritten;
                        BOOL Result = WriteFile(RecordFile, NewInput,
                                                sizeof(*NewInput),
                                                &BytesWritten, 0);
                        Assert(Result && (BytesWritten == sizeof(*NewInput)));
                    } break;
                    
                    case RecordingState_PlayingRecord:
                    {
                        DWORD BytesRead;
                        BOOL Result = ReadFile(RecordFile, NewInput,
                                               sizeof(*NewInput),
                                               &BytesRead, 0);
                        if(Result && (BytesRead == 0))
                        {
                            OVERLAPPED Overlapped = {};
                            Result = ReadFile(RecordFile, &GameState,
                                              sizeof(GameState),
                                              &BytesRead, &Overlapped);
                            Assert(Result && (BytesRead == sizeof(GameState)));
                            Result = ReadFile(RecordFile, NewInput,
                                              sizeof(*NewInput),
                                              &BytesRead, 0);
                        }
                        Assert(Result && (BytesRead == sizeof(*NewInput)));
                    } break;
                }
                
                LARGE_INTEGER InputCounter;
                QueryPerformanceCounter(&InputCounter);
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
                        GameCode = LoadLibrary(CodePath);
                        if(GameCode)
                        {
                            CodeWriteTime = LastWriteTime;
                            GameUpdateAndRender =
                                (game_update_and_render *)GetProcAddress(GameCode, "GameUpdateAndRender");
                            GameGetSoundSamples =
                                (game_get_sound_samples *)GetProcAddress(GameCode, "GameGetSoundSamples");
                        }
                        else
                        {
                            GameUpdateAndRender = 0;
                            GameGetSoundSamples = 0;
                        }
                    }
                }

                if(GameUpdateAndRender)
                {
                    GameUpdateAndRender(&GameState, NewInput, &GameBackBuffer);
                }
                if(GameGetSoundSamples && SecondarySoundBuffer)
                {
                    DWORD PlayCursor;
                    DWORD WriteCursor;
                    SecondarySoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
                    DWORD BytesToWrite;
                    u32 SampleSize = GameSoundBuffer.Channels*GameSoundBuffer.BitsPerSample / 8;
                    if(PlayCursor > ByteToLock)
                    {
                        BytesToWrite = PlayCursor - ByteToLock;
                    }
                    else
                    {
                        BytesToWrite = GameSoundBuffer.Size - ByteToLock;
                        BytesToWrite += PlayCursor;
                    }
                    BytesToWrite = Min(8192, BytesToWrite);
                    Assert(SUCCEEDED(SecondarySoundBuffer->Lock(ByteToLock, BytesToWrite,
                                                                &GameSoundBuffer.Region1, (LPDWORD)&GameSoundBuffer.Region1Size,
                                                                &GameSoundBuffer.Region2, (LPDWORD)&GameSoundBuffer.Region2Size,
                                                                0)));
                    GameGetSoundSamples(&GameState, GameInput, &GameSoundBuffer);
                    Assert(SUCCEEDED(SecondarySoundBuffer->Unlock(GameSoundBuffer.Region1, GameSoundBuffer.Region1Size,
                                                                  GameSoundBuffer.Region2, GameSoundBuffer.Region2Size)));
                    ByteToLock += BytesToWrite;
                    if(ByteToLock >= GameSoundBuffer.Size)
                    {
                        ByteToLock -= GameSoundBuffer.Size;
                    }
                }

                u32 RecordIconMargin = 10;
                u32 RecordIconRadius = 10;
                switch(RecordingState)
                {
                    case RecordingState_Recording:
                    {
                        u32 RecordIconRadius = 10;
                        u8 *PixelRow = ((u8 *)GameBackBuffer.Memory +
                                        GameBackBuffer.Pitch*(GameBackBuffer.Height -
                                                              (RecordIconMargin + 1)));
                        for(u32 Y = RecordIconMargin;
                            Y < RecordIconMargin + RecordIconRadius*2;
                            ++Y)
                        {
                            u32 *Pixel = (u32 *)PixelRow + RecordIconMargin;
                            for(u32 X = RecordIconMargin;
                                X < RecordIconMargin + RecordIconRadius*2;
                                ++X)
                            {
                                *Pixel++ = 0xFFFF0000;
                            }
                            PixelRow -= GameBackBuffer.Pitch;
                        }
                    } break;

                    case RecordingState_PlayingRecord:
                    {
                        u8 *PixelRow = ((u8 *)GameBackBuffer.Memory +
                                        GameBackBuffer.Pitch*(GameBackBuffer.Height -
                                                              (RecordIconMargin + 1)));
                        for(u32 Y = RecordIconMargin;
                            Y < RecordIconMargin + RecordIconRadius;
                            ++Y)
                        {
                            u32 *Pixel = (u32 *)PixelRow + RecordIconMargin;
                            u32 Width = (Y - RecordIconMargin)*2;
                            for(u32 X = RecordIconMargin;
                                X < RecordIconMargin + Width;
                                ++X)
                            {
                                *Pixel++ = 0xFFFFFFFF;
                            }
                            PixelRow -= GameBackBuffer.Pitch;
                        }
                        for(u32 Y = RecordIconMargin + RecordIconRadius;
                            Y < RecordIconMargin + 2*RecordIconRadius;
                            ++Y)
                        {
                            u32 *Pixel = (u32 *)PixelRow + RecordIconMargin;
                            u32 Width = (RecordIconMargin + 2*RecordIconRadius - Y)*2;
                            for(u32 X = RecordIconMargin;
                                X < RecordIconMargin + Width;
                                ++X)
                            {
                                *Pixel++ = 0xFFFFFFFF;
                            }
                            PixelRow -= GameBackBuffer.Pitch;
                        }
                    } break;
                }

                HDC DeviceContext = GetDC(Window);
                CopyBackBufferToWindow(DeviceContext, &GlobalBackBuffer);
                ReleaseDC(Window, DeviceContext);
                
                QueryPerformanceCounter(&Counter);
                char Text[256];
                _snprintf_s(Text, sizeof(Text), "Input time: %fms\n",
                            (InputCounter.QuadPart - LastCounter.QuadPart)*ClocksToSeconds*1000);
                OutputDebugStringA(Text);
                _snprintf_s(Text, sizeof(Text), "Frame time: %fms\n",
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
