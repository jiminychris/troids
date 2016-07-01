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

// NOTE(chris): This has become a matter of reverse-engineering. It seems like this will only be good
// for input via DirectInput, as DirectInput can't handle output and the HID output is poorly
// documented, and it may even be illegal for me to do this without licensing it.
#define DUALSHOCK_RAW_HID 0

inline u32
Minimum(u32 A, u32 B)
{
    if(B < A)
    {
        A = B;
    }
    return(A);
}

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
#if DUALSHOCK_RAW_HID
#include <hidsdi.h>
#define GET_HID_GUID(Name) void Name(LPGUID HIDGUID);
typedef GET_HID_GUID(get_hid_guid);
#define GET_DEVICE_ATTRIBUTES(Name) BOOLEAN Name(HANDLE HidDeviceObject, PHIDD_ATTRIBUTES Attributes);
typedef GET_DEVICE_ATTRIBUTES(get_device_attributes);
#define GET_INPUT_REPORT(Name) BOOLEAN Name(HANDLE HidDeviceObject, PVOID ReportBuffer, ULONG ReportBufferLength);
typedef GET_INPUT_REPORT(get_input_report);
#define SET_OUTPUT_REPORT(Name) BOOLEAN Name(HANDLE HidDeviceObject, PVOID ReportBuffer, ULONG ReportBufferLength);
typedef SET_OUTPUT_REPORT(set_output_report);
#include <setupapi.h>
#define GET_CLASS_DEVS(Name) HDEVINFO Name(GUID *ClassGuid, PCTSTR Enumerator, HWND hwndParent, DWORD Flags);
typedef GET_CLASS_DEVS(get_class_devs);
#define ENUM_DEVICE_INTERFACES(Name) BOOL Name(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, GUID *InterfaceClassGuid, DWORD MemberIndex, PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData);
typedef ENUM_DEVICE_INTERFACES(enum_device_interfaces);
#define GET_DEVICE_INTERFACE_DETAIL(Name) BOOL Name(HDEVINFO DeviceInfoSet, PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData, PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData, DWORD DeviceInterfaceDetailDataSize, PDWORD RequiredSize, PSP_DEVINFO_DATA DeviceInfoData);
typedef GET_DEVICE_INTERFACE_DETAIL(get_device_interface_detail);
#else
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
CREATE_GUID(GUID_ConstantForce, 0x13541C20,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
#define DIRECT_INPUT_CREATE(Name) HRESULT Name(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, LPUNKNOWN punkOuter);
typedef DIRECT_INPUT_CREATE(direct_input_create);
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

#if DUALSHOCK_RAW_HID
struct dualshock_4_input
{
    u8 ReportID;
    u8 LeftStickX;
    u8 LeftStickY;
    u8 RightStickX;
    u8 RightStickY;
    u8 ActionAndDPad;
    u8 ShouldersOptAndShare;
    u8 CounterTouchAndPS;
    u8 LeftTrigger;
    u8 RightTrigger;
    u8 Padding[54];
};
#pragma pack(push, 1)
struct dualshock_4_output
{
    u8 TransactionAndReportType;
    u8 ProtocolCode;
    u8 Unknown0[2];
    u8 RumbleEnabled;
    u8 Unknown1[2];
    u8 RumbleRightWeak;
    u8 RumbleLeftStrong;
    u8 R;
    u8 G;
    u8 B;
    u8 Unknown2[63];
    u32 CRC;
};
#pragma pack(pop)
#else
struct dualshock_4_input
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
#endif

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

internal read_file_result
Win32ReadFile(char *FileName)
{
    read_file_result Result = {};
    
    HANDLE File = CreateFile(FileName,
                             GENERIC_READ,
                             0, 0,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, 0);
    if(INVALID_HANDLE_VALUE == File)
    {
        // TODO(chris): Logging
    }
    else
    {
        Result.ContentsSize = GetFileSize(File, 0);
        Result.Contents = VirtualAlloc(0, Result.ContentsSize,
                                       MEM_COMMIT|MEM_RESERVE,
                                       PAGE_READWRITE);
        DWORD BytesRead;
        BOOL ReadFileResult = ReadFile(File, Result.Contents,
                                       (u32)Result.ContentsSize,
                                       &BytesRead, 0);

        Assert(ReadFileResult);
    }

    return(Result);
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
                                      0,
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

            game_memory GameMemory;
            GameMemory.PermanentMemorySize = Megabytes(256);
            GameMemory.TemporaryMemorySize = Gigabytes(2);
            u64 TotalMemorySize = GameMemory.PermanentMemorySize + GameMemory.TemporaryMemorySize;
            GameMemory.PermanentMemory = VirtualAlloc(0, TotalMemorySize,
                                                      MEM_COMMIT|MEM_RESERVE,
                                                      PAGE_READWRITE);
            GameMemory.TemporaryMemory = (u8 *)GameMemory.PermanentMemory + GameMemory.PermanentMemorySize;

            GameMemory.PlatformReadFile = Win32ReadFile;
            
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
#if DUALSHOCK_RAW_HID
            HANDLE Dualshock4Controllers[4] =
            {
                INVALID_HANDLE_VALUE,
                INVALID_HANDLE_VALUE,
                INVALID_HANDLE_VALUE,
                INVALID_HANDLE_VALUE,
            };
            
            get_hid_guid *GetHIDGUID = 0;
            get_device_attributes *GetDeviceAttributes = 0;
            get_input_report *GetInputReport = 0;
            set_output_report *SetOutputReport = 0;
            
            get_class_devs *GetClassDevs = 0;
            enum_device_interfaces *EnumDeviceInterfaces = 0;
            get_device_interface_detail *GetDeviceInterfaceDetail = 0;

            HMODULE HIDCode = LoadLibrary("hid.dll");
            HMODULE SetupCode = LoadLibrary("setupapi.dll");

            if(HIDCode)
            {
                GetHIDGUID = (get_hid_guid *)GetProcAddress(HIDCode, "HidD_GetHidGuid");
                GetDeviceAttributes = (get_device_attributes *)GetProcAddress(HIDCode, "HidD_GetAttributes");
                GetInputReport = (get_input_report *)GetProcAddress(HIDCode, "HidD_GetInputReport");
                SetOutputReport = (set_output_report *)GetProcAddress(HIDCode, "HidD_SetOutputReport");
            }
            if(SetupCode)
            {
                GetClassDevs = (get_class_devs *)GetProcAddress(SetupCode, "SetupDiGetClassDevsA");
                EnumDeviceInterfaces = (enum_device_interfaces *)GetProcAddress(SetupCode, "SetupDiEnumDeviceInterfaces");
                GetDeviceInterfaceDetail = (get_device_interface_detail *)GetProcAddress(SetupCode, "SetupDiGetDeviceInterfaceDetailA");
            }

            if(GetHIDGUID && GetDeviceAttributes && GetClassDevs && EnumDeviceInterfaces &&
               GetDeviceInterfaceDetail)
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
#if 1
                    // NOTE(chris): This shouldn't be a problem. The biggest I saw was 103.
                    u8 Buffer[512];
                    DWORD TotalBufferSize = sizeof(Buffer);
                    SP_DEVICE_INTERFACE_DETAIL_DATA *DeviceInterfaceDetailData =
                        (SP_DEVICE_INTERFACE_DETAIL_DATA *)Buffer;
#else
                    // TODO(chris): If I end up using this path, I need to free this memory.
                    DWORD BufferSize;
                    GetDeviceInterfaceDetail(DeviceInfo, &DeviceInterfaceData, 0, 0, &BufferSize, 0);
                    DWORD TotalBufferSize = sizeof(u32) + BufferSize;
                    SP_DEVICE_INTERFACE_DETAIL_DATA *DeviceInterfaceDetailData =
                        (SP_DEVICE_INTERFACE_DETAIL_DATA *)VirtualAlloc(0, TotalBufferSize,
                                                                        MEM_COMMIT|MEM_RESERVE,
                                                                        PAGE_READWRITE);
#endif
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
                        if(Device != INVALID_HANDLE_VALUE)
                        {
                            HIDD_ATTRIBUTES Attributes;
                            Attributes.Size = sizeof(Attributes);
                            BOOL AttributesResult = GetDeviceAttributes(Device, &Attributes);
                            if(AttributesResult &&
                               Attributes.VendorID == 0x054C && Attributes.ProductID == 0x05C4)
                            {
                                Dualshock4Controllers[0] = Device;

                                dualshock_4_output Output = {};
#if 1
                                Output.TransactionAndReportType = 0xa2;
                                Output.ProtocolCode = 0x11;
                                Output.R = 0;
                                Output.G = 0;
                                Output.B = 0xFF;
#else
                                Output = {0xa2, 0x11, 0xc0, 0x20, 0xf0, 0x04, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x43,
                                          0x00, 0x4d, 0x85, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00};
#endif
                                Output.CRC = stbiw__crc32((u8 *)&Output, sizeof(Output) - 4);
                                DWORD BytesWritten;
                                BOOL OutputResult = WriteFile(Device, &Output, sizeof(Output),
                                                              &BytesWritten, 0);
                                if(OutputResult)
                                {
                                    int A = 0;
                                }
                            }
                            else
                            {
                                CloseHandle(Device);
                            }
                        }
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
                                    {{&GUID_XAxis, OffsetOf(dualshock_4_input, LeftStickX), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                     {&GUID_YAxis, OffsetOf(dualshock_4_input, LeftStickY), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                     {&GUID_ZAxis, OffsetOf(dualshock_4_input, RightStickX), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                     {&GUID_RxAxis, OffsetOf(dualshock_4_input, LeftTrigger), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                     {&GUID_RyAxis, OffsetOf(dualshock_4_input, RightTrigger), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                     {&GUID_RzAxis, OffsetOf(dualshock_4_input, RightStickY), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                     {0, OffsetOf(dualshock_4_input, Square), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(0), 0},
                                     {0, OffsetOf(dualshock_4_input, Cross), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(1), 0},
                                     {0, OffsetOf(dualshock_4_input, Circle), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(2), 0},
                                     {0, OffsetOf(dualshock_4_input, Triangle), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(3), 0},
                                     {0, OffsetOf(dualshock_4_input, L1), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(4), 0},
                                     {0, OffsetOf(dualshock_4_input, R1), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(5), 0},
                                     {0, OffsetOf(dualshock_4_input, L2), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(6), 0},
                                     {0, OffsetOf(dualshock_4_input, R2), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(7), 0},
                                     {0, OffsetOf(dualshock_4_input, Share), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(8), 0},
                                     {0, OffsetOf(dualshock_4_input, Options), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(9), 0},
                                     {0, OffsetOf(dualshock_4_input, L3), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(10), 0},
                                     {0, OffsetOf(dualshock_4_input, R3), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(11), 0},
                                     {0, OffsetOf(dualshock_4_input, PS), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(12), 0},
                                     {0, OffsetOf(dualshock_4_input, Touchpad), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(13), 0},
                                     {0, OffsetOf(dualshock_4_input, DPad), DIDFT_POV|DIDFT_ANYINSTANCE, 0},
                                    };
                                DIDATAFORMAT Dualshock4DataFormat;
                                Dualshock4DataFormat.dwSize = sizeof(DIDATAFORMAT);
                                Dualshock4DataFormat.dwObjSize = sizeof(DIOBJECTDATAFORMAT);
                                Dualshock4DataFormat.dwFlags = DIDF_ABSAXIS;
                                Dualshock4DataFormat.dwDataSize = sizeof(dualshock_4_input);
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

            TIMECAPS TimeCaps;
            timeGetDevCaps(&TimeCaps, sizeof(TimeCaps));
            if(TimeCaps.wPeriodMin != 1)
            {
                Assert(!"Couldn't fix timer granularity");
                // TODO(chris): Logging
            }
            if(TIMERR_NOCANDO == timeBeginPeriod(TimeCaps.wPeriodMin))
            {
                Assert(!"Couldn't fix timer granularity");
                // TODO(chris): Logging
            }

            r32 ClocksToSeconds = 0.0f;
            LARGE_INTEGER Freq, LastCounter, Counter;
            if(QueryPerformanceFrequency(&Freq))
            {
                ClocksToSeconds = 1.0f / Freq.QuadPart;
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
                                                        BOOL Result = WriteFile(RecordFile, &GameMemory,
                                                                                sizeof(GameMemory),
                                                                                &BytesWritten, &Overlapped);
                                                        Assert(Result && (BytesWritten == sizeof(GameMemory)));
                                                    }
                                                } break;

                                                case RecordingState_Recording:
                                                {
                                                    RecordingState = RecordingState_PlayingRecord;
                                                    if(RecordFile)
                                                    {
                                                        DWORD BytesRead;
                                                        OVERLAPPED Overlapped = {};
                                                        BOOL Result = ReadFile(RecordFile, &GameMemory,
                                                                               sizeof(GameMemory),
                                                                               &BytesRead, &Overlapped);
                                                        Assert(Result && (BytesRead == sizeof(GameMemory)));
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
#if DUALSHOCK_RAW_HID
                    HANDLE Dualshock4Controller = Dualshock4Controllers[ControllerIndex];
                    if(Dualshock4Controller != INVALID_HANDLE_VALUE && GetInputReport)
                    {
                        dualshock_4_input Dualshock4Reports[32];

#if 0
                        BOOL GetReportResult = GetInputReport(Dualshock4Controller,
                                                              Report, sizeof(Report));
#else
                        DWORD BytesRead;
                        BOOL GetReportResult = ReadFile(Dualshock4Controller,
                                                        &Dualshock4Reports, sizeof(Dualshock4Reports),
                                                        &BytesRead, 0);
#endif
                        if(GetReportResult)
                        {
                            // TODO(chris): Average all received inputs?
                            dualshock_4_input *Dualshock4Data = Dualshock4Reports + 0;
                            // NOTE(chris): Analog sticks
                            {
                                Dualshock4Data->LeftStickY = 0xFF - Dualshock4Data->LeftStickY;
                                Dualshock4Data->RightStickY = 0xFF - Dualshock4Data->RightStickY;
                                r32 Center = 0xFF / 2.0f;
                                s32 Deadzone = 4;
                        
                                Controller->LeftStickX = 0.0f;
                                if(Dualshock4Data->LeftStickX < (Center - Deadzone))
                                {
                                    Controller->LeftStickX = (r32)(-(Center - Deadzone) + Dualshock4Data->LeftStickX) / (r32)(Center - Deadzone);
                                }
                                else if(Dualshock4Data->LeftStickX > (Center + Deadzone))
                                {
                                    Controller->LeftStickX = (r32)(Dualshock4Data->LeftStickX - Center - Deadzone) / (r32)(Center - Deadzone);
                                }

                                Controller->LeftStickY = 0.0f;
                                if(Dualshock4Data->LeftStickY < (Center - Deadzone))
                                {
                                    Controller->LeftStickY = (r32)(-(Center - Deadzone) + Dualshock4Data->LeftStickY) / (r32)(Center - Deadzone);
                                }
                                else if(Dualshock4Data->LeftStickY > (Center + Deadzone))
                                {
                                    Controller->LeftStickY = (r32)(Dualshock4Data->LeftStickY - Center - Deadzone) / (r32)(Center - Deadzone);
                                }

                                Controller->RightStickX = 0.0f;
                                if(Dualshock4Data->RightStickX < (Center - Deadzone))
                                {
                                    Controller->RightStickX = (r32)(-(Center - Deadzone) + Dualshock4Data->RightStickX) / (r32)(Center - Deadzone);
                                }
                                else if(Dualshock4Data->RightStickX > (Center + Deadzone))
                                {
                                    Controller->RightStickX = (r32)(Dualshock4Data->RightStickX - Center - Deadzone) / (r32)(Center - Deadzone);
                                }

                                Controller->RightStickY = 0.0f;
                                if(Dualshock4Data->RightStickY < (Center - Deadzone))
                                {
                                    Controller->RightStickY = (r32)(-(Center - Deadzone) + Dualshock4Data->RightStickY) / (r32)(Center - Deadzone);
                                }
                                else if(Dualshock4Data->RightStickY > (Center + Deadzone))
                                {
                                    Controller->RightStickY = (r32)(Dualshock4Data->RightStickY - Center - Deadzone) / (r32)(Center - Deadzone);
                                }
                            }
                            
                            // NOTE(chris): Triggers
                            {
                                Controller->LeftTrigger = (r32)Dualshock4Data->LeftTrigger / 0xFF;
                                Controller->RightTrigger = (r32)Dualshock4Data->RightTrigger / 0xFF;
                            }
// TODO(chris): Left off here!!! Make these actually work
#if 0
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
#endif
                            
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
#else
                    LPDIRECTINPUTDEVICE8A Dualshock4Controller = Dualshock4Controllers[ControllerIndex];
                    if(Dualshock4Controller)
                    {
                        dualshock_4_input Dualshock4Data = {};
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
#endif
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
                            Result = ReadFile(RecordFile, &GameMemory,
                                              sizeof(GameMemory),
                                              &BytesRead, &Overlapped);
                            Assert(Result && (BytesRead == sizeof(GameMemory)));
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
                    GameUpdateAndRender(&GameMemory, NewInput, &GameBackBuffer);
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
                    BytesToWrite = Minimum(8192, BytesToWrite);
                    Assert(SUCCEEDED(SecondarySoundBuffer->Lock(ByteToLock, BytesToWrite,
                                                                &GameSoundBuffer.Region1, (LPDWORD)&GameSoundBuffer.Region1Size,
                                                                &GameSoundBuffer.Region2, (LPDWORD)&GameSoundBuffer.Region2Size,
                                                                0)));
                    GameGetSoundSamples(&GameMemory, GameInput, &GameSoundBuffer);
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

                char Text[256];

                QueryPerformanceCounter(&Counter);
                r32 ElapsedSeconds = (Counter.QuadPart - LastCounter.QuadPart)*ClocksToSeconds;
                if(ElapsedSeconds < dtForFrame)
                {
                    s32 MSToSleep  = (u32)((dtForFrame - ElapsedSeconds)*1000.0f) - 1;
                    if(MSToSleep > 0)
                    {
                        Sleep(MSToSleep);
                    }
                    do
                    {
                        // TODO(chris): This can be way faster if I can get rdtsc to work.
                        QueryPerformanceCounter(&Counter);
                        ElapsedSeconds = (Counter.QuadPart - LastCounter.QuadPart)*ClocksToSeconds;
                    } while (ElapsedSeconds < dtForFrame);
                }
                else if (ElapsedSeconds > dtForFrame)
                {
                    OutputDebugStringA("Missed framerate!\n");
                }
                
                _snprintf_s(Text, sizeof(Text), "Frame time: %fms\n",
                            ElapsedSeconds*1000);
                OutputDebugStringA(Text);
                LastCounter = Counter;
            }
            timeEndPeriod(TimeCaps.wPeriodMin);
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
