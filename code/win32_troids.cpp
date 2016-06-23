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
#include <guiddef.h>

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

#define CREATE_GUID(Name, x, y, z, a, b, c, d, e, f, g, h) const GUID Name = {x, y, z, {a, b, c, d, e, f, g, h}}
#if 1
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
CREATE_GUID(Dualshock4GUID, 0x05c4054c,0x0,0x0,0x0,0x0,0x50,0x49,0x44,0x56,0x49,0x44);
CREATE_GUID(IID_IDirectInput8A, 0xBF798030,0x483A,0x4DA2,0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00);
CREATE_GUID(GUID_Joystick, 0x6F1D2B70,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
CREATE_GUID(GUID_XAxis, 0xA36D02E0,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
CREATE_GUID(GUID_YAxis, 0xA36D02E1,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
CREATE_GUID(GUID_ZAxis, 0xA36D02E2,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
CREATE_GUID(GUID_RxAxis, 0xA36D02F4,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
CREATE_GUID(GUID_RyAxis, 0xA36D02F5,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
CREATE_GUID(GUID_RzAxis, 0xA36D02E3,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
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

struct dualshock_4_data
{
    s32 LeftStickX;
    s32 LeftStickY;
    s32 RightStickX;
    s32 RightStickY;
    s32 LeftTrigger;
    s32 RightTrigger;
    u32 DPad;
    union
    {
        u8 Buttons[14];
        struct
        {
            u8 Square;
            u8 Cross;
            u8 Circle;
            u8 Triangle;
            u8 L1;
            u8 R1;
            u8 L2;
            u8 R2;
            u8 Share;
            u8 Options;
            u8 L3;
            u8 R3;
            u8 PS;
            u8 Touchpad;
        };
    };
    u8 Padding[2];
};

inline void
ProcessDualshock4Button(game_button *Button, u8 State)
{
    Button->EndedDown = State;
    Button->HalfTransitionCount = Button->EndedDown ? 1 : 0;
}

inline void
ProcessKeyboardMessage(game_button *Button)
{
    Button->EndedDown = !Button->EndedDown;
    ++Button->HalfTransitionCount;
}

inline void
ProcessKeyboardMessage(r32 *AnalogInput, b32 WentDown, r32 Value)
{
    if(!WentDown)
    {
        Value = -Value;
    }
    *AnalogInput += Value;
}

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

            // NOTE(chris): Maybe use this if we need to get battery info, set LED color?
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
            LPDIRECTINPUTDEVICE8A Dualshock4Controllers[4] = {};
            if(DInputCode)
            {
                DirectInputCreate = (direct_input_create *)GetProcAddress(DInputCode, "DirectInput8Create");
            }
            if(DirectInputCreate)
            {
                LPDIRECTINPUT8 DirectInput;
                if(SUCCEEDED(DirectInputCreate(Instance, DIRECTINPUT_VERSION,
                                               IID_IDirectInput8A,
                                               (void **)&DirectInput, 0)))
                {
                    // TODO(chris): Either poll or listen for device connection?
                    LPDIRECTINPUTDEVICE8A Device;
                    if(SUCCEEDED(DirectInput->CreateDevice(GUID_Joystick, &Device, 0)) &&
                       SUCCEEDED(Device->SetCooperativeLevel(Window, DISCL_BACKGROUND|DISCL_NONEXCLUSIVE)))
                    {
                        DIDEVICEINSTANCE DeviceInfo;
                        DeviceInfo.dwSize = sizeof(DeviceInfo);
                        if(SUCCEEDED(Device->GetDeviceInfo(&DeviceInfo)))
                        {
                            if(IsEqualGUID(Dualshock4GUID, DeviceInfo.guidProduct))
                            {
                                DIOBJECTDATAFORMAT Dualshock4ObjectDataFormat[] =
                                    {{&GUID_XAxis, OffsetOf(dualshock_4_data, LeftStickX), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                     {&GUID_YAxis, OffsetOf(dualshock_4_data, LeftStickY), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                     {&GUID_ZAxis, OffsetOf(dualshock_4_data, RightStickX), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                     {&GUID_RxAxis, OffsetOf(dualshock_4_data, LeftTrigger), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                     {&GUID_RyAxis, OffsetOf(dualshock_4_data, RightTrigger), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                     {&GUID_RzAxis, OffsetOf(dualshock_4_data, RightStickY), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                     {0, OffsetOf(dualshock_4_data, Square), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(0), 0},
                                     {0, OffsetOf(dualshock_4_data, Cross), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(1), 0},
                                     {0, OffsetOf(dualshock_4_data, Circle), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(2), 0},
                                     {0, OffsetOf(dualshock_4_data, Triangle), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(3), 0},
                                     {0, OffsetOf(dualshock_4_data, L1), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(4), 0},
                                     {0, OffsetOf(dualshock_4_data, R1), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(5), 0},
                                     {0, OffsetOf(dualshock_4_data, L2), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(6), 0},
                                     {0, OffsetOf(dualshock_4_data, R2), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(7), 0},
                                     {0, OffsetOf(dualshock_4_data, Share), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(8), 0},
                                     {0, OffsetOf(dualshock_4_data, Options), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(9), 0},
                                     {0, OffsetOf(dualshock_4_data, L3), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(10), 0},
                                     {0, OffsetOf(dualshock_4_data, R3), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(11), 0},
                                     {0, OffsetOf(dualshock_4_data, PS), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(12), 0},
                                     {0, OffsetOf(dualshock_4_data, Touchpad), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(13), 0},
                                     {0, OffsetOf(dualshock_4_data, DPad), DIDFT_POV|DIDFT_ANYINSTANCE, 0},
                                    };
                                DIDATAFORMAT Dualshock4DataFormat;
                                Dualshock4DataFormat.dwSize = sizeof(DIDATAFORMAT);
                                Dualshock4DataFormat.dwObjSize = sizeof(DIOBJECTDATAFORMAT);
                                Dualshock4DataFormat.dwFlags = DIDF_ABSAXIS;
                                Dualshock4DataFormat.dwDataSize = sizeof(dualshock_4_data);
                                Dualshock4DataFormat.dwNumObjs = ArrayCount(Dualshock4ObjectDataFormat);
                                Dualshock4DataFormat.rgodf = Dualshock4ObjectDataFormat;
                                HRESULT DataFormatResult = Device->SetDataFormat(&Dualshock4DataFormat);
                                if(SUCCEEDED(DataFormatResult))
                                {
                                    HRESULT Result = Device->Acquire();
                                    if(SUCCEEDED(Result))
                                    {
                                        Dualshock4Controllers[0] = Device;
                                    }
                                }
                            }
                        }
                    }
                }
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
            // TODO(chris): Maybe initialize this with starting controller state?
            game_input GameInput[3] = {};
            game_input *OldInput = GameInput;
            game_input *NewInput = GameInput + 1;
            QueryPerformanceCounter(&LastCounter);
            while(GlobalRunning)
            {
                game_input *Temp = NewInput;
                NewInput = OldInput;
                OldInput = Temp;
                *NewInput = {};
                NewInput->dtForFrame = dtForFrame;

                game_controller *NewKeyboard = &NewInput->Keyboard;
                game_controller *OldKeyboard = &OldInput->Keyboard;
                for(u32 ButtonIndex = 0;
                    ButtonIndex < ArrayCount(NewKeyboard->Buttons);
                    ++ButtonIndex)
                {
                    game_button *NewButton = NewKeyboard->Buttons + ButtonIndex;
                    game_button *OldButton = OldKeyboard->Buttons + ButtonIndex;
                    NewButton->EndedDown = OldButton->EndedDown;
                }
                NewKeyboard->LeftStickX = OldKeyboard->LeftStickX;
                NewKeyboard->LeftStickY = OldKeyboard->LeftStickY;
                NewKeyboard->RightStickX = OldKeyboard->RightStickX;
                NewKeyboard->RightStickY = OldKeyboard->RightStickY;

                while(PeekMessageA(&Message, Window, 0, 0, PM_REMOVE))
                {
                    switch(Message.message)
                    {
                        case WM_KEYDOWN:
                        case WM_KEYUP:
                        {
                            b32 AlreadyDown = (Message.lParam & (1 << 30));
                            b32 WentDown = !(Message.lParam & (1 << 31));
                            if(!(AlreadyDown && WentDown))
                            {
                                switch(Message.wParam)
                                {
                                    case VK_UP:
                                    {
                                        ProcessKeyboardMessage(&NewKeyboard->ActionUp);
                                    } break;

                                    case VK_LEFT:
                                    {
                                        ProcessKeyboardMessage(&NewKeyboard->ActionLeft);
                                    } break;

                                    case VK_DOWN:
                                    {
                                        ProcessKeyboardMessage(&NewKeyboard->ActionDown);
                                    } break;

                                    case VK_RIGHT:
                                    {
                                        ProcessKeyboardMessage(&NewKeyboard->ActionRight);
                                    } break;

                                    case 'W':
                                    {
                                        ProcessKeyboardMessage(&NewKeyboard->LeftStickY, WentDown, 1.0f);
                                    } break;

                                    case 'A':
                                    {
                                        ProcessKeyboardMessage(&NewKeyboard->LeftStickX, WentDown, -1.0f);
                                    } break;

                                    case 'S':
                                    {
                                        ProcessKeyboardMessage(&NewKeyboard->LeftStickY, WentDown, -1.0f);
                                    } break;

                                    case 'D':
                                    {
                                        ProcessKeyboardMessage(&NewKeyboard->LeftStickX, WentDown, 1.0f);
                                    } break;
                                }
                                if(WentDown)
                                {
                                    switch(Message.wParam)
                                    {
                                        case 'R':
                                        {
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
                                                    // TODO(chris): Do something to reset controller
                                                    // state so we don't have errant inputs.
                                                } break;
                                            }
                                        }
                                    }
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
                
                for(u32 ControllerIndex = 0;
                    ControllerIndex < ArrayCount(NewInput->Controllers);
                    ++ControllerIndex)
                {
                    game_controller *Controller = NewInput->Controllers + ControllerIndex;

                    LPDIRECTINPUTDEVICE8A Dualshock4Controller = Dualshock4Controllers[ControllerIndex];
                    if(Dualshock4Controller)
                    {
                        dualshock_4_data Dualshock4Data;
                        HRESULT DataFetchResult = Dualshock4Controller->GetDeviceState(sizeof(Dualshock4Data), &Dualshock4Data);
                        if(SUCCEEDED(DataFetchResult))
                        {
                            // NOTE(chris): Analog sticks
                            {
                                Dualshock4Data.LeftStickY = 0xFFFF - Dualshock4Data.LeftStickY;
                                Dualshock4Data.RightStickY = 0xFFFF - Dualshock4Data.RightStickY;
                                r32 Center = 0xFFFF / 2.0f;
                                s32 Deadzone = 1500;
                        
                                Controller->LeftStickX = 0.0f;
                                if(Dualshock4Data.LeftStickX < (Center - Deadzone))
                                {
                                    Controller->LeftStickX = (r32)(-(Center - Deadzone) + Dualshock4Data.LeftStickX) / (r32)(Center - Deadzone);
                                }
                                else if(Dualshock4Data.LeftStickX > (Center + Deadzone))
                                {
                                    Controller->LeftStickX = (r32)(Dualshock4Data.LeftStickX - Center - Deadzone) / (r32)(Center - Deadzone);
                                }

                                Controller->LeftStickY = 0.0f;
                                if(Dualshock4Data.LeftStickY < (Center - Deadzone))
                                {
                                    Controller->LeftStickY = (r32)(-(Center - Deadzone) + Dualshock4Data.LeftStickY) / (r32)(Center - Deadzone);
                                }
                                else if(Dualshock4Data.LeftStickY > (Center + Deadzone))
                                {
                                    Controller->LeftStickY = (r32)(Dualshock4Data.LeftStickY - Center - Deadzone) / (r32)(Center - Deadzone);
                                }

                                Controller->RightStickX = 0.0f;
                                if(Dualshock4Data.RightStickX < (Center - Deadzone))
                                {
                                    Controller->RightStickX = (r32)(-(Center - Deadzone) + Dualshock4Data.RightStickX) / (r32)(Center - Deadzone);
                                }
                                else if(Dualshock4Data.RightStickX > (Center + Deadzone))
                                {
                                    Controller->RightStickX = (r32)(Dualshock4Data.RightStickX - Center - Deadzone) / (r32)(Center - Deadzone);
                                }

                                Controller->RightStickY = 0.0f;
                                if(Dualshock4Data.RightStickY < (Center - Deadzone))
                                {
                                    Controller->RightStickY = (r32)(-(Center - Deadzone) + Dualshock4Data.RightStickY) / (r32)(Center - Deadzone);
                                }
                                else if(Dualshock4Data.RightStickY > (Center + Deadzone))
                                {
                                    Controller->RightStickY = (r32)(Dualshock4Data.RightStickY - Center - Deadzone) / (r32)(Center - Deadzone);
                                }
                            }

                            // NOTE(chris): Triggers
                            {
                                Controller->LeftTrigger = (r32)Dualshock4Data.LeftTrigger / 0xFFFF;
                                Controller->RightTrigger = (r32)Dualshock4Data.RightTrigger / 0xFFFF;
                            }

                            // NOTE(chris): Buttons
                            {
                                ProcessDualshock4Button(&Controller->ActionLeft, Dualshock4Data.Square);
                                ProcessDualshock4Button(&Controller->ActionDown, Dualshock4Data.Cross);
                                ProcessDualshock4Button(&Controller->ActionRight, Dualshock4Data.Circle);
                                ProcessDualshock4Button(&Controller->ActionUp, Dualshock4Data.Triangle);
                                ProcessDualshock4Button(&Controller->LeftShoulder1, Dualshock4Data.L1);
                                ProcessDualshock4Button(&Controller->RightShoulder1, Dualshock4Data.R1);
                                ProcessDualshock4Button(&Controller->LeftShoulder2, Dualshock4Data.L2);
                                ProcessDualshock4Button(&Controller->RightShoulder2, Dualshock4Data.R2);
                                ProcessDualshock4Button(&Controller->Select, Dualshock4Data.Share);
                                ProcessDualshock4Button(&Controller->Start, Dualshock4Data.Options);
                                ProcessDualshock4Button(&Controller->LeftClick, Dualshock4Data.L3);
                                ProcessDualshock4Button(&Controller->RightClick, Dualshock4Data.R3);
                                ProcessDualshock4Button(&Controller->Power, Dualshock4Data.PS);
                                ProcessDualshock4Button(&Controller->CenterClick, Dualshock4Data.Touchpad);
                            }

                            // NOTE(chris): DPad
                            {
                                switch(Dualshock4Data.DPad)
                                {
                                    case 0xFFFFFFFF:
                                    {
                                        // NOTE(chris): Centered. Don't interfere with analog sticks.
                                    } break;

                                    case 0:
                                    {
                                        Controller->LeftStickX = 0.0f;
                                        Controller->LeftStickY = 1.0f;
                                    } break;

                                    case 4500:
                                    {
                                        Controller->LeftStickX = 1.0f;
                                        Controller->LeftStickY = 1.0f;
                                    } break;

                                    case 9000:
                                    {
                                        Controller->LeftStickX = 1.0f;
                                        Controller->LeftStickY = 0.0f;
                                    } break;

                                    case 13500:
                                    {
                                        Controller->LeftStickX = 1.0f;
                                        Controller->LeftStickY = -1.0f;
                                    } break;

                                    case 18000:
                                    {
                                        Controller->LeftStickX = 0.0f;
                                        Controller->LeftStickY = -1.0f;
                                    } break;

                                    case 22500:
                                    {
                                        Controller->LeftStickX = -1.0f;
                                        Controller->LeftStickY = -1.0f;
                                    } break;

                                    case 27000:
                                    {
                                        Controller->LeftStickX = -1.0f;
                                        Controller->LeftStickY = 0.0f;
                                    } break;

                                    case 31500:
                                    {
                                        Controller->LeftStickX = -1.0f;
                                        Controller->LeftStickY = 1.0f;
                                    } break;

                                    InvalidDefaultCase;
                                }
                            }

                            Assert(Controller->LeftStickX >= -1.0f);
                            Assert(Controller->LeftStickX <= 1.0f);
                            Assert(Controller->LeftStickY >= -1.0f);
                            Assert(Controller->LeftStickY <= 1.0f);
                            Assert(Controller->RightStickX >= -1.0f);
                            Assert(Controller->RightStickX <= 1.0f);
                            Assert(Controller->RightStickY >= -1.0f);
                            Assert(Controller->RightStickY <= 1.0f);
                            Assert(Controller->LeftTrigger >= 0.0f);
                            Assert(Controller->LeftTrigger <= 1.0f);
                            Assert(Controller->RightTrigger >= 0.0f);
                            Assert(Controller->RightTrigger <= 1.0f);
                        }
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