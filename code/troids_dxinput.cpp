/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include "troids_input.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <xinput.h>
CREATE_GUID(Dualshock4GUID, 0x05c4054c,0x0000,0x0000,0x00,0x00,0x50,0x49,0x44,0x56,0x49,0x44);
CREATE_GUID(XboxOneGUID, 0x02D1045E,0x0000,0x0000,0x00,0x00,0x50,0x49,0x44,0x56,0x49,0x44);
CREATE_GUID(Xbox360GUID, 0x02A1045E,0x0000,0x0000,0x00,0x00,0x50,0x49,0x44,0x56,0x49,0x44);
CREATE_GUID(N64GUID, 0x00060079,0x0000,0x0000,0x00,0x00,0x50,0x49,0x44,0x56,0x49,0x44);
CREATE_GUID(IID_IDirectInput8A, 0xBF798030,0x483A,0x4DA2,0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00);
CREATE_GUID(GUID_Joystick, 0x6F1D2B70,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
CREATE_GUID(GUID_XAxis, 0xA36D02E0,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
CREATE_GUID(GUID_YAxis, 0xA36D02E1,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
CREATE_GUID(GUID_ZAxis, 0xA36D02E2,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
CREATE_GUID(GUID_RxAxis, 0xA36D02F4,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
CREATE_GUID(GUID_RyAxis, 0xA36D02F5,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
CREATE_GUID(GUID_RzAxis, 0xA36D02E3,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
CREATE_GUID(GUID_ConstantForce, 0x13541C20,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
#define DIRECT_INPUT_CREATE(Name) HRESULT Name(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, LPUNKNOWN punkOuter)
typedef DIRECT_INPUT_CREATE(direct_input_create);
#define XINPUT_GET_STATE(Name) DWORD Name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef XINPUT_GET_STATE(xinput_get_state);
#define XINPUT_SET_STATE(Name) DWORD Name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef XINPUT_SET_STATE(xinput_set_state);
    
#define XBOX_GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define XBOX_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#define XBOX_GAMEPAD_TRIGGER_THRESHOLD    30

// TODO(chris): Figure out a way to support arbitrary generic controllers.

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

struct xbox_input
{
    s32 LeftStickX;
    s32 LeftStickY;
    s32 RightStickX;
    s32 RightStickY;
    s32 Triggers;
    u32 DPad;
    union
    {
        u8 Buttons[10];
        struct
        {
            u8 A;
            u8 B;
            u8 X;
            u8 Y;
            u8 LB;
            u8 RB;
            union
            {
                u8 Back;
                u8 View;
            };
            union
            {
                u8 Start;
                u8 Menu;
            };
            u8 LS;
            u8 RS;
        };
    };
    u8 Padding[2];
};

struct n64_input
{
    s32 StickX;
    s32 StickY;
    u32 DPad;
    union
    {
        u8 Buttons[10];
        struct
        {
            u8 CUp;
            u8 CRight;
            u8 CDown;
            u8 CLeft;
            u8 L;
            u8 R;
            u8 A;
            u8 Z;
            u8 B;
            u8 Start;
        };
    };
    u8 Padding[2];
};

struct input_device
{
    u32 Index;
    controller_type Type;
    GUID GUID;
    u32 XInputIndex;
    LPDIRECTINPUTDEVICE8A Device;
    
    input_device *Next;
};

#define INVALID_XINPUT_INDEX 0xFFFF
// NOTE(chris): Only call this function when type is XboxOne or Xbox360
inline b32 IsXInputGamePad(input_device *Device)
{
    b32 Result = (Device->XInputIndex != INVALID_XINPUT_INDEX);
    return(Result);
}

global_variable input_device GlobalGamePads_[MAX_GAMEPADS] = {};
global_variable input_device GlobalActiveGamePadsSentinel;
global_variable input_device *GlobalFreeGamePads = 0;
global_variable direct_input_create *DirectInputCreate = 0;
global_variable xinput_get_state *XInputGetState_ = 0;
global_variable xinput_set_state *XInputSetState_ = 0;
global_variable LPDIRECTINPUT8 GlobalDirectInput = 0;

inline XINPUT_GET_STATE(XInputGetState){return XInputGetState_(dwUserIndex, pState);}
inline XINPUT_SET_STATE(XInputSetState){return XInputSetState_(dwUserIndex, pVibration);}

internal void
InitializeInput(HINSTANCE Instance)
{
    GlobalActiveGamePadsSentinel.Next = &GlobalActiveGamePadsSentinel;
    for(s32 GamePadIndex = ArrayCount(GlobalGamePads_) - 1;
        GamePadIndex >= 0;
        --GamePadIndex)
    {
        input_device *GamePad = GlobalGamePads_ + GamePadIndex;
        GamePad->Index = GamePadIndex;
        GamePad->Next = GlobalFreeGamePads;
        GlobalFreeGamePads = GamePad;
    }
    
    HMODULE DInputCode = LoadLibrary("dinput8.dll");
    if(DInputCode)
    {
        DirectInputCreate = (direct_input_create *)GetProcAddress(DInputCode, "DirectInput8Create");
        if(DirectInputCreate)
        {
            if(SUCCEEDED(DirectInputCreate(Instance, DIRECTINPUT_VERSION,
                                           IID_IDirectInput8A,
                                           (void **)&GlobalDirectInput, 0)))
            {
            }
            else
            {
                // TODO(chris): Diagnostic
            }
        }
        else
        {
            // TODO(chris): Diagnostic
        }
    }
    else
    {
        // TODO(chris): Diagnostic
    }
    HMODULE XInputCode = LoadLibrary("xinput1_4.dll");
    if(!XInputCode)
    {
        HMODULE XInputCode = LoadLibrary("xinput1_3.dll");
    }

    if(XInputCode)
    {
        XInputGetState_ = (xinput_get_state *)GetProcAddress(XInputCode, "XInputGetState");
        if(XInputGetState_)
        {
        }
        else
        {
            // TODO(chris): Diagnostic
        }
        XInputSetState_ = (xinput_set_state *)GetProcAddress(XInputCode, "XInputSetState");
        if(XInputSetState_)
        {
        }
        else
        {
            // TODO(chris): Diagnostic
        }
    }
    else
    {
        // TODO(chris): Diagnostic
    }
}

struct device_guids
{
    u32 Count;
    // NOTE(chris): Enumerate as many as I can, handle actually latching them elsewhere.
    GUID GUIDs[16];
};

internal BOOL
EnumDevicesCallback(LPCDIDEVICEINSTANCE EnumeratedDevice, LPVOID Data)
{
    device_guids *GamePadGUIDs = (device_guids *)Data;
    // NOTE(chris): The end condition should catch this. First time through this should never trigger.
    Assert(GamePadGUIDs->Count < ArrayCount(GamePadGUIDs->GUIDs));

    GamePadGUIDs->GUIDs[GamePadGUIDs->Count++] = EnumeratedDevice->guidInstance;

    BOOL Result = ((GamePadGUIDs->Count < ArrayCount(GamePadGUIDs->GUIDs)) ?
                   DIENUM_CONTINUE : DIENUM_STOP);
    return(Result);
}

internal void
UnlatchGamePads()
{
    if(GlobalDirectInput)
    {
        device_guids GamePadGUIDs = {};
        if(SUCCEEDED(GlobalDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL,
                                                    EnumDevicesCallback,
                                                    &GamePadGUIDs,
                                                    DIEDFL_ALLDEVICES)))
        {
            for(input_device **ActiveGamePadPtr = &GlobalActiveGamePadsSentinel.Next;
                *ActiveGamePadPtr != &GlobalActiveGamePadsSentinel;
                )
            {
                input_device *ActiveGamePad = *ActiveGamePadPtr;
                Assert(ActiveGamePad->Type != ControllerType_None);
                b32 StillAttached = false;
                for(u32 DeviceGUIDIndex = 0;
                    DeviceGUIDIndex < GamePadGUIDs.Count;
                    ++DeviceGUIDIndex)
                {
                    GUID *DeviceGUID = GamePadGUIDs.GUIDs + DeviceGUIDIndex;
                    if(IsEqualGUID(*DeviceGUID, ActiveGamePad->GUID))
                    {
                        StillAttached = true;
                    }
                }
                if(!StillAttached)
                {
                    *ActiveGamePadPtr = ActiveGamePad->Next;

                    input_device **NextPtr = &GlobalFreeGamePads;
                    input_device *Next = *NextPtr;

                    // NOTE(chris): New controllers should latch to lowest open index.
                    while(Next && Next->Index < ActiveGamePad->Index)
                    {
                        NextPtr = &Next->Next;
                        Next = *NextPtr;
                    }
                    ActiveGamePad->Next = Next;
                    *NextPtr = ActiveGamePad;
                }
                else
                {
                    ActiveGamePadPtr = &ActiveGamePad->Next;
                }
            }
        }
    }
}

internal void
LatchGamePads(HWND Window)
{
    if(GlobalDirectInput)
    {
        device_guids GamePadGUIDs = {};
        if(SUCCEEDED(GlobalDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL,
                                                    EnumDevicesCallback,
                                                    &GamePadGUIDs,
                                                    DIEDFL_ALLDEVICES)))
        {
            for(u32 GamePadGUIDIndex = 0;
                (GamePadGUIDIndex < GamePadGUIDs.Count) && (GlobalFreeGamePads);
                ++GamePadGUIDIndex)
            {
                GUID *GamePadGUID = GamePadGUIDs.GUIDs + GamePadGUIDIndex;
                b32 AlreadyLatched = false;
                for(input_device *ActiveGamePad = GlobalActiveGamePadsSentinel.Next;
                    ActiveGamePad != &GlobalActiveGamePadsSentinel;
                    ActiveGamePad = ActiveGamePad->Next)
                {
                    Assert(ActiveGamePad->Type != ControllerType_None);
                    if(IsEqualGUID(ActiveGamePad->GUID, *GamePadGUID))
                    {
                        AlreadyLatched = true;
                        break;
                    }
                }

                if(!AlreadyLatched)
                {
                    input_device *NewGamePad = GlobalFreeGamePads;
                    NewGamePad->Type = ControllerType_None;
                    NewGamePad->Device = 0;
                    NewGamePad->XInputIndex = INVALID_XINPUT_INDEX;

                    LPDIRECTINPUTDEVICE8A Device;
                    if(SUCCEEDED(GlobalDirectInput->CreateDevice(*GamePadGUID, &Device, 0)) &&
                       SUCCEEDED(Device->SetCooperativeLevel(Window, DISCL_BACKGROUND|DISCL_NONEXCLUSIVE)))
                    {
                        DIDEVICEINSTANCE DeviceInfo;
                        DeviceInfo.dwSize = sizeof(DeviceInfo);
                        if(SUCCEEDED(Device->GetDeviceInfo(&DeviceInfo)))
                        {
                            if(IsEqualGUID(Dualshock4GUID, DeviceInfo.guidProduct))
                            {
                                NewGamePad->Type = ControllerType_Dualshock4;
                            }
                            else if(IsEqualGUID(XboxOneGUID, DeviceInfo.guidProduct))
                            {
                                NewGamePad->Type = ControllerType_XboxOne;
                            }
                            else if(IsEqualGUID(Xbox360GUID, DeviceInfo.guidProduct))
                            {
                                NewGamePad->Type = ControllerType_Xbox360;
                            }
                            else if(IsEqualGUID(N64GUID, DeviceInfo.guidProduct))
                            {
                                NewGamePad->Type = ControllerType_N64;
                            }

                            switch(NewGamePad->Type)
                            {
                                case ControllerType_Dualshock4:
                                {
                                    DIOBJECTDATAFORMAT Dualshock4ObjectDataFormat[] =
                                    {
                                        {&GUID_XAxis, OffsetOf(dualshock_4_input, LeftStickX), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
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
                                            NewGamePad->Device = Device;
                                        }
                                    }
                                } break;

                                case ControllerType_XboxOne:
                                case ControllerType_Xbox360:
                                {
                                    for(u32 XInputIndex = 0;
                                        (XInputIndex < XUSER_MAX_COUNT) && XInputGetState_;
                                        ++XInputIndex)
                                    {
                                        b32 XInputAlreadyLatched = false;
                                        for(input_device *XInputGamePad = GlobalActiveGamePadsSentinel.Next;
                                            XInputGamePad != &GlobalActiveGamePadsSentinel;
                                            XInputGamePad = XInputGamePad->Next)
                                        {
                                            if(XInputGamePad->XInputIndex == XInputIndex)
                                            {
                                                XInputAlreadyLatched = true;
                                                break;
                                            }
                                        }

                                        if(!XInputAlreadyLatched)
                                        {
                                            XINPUT_STATE XInputState;
                                            ZeroMemory(&XInputState, sizeof(XInputState));

                                            // Simply get the state of the controller from XInput.
                                            u32 GetStateResult = XInputGetState(XInputIndex, &XInputState);

                                            if(SUCCEEDED(GetStateResult))
                                            {
                                                NewGamePad->XInputIndex = XInputIndex;
                                                break;
                                            }
                                        }
                                    }
                                    if(!IsXInputGamePad(NewGamePad))
                                    {
                                        DIOBJECTDATAFORMAT XboxObjectDataFormat[] =
                                            {
                                                {&GUID_XAxis, OffsetOf(xbox_input, LeftStickX), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                                {&GUID_YAxis, OffsetOf(xbox_input, LeftStickY), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                                {&GUID_ZAxis, OffsetOf(xbox_input, Triggers), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                                {&GUID_RxAxis, OffsetOf(xbox_input, RightStickX), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                                {&GUID_RyAxis, OffsetOf(xbox_input, RightStickY), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                                {0, OffsetOf(xbox_input, A), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(0), 0},
                                                {0, OffsetOf(xbox_input, B), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(1), 0},
                                                {0, OffsetOf(xbox_input, X), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(2), 0},
                                                {0, OffsetOf(xbox_input, Y), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(3), 0},
                                                {0, OffsetOf(xbox_input, LB), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(4), 0},
                                                {0, OffsetOf(xbox_input, RB), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(5), 0},
                                                {0, OffsetOf(xbox_input, View), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(6), 0},
                                                {0, OffsetOf(xbox_input, Menu), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(7), 0},
                                                {0, OffsetOf(xbox_input, LS), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(8), 0},
                                                {0, OffsetOf(xbox_input, RS), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(9), 0},
                                                {0, OffsetOf(xbox_input, DPad), DIDFT_POV|DIDFT_ANYINSTANCE, 0},
                                            };
                                        DIDATAFORMAT XboxDataFormat;
                                        XboxDataFormat.dwSize = sizeof(DIDATAFORMAT);
                                        XboxDataFormat.dwObjSize = sizeof(DIOBJECTDATAFORMAT);
                                        XboxDataFormat.dwFlags = DIDF_ABSAXIS;
                                        XboxDataFormat.dwDataSize = sizeof(xbox_input);
                                        XboxDataFormat.dwNumObjs = ArrayCount(XboxObjectDataFormat);
                                        XboxDataFormat.rgodf = XboxObjectDataFormat;
                                        HRESULT DataFormatResult = Device->SetDataFormat(&XboxDataFormat);
                                        if(SUCCEEDED(DataFormatResult))
                                        {
                                            HRESULT Result = Device->Acquire();
                                            if(SUCCEEDED(Result))
                                            {
                                                NewGamePad->Device = Device;
                                            }
                                        }
                                    }
                                } break;

                                case ControllerType_N64:
                                {
                                    DIOBJECTDATAFORMAT N64ObjectDataFormat[] =
                                    {
                                        {&GUID_XAxis, OffsetOf(n64_input, StickX), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                        {&GUID_YAxis, OffsetOf(n64_input, StickY), DIDFT_AXIS|DIDFT_ANYINSTANCE, 0},
                                        {0, OffsetOf(n64_input, CUp), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(0), 0},
                                        {0, OffsetOf(n64_input, CRight), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(1), 0},
                                        {0, OffsetOf(n64_input, CDown), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(2), 0},
                                        {0, OffsetOf(n64_input, CLeft), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(3), 0},
                                        {0, OffsetOf(n64_input, L), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(4), 0},
                                        {0, OffsetOf(n64_input, R), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(5), 0},
                                        {0, OffsetOf(n64_input, A), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(6), 0},
                                        {0, OffsetOf(n64_input, Z), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(7), 0},
                                        {0, OffsetOf(n64_input, B), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(8), 0},
                                        {0, OffsetOf(n64_input, Start), DIDFT_BUTTON|DIDFT_MAKEINSTANCE(9), 0},
                                        {0, OffsetOf(n64_input, DPad), DIDFT_POV|DIDFT_ANYINSTANCE, 0},
                                    };
                                    DIDATAFORMAT N64DataFormat;
                                    N64DataFormat.dwSize = sizeof(DIDATAFORMAT);
                                    N64DataFormat.dwObjSize = sizeof(DIOBJECTDATAFORMAT);
                                    N64DataFormat.dwFlags = DIDF_ABSAXIS;
                                    N64DataFormat.dwDataSize = sizeof(n64_input);
                                    N64DataFormat.dwNumObjs = ArrayCount(N64ObjectDataFormat);
                                    N64DataFormat.rgodf = N64ObjectDataFormat;
                                    HRESULT DataFormatResult = Device->SetDataFormat(&N64DataFormat);
                                    if(SUCCEEDED(DataFormatResult))
                                    {
                                        HRESULT Result = Device->Acquire();
                                        if(SUCCEEDED(Result))
                                        {
                                            NewGamePad->Device = Device;
                                        }
                                    }
                                } break;
                            }
                            
                            if(IsXInputGamePad(NewGamePad) || NewGamePad->Device)
                            {
                                GlobalFreeGamePads = GlobalFreeGamePads->Next;
                                NewGamePad->Next = GlobalActiveGamePadsSentinel.Next;
                                GlobalActiveGamePadsSentinel.Next = NewGamePad;

                                NewGamePad->GUID = *GamePadGUID;
                            }
                        }
                    }
                }
            }
        }
    }
}

inline void
ProcessButton(game_button *OldButton, game_button *NewButton, u16 State)
{
    NewButton->EndedDown = State;
    NewButton->HalfTransitionCount = (OldButton->EndedDown == NewButton->EndedDown) ? 0 : 1;
}

inline u32
LatchedGamePadCount()
{
    u32 Result = 0;
    for(input_device *ActiveGamePad = GlobalActiveGamePadsSentinel.Next;
        ActiveGamePad != &GlobalActiveGamePadsSentinel;
        ActiveGamePad = ActiveGamePad->Next)
    {
        ++Result;
    }
    return(Result);
}

internal void
ProcessGamePadInput(game_input *OldInput, game_input *NewInput)
{
    for(input_device *ActiveGamePad = GlobalActiveGamePadsSentinel.Next;
        ActiveGamePad != &GlobalActiveGamePadsSentinel;
        ActiveGamePad = ActiveGamePad->Next)
    {
        game_controller *OldGamePad = OldInput->GamePads + ActiveGamePad->Index;
        game_controller *NewGamePad = NewInput->GamePads + ActiveGamePad->Index;
        NewGamePad->Type = ActiveGamePad->Type;
        LPDIRECTINPUTDEVICE8A Device = ActiveGamePad->Device;
        switch(ActiveGamePad->Type)
        {
            case ControllerType_Dualshock4:
            {
                if(Device)
                {
                    dualshock_4_input Dualshock4Data = {};
                    HRESULT DataFetchResult = Device->GetDeviceState(sizeof(Dualshock4Data), &Dualshock4Data);
                    if(SUCCEEDED(DataFetchResult))
                    {
                        // NOTE(chris): Analog sticks
                        {
                            Dualshock4Data.LeftStickY = 0xFFFF - Dualshock4Data.LeftStickY;
                            Dualshock4Data.RightStickY = 0xFFFF - Dualshock4Data.RightStickY;
                            r32 Center = 0xFFFF / 2.0f;
                            s32 Deadzone = 1500;
                        
                            NewGamePad->LeftStick.x = 0.0f;
                            if(Dualshock4Data.LeftStickX < (Center - Deadzone))
                            {
                                NewGamePad->LeftStick.x = -(Center - Deadzone - (r32)Dualshock4Data.LeftStickX) / (Center - Deadzone);
                            }
                            else if(Dualshock4Data.LeftStickX > (Center + Deadzone))
                            {
                                NewGamePad->LeftStick.x = ((r32)Dualshock4Data.LeftStickX - Center - Deadzone) / (Center - Deadzone);
                            }

                            NewGamePad->LeftStick.y = 0.0f;
                            if(Dualshock4Data.LeftStickY < (Center - Deadzone))
                            {
                                NewGamePad->LeftStick.y = -(Center - Deadzone - (r32)Dualshock4Data.LeftStickY) / (Center - Deadzone);
                            }
                            else if(Dualshock4Data.LeftStickY > (Center + Deadzone))
                            {
                                NewGamePad->LeftStick.y = ((r32)Dualshock4Data.LeftStickY - Center - Deadzone) / (Center - Deadzone);
                            }

                            NewGamePad->RightStick.x = 0.0f;
                            if(Dualshock4Data.RightStickX < (Center - Deadzone))
                            {
                                NewGamePad->RightStick.x = -(Center - Deadzone - (r32)Dualshock4Data.RightStickX) / (Center - Deadzone);
                            }
                            else if(Dualshock4Data.RightStickX > (Center + Deadzone))
                            {
                                NewGamePad->RightStick.x = ((r32)Dualshock4Data.RightStickX - Center - Deadzone) / (Center - Deadzone);
                            }

                            NewGamePad->RightStick.y = 0.0f;
                            if(Dualshock4Data.RightStickY < (Center - Deadzone))
                            {
                                NewGamePad->RightStick.y = -(Center - Deadzone - (r32)Dualshock4Data.RightStickY) / (Center - Deadzone);
                            }
                            else if(Dualshock4Data.RightStickY > (Center + Deadzone))
                            {
                                NewGamePad->RightStick.y = ((r32)Dualshock4Data.RightStickY - Center - Deadzone) / (Center - Deadzone);
                            }
                        }

                        // NOTE(chris): Triggers
                        {
                            NewGamePad->LeftTrigger = (r32)Dualshock4Data.LeftTrigger / (r32)0xFFFF;
                            NewGamePad->RightTrigger = (r32)Dualshock4Data.RightTrigger / (r32)0xFFFF;
                        }

                        // NOTE(chris): Buttons
                        {
                            ProcessButton(&OldGamePad->ActionLeft, &NewGamePad->ActionLeft, Dualshock4Data.Square);
                            ProcessButton(&OldGamePad->ActionDown, &NewGamePad->ActionDown, Dualshock4Data.Cross);
                            ProcessButton(&OldGamePad->ActionRight, &NewGamePad->ActionRight, Dualshock4Data.Circle);
                            ProcessButton(&OldGamePad->ActionUp, &NewGamePad->ActionUp, Dualshock4Data.Triangle);
                            ProcessButton(&OldGamePad->LeftShoulder1, &NewGamePad->LeftShoulder1, Dualshock4Data.L1);
                            ProcessButton(&OldGamePad->RightShoulder1, &NewGamePad->RightShoulder1, Dualshock4Data.R1);
                            ProcessButton(&OldGamePad->LeftShoulder2, &NewGamePad->LeftShoulder2, Dualshock4Data.L2);
                            ProcessButton(&OldGamePad->RightShoulder2, &NewGamePad->RightShoulder2, Dualshock4Data.R2);
                            ProcessButton(&OldGamePad->Select, &NewGamePad->Select, Dualshock4Data.Share);
                            ProcessButton(&OldGamePad->Start, &NewGamePad->Start, Dualshock4Data.Options);
                            ProcessButton(&OldGamePad->LeftClick, &NewGamePad->LeftClick, Dualshock4Data.L3);
                            ProcessButton(&OldGamePad->RightClick, &NewGamePad->RightClick, Dualshock4Data.R3);
                            ProcessButton(&OldGamePad->Power, &NewGamePad->Power, Dualshock4Data.PS);
                            ProcessButton(&OldGamePad->CenterClick, &NewGamePad->CenterClick, Dualshock4Data.Touchpad);
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
                                    NewGamePad->LeftStick.x = 0.0f;
                                    NewGamePad->LeftStick.y = 1.0f;
                                } break;

                                case 4500:
                                {
                                    NewGamePad->LeftStick.x = 1.0f;
                                    NewGamePad->LeftStick.y = 1.0f;
                                } break;

                                case 9000:
                                {
                                    NewGamePad->LeftStick.x = 1.0f;
                                    NewGamePad->LeftStick.y = 0.0f;
                                } break;

                                case 13500:
                                {
                                    NewGamePad->LeftStick.x = 1.0f;
                                    NewGamePad->LeftStick.y = -1.0f;
                                } break;

                                case 18000:
                                {
                                    NewGamePad->LeftStick.x = 0.0f;
                                    NewGamePad->LeftStick.y = -1.0f;
                                } break;

                                case 22500:
                                {
                                    NewGamePad->LeftStick.x = -1.0f;
                                    NewGamePad->LeftStick.y = -1.0f;
                                } break;

                                case 27000:
                                {
                                    NewGamePad->LeftStick.x = -1.0f;
                                    NewGamePad->LeftStick.y = 0.0f;
                                } break;

                                case 31500:
                                {
                                    NewGamePad->LeftStick.x = -1.0f;
                                    NewGamePad->LeftStick.y = 1.0f;
                                } break;

                                InvalidDefaultCase;
                            }
                        }

                        Assert(NewGamePad->LeftStick.x >= -1.0f);
                        Assert(NewGamePad->LeftStick.x <= 1.0f);
                        Assert(NewGamePad->LeftStick.y >= -1.0f);
                        Assert(NewGamePad->LeftStick.y <= 1.0f);
                        Assert(NewGamePad->RightStick.x >= -1.0f);
                        Assert(NewGamePad->RightStick.x <= 1.0f);
                        Assert(NewGamePad->RightStick.y >= -1.0f);
                        Assert(NewGamePad->RightStick.y <= 1.0f);
                        Assert(NewGamePad->LeftTrigger >= 0.0f);
                        Assert(NewGamePad->LeftTrigger <= 1.0f);
                        Assert(NewGamePad->RightTrigger >= 0.0f);
                        Assert(NewGamePad->RightTrigger <= 1.0f);
                    }
                }
            } break;

            case ControllerType_XboxOne:
            case ControllerType_Xbox360:
            {
                if(IsXInputGamePad(ActiveGamePad))
                {
#if TROIDS_RUMBLE
                    if(XInputSetState_)
                    {
                        XINPUT_VIBRATION Vibration;
                        Vibration.wLeftMotorSpeed = (u16)(OldGamePad->LowFrequencyMotor*(r32)0xFFFF + 0.5f);
                        Vibration.wRightMotorSpeed = (u16)(OldGamePad->HighFrequencyMotor*(r32)0xFFF + 0.5f);

                        b32 SetStateResult = XInputSetState(ActiveGamePad->XInputIndex, &Vibration);
                        Assert(SUCCEEDED(SetStateResult));
                    }
#endif

                    Assert(XInputGetState_);
                    XINPUT_STATE XInputState;
                    ZeroMemory(&XInputState, sizeof(XInputState));

                    // Simply get the state of the controller from XInput.
                    u32 GetStateResult = XInputGetState(ActiveGamePad->XInputIndex, &XInputState);

                    Assert(SUCCEEDED(GetStateResult));

                    XINPUT_GAMEPAD *GamePad = &XInputState.Gamepad;
                    // NOTE(chris): Analog sticks
                    {
                        {
                            s32 Deadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
                        
                            NewGamePad->LeftStick.x = 0.0f;
                            if(GamePad->sThumbLX < -Deadzone)
                            {
                                NewGamePad->LeftStick.x = -(r32)(GamePad->sThumbLX + Deadzone) / (S32_MIN + Deadzone);
                            }
                            else if(GamePad->sThumbLX > Deadzone)
                            {
                                NewGamePad->LeftStick.x = (r32)(GamePad->sThumbLX - Deadzone) / (S32_MAX - Deadzone);
                            }

                            NewGamePad->LeftStick.y = 0.0f;
                            if(GamePad->sThumbLY < -Deadzone)
                            {
                                NewGamePad->LeftStick.y = -(r32)(GamePad->sThumbLY + Deadzone) / (S32_MIN + Deadzone);
                            }
                            else if(GamePad->sThumbLY > Deadzone)
                            {
                                NewGamePad->LeftStick.y = (r32)(GamePad->sThumbLY - Deadzone) / (S32_MAX - Deadzone);
                            }
                        }


                        {
                            s32 Deadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;

                            NewGamePad->RightStick.x = 0.0f;
                            if(GamePad->sThumbRX < -Deadzone)
                            {
                                NewGamePad->RightStick.x = -(r32)(GamePad->sThumbRX + Deadzone) / (S32_MIN + Deadzone);
                            }
                            else if(GamePad->sThumbRX > Deadzone)
                            {
                                NewGamePad->RightStick.x = (r32)(GamePad->sThumbRX - Deadzone) / (S32_MAX - Deadzone);
                            }

                            NewGamePad->RightStick.y = 0.0f;
                            if(GamePad->sThumbRY < -Deadzone)
                            {
                                NewGamePad->RightStick.y = -(r32)(GamePad->sThumbRY + Deadzone) / (S32_MIN + Deadzone);
                            }
                            else if(GamePad->sThumbRY > Deadzone)
                            {
                                NewGamePad->RightStick.y = (r32)(GamePad->sThumbRY - Deadzone) / (S32_MAX - Deadzone);
                            }
                        }
                    }

                    // NOTE(chris): Triggers
                    {
                        r32 Deadzone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
                        NewGamePad->LeftTrigger = 0.0f;
                        NewGamePad->RightTrigger = 0.0f;
                        if(GamePad->bLeftTrigger > Deadzone)
                        {
                            NewGamePad->LeftTrigger = (r32)(GamePad->bLeftTrigger - Deadzone) / (U8_MAX - Deadzone);
                            ProcessButton(&OldGamePad->LeftShoulder2, &NewGamePad->LeftShoulder2, 1);
                        }
                        if(GamePad->bRightTrigger > Deadzone)
                        {
                            NewGamePad->RightTrigger = (r32)(GamePad->bRightTrigger - Deadzone) / (U8_MAX - Deadzone);
                            ProcessButton(&OldGamePad->RightShoulder2, &NewGamePad->RightShoulder2, 1);
                        }
                    }

                    // NOTE(chris): Buttons
                    {
                        ProcessButton(&OldGamePad->ActionLeft, &NewGamePad->ActionLeft,
                                      GamePad->wButtons & XINPUT_GAMEPAD_X);
                        ProcessButton(&OldGamePad->ActionDown, &NewGamePad->ActionDown,
                                      GamePad->wButtons & XINPUT_GAMEPAD_A);
                        ProcessButton(&OldGamePad->ActionRight, &NewGamePad->ActionRight,
                                      GamePad->wButtons & XINPUT_GAMEPAD_B);
                        ProcessButton(&OldGamePad->ActionUp, &NewGamePad->ActionUp,
                                      GamePad->wButtons & XINPUT_GAMEPAD_Y);
                        ProcessButton(&OldGamePad->LeftShoulder1, &NewGamePad->LeftShoulder1,
                                      GamePad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        ProcessButton(&OldGamePad->RightShoulder1, &NewGamePad->RightShoulder1,
                                      GamePad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        ProcessButton(&OldGamePad->Select, &NewGamePad->Select,
                                      GamePad->wButtons & XINPUT_GAMEPAD_BACK);
                        ProcessButton(&OldGamePad->Start, &NewGamePad->Start,
                                      GamePad->wButtons & XINPUT_GAMEPAD_START);
                        ProcessButton(&OldGamePad->LeftClick, &NewGamePad->LeftClick,
                                      GamePad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
                        ProcessButton(&OldGamePad->RightClick, &NewGamePad->RightClick,
                                      GamePad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
                    }

                    if(GamePad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                    {
                        NewGamePad->LeftStick.y = 1.0f;
                    }
                    else if(GamePad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                    {
                        NewGamePad->LeftStick.y = -1.0f;
                    }
                    
                    if(GamePad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                    {
                        NewGamePad->LeftStick.x = -1.0f;
                    }
                    else if(GamePad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                    {
                        NewGamePad->LeftStick.x = 1.0f;
                    }

                    Assert(NewGamePad->LeftStick.x >= -1.0f);
                    Assert(NewGamePad->LeftStick.x <= 1.0f);
                    Assert(NewGamePad->LeftStick.y >= -1.0f);
                    Assert(NewGamePad->LeftStick.y <= 1.0f);
                    Assert(NewGamePad->RightStick.x >= -1.0f);
                    Assert(NewGamePad->RightStick.x <= 1.0f);
                    Assert(NewGamePad->RightStick.y >= -1.0f);
                    Assert(NewGamePad->RightStick.y <= 1.0f);
                    Assert(NewGamePad->LeftTrigger >= 0.0f);
                    Assert(NewGamePad->LeftTrigger <= 1.0f);
                    Assert(NewGamePad->RightTrigger >= 0.0f);
                    Assert(NewGamePad->RightTrigger <= 1.0f);
                }
                else if(Device)
                {
                    xbox_input XboxData = {};
                    HRESULT DataFetchResult = Device->GetDeviceState(sizeof(XboxData), &XboxData);
                    if(SUCCEEDED(DataFetchResult))
                    {
                        r32 Center = 0xFFFF / 2.0f;
                        // NOTE(chris): Analog sticks
                        {
                            {
                                XboxData.LeftStickY = 0xFFFF - XboxData.LeftStickY;
                                s32 Deadzone = XBOX_GAMEPAD_LEFT_THUMB_DEADZONE;
                        
                                NewGamePad->LeftStick.x = 0.0f;
                                if(XboxData.LeftStickX < (Center - Deadzone))
                                {
                                    NewGamePad->LeftStick.x = -(Center - Deadzone - (r32)XboxData.LeftStickX) / (Center - Deadzone);
                                }
                                else if(XboxData.LeftStickX > (Center + Deadzone))
                                {
                                    NewGamePad->LeftStick.x = ((r32)XboxData.LeftStickX - Center - Deadzone) / (Center - Deadzone);
                                }

                                NewGamePad->LeftStick.y = 0.0f;
                                if(XboxData.LeftStickY < (Center - Deadzone))
                                {
                                    NewGamePad->LeftStick.y = -(Center - Deadzone - (r32)XboxData.LeftStickY) / (Center - Deadzone);
                                }
                                else if(XboxData.LeftStickY > (Center + Deadzone))
                                {
                                    NewGamePad->LeftStick.y = ((r32)XboxData.LeftStickY - Center - Deadzone) / (Center - Deadzone);
                                }
                            }


                            {
                                XboxData.RightStickY = 0xFFFF - XboxData.RightStickY;
                                s32 Deadzone = XBOX_GAMEPAD_RIGHT_THUMB_DEADZONE;

                                NewGamePad->RightStick.x = 0.0f;
                                if(XboxData.RightStickX < (Center - Deadzone))
                                {
                                    NewGamePad->RightStick.x = -(Center - Deadzone - (r32)XboxData.RightStickX) / (Center - Deadzone);
                                }
                                else if(XboxData.RightStickX > (Center + Deadzone))
                                {
                                    NewGamePad->RightStick.x = ((r32)XboxData.RightStickX - Center - Deadzone) / (Center - Deadzone);
                                }

                                NewGamePad->RightStick.y = 0.0f;
                                if(XboxData.RightStickY < (Center - Deadzone))
                                {
                                    NewGamePad->RightStick.y = -(Center - Deadzone - (r32)XboxData.RightStickY) / (Center - Deadzone);
                                }
                                else if(XboxData.RightStickY > (Center + Deadzone))
                                {
                                    NewGamePad->RightStick.y = ((r32)XboxData.RightStickY - Center - Deadzone) / (Center - Deadzone);
                                }
                            }
                        }

                        // NOTE(chris): Triggers
                        {
                            r32 Deadzone = XBOX_GAMEPAD_TRIGGER_THRESHOLD;
                            NewGamePad->LeftTrigger = 0.0f;
                            NewGamePad->RightTrigger = 0.0f;
                            u32 TriggerMax = 0xff80;
                            u32 TriggerMin = 0x0080;
                            if(XboxData.Triggers > (Center + Deadzone))
                            {
                                NewGamePad->LeftTrigger = ((r32)XboxData.Triggers - Center - Deadzone) / (TriggerMax - Center - Deadzone);
                                ProcessButton(&OldGamePad->LeftShoulder2, &NewGamePad->LeftShoulder2, 1);
                            }
                            else if(XboxData.Triggers < (Center - Deadzone))
                            {
                                NewGamePad->RightTrigger = (Center - Deadzone - (r32)XboxData.Triggers) / (Center - Deadzone - TriggerMin);
                                ProcessButton(&OldGamePad->RightShoulder2, &NewGamePad->RightShoulder2, 1);
                            }
                        }

                        // NOTE(chris): Buttons
                        {
                            ProcessButton(&OldGamePad->ActionLeft, &NewGamePad->ActionLeft, XboxData.X);
                            ProcessButton(&OldGamePad->ActionDown, &NewGamePad->ActionDown, XboxData.A);
                            ProcessButton(&OldGamePad->ActionRight, &NewGamePad->ActionRight, XboxData.B);
                            ProcessButton(&OldGamePad->ActionUp, &NewGamePad->ActionUp, XboxData.Y);
                            ProcessButton(&OldGamePad->LeftShoulder1, &NewGamePad->LeftShoulder1, XboxData.LB);
                            ProcessButton(&OldGamePad->RightShoulder1, &NewGamePad->RightShoulder1, XboxData.RB);
                            ProcessButton(&OldGamePad->Select, &NewGamePad->Select, XboxData.View);
                            ProcessButton(&OldGamePad->Start, &NewGamePad->Start, XboxData.Menu);
                            ProcessButton(&OldGamePad->LeftClick, &NewGamePad->LeftClick, XboxData.LS);
                            ProcessButton(&OldGamePad->RightClick, &NewGamePad->RightClick, XboxData.RS);
                        }

                        // NOTE(chris): DPad
                        {
                            switch(XboxData.DPad)
                            {
                                case 0xFFFFFFFF:
                                {
                                    // NOTE(chris): Centered. Don't interfere with analog sticks.
                                } break;

                                case 0:
                                {
                                    NewGamePad->LeftStick.x = 0.0f;
                                    NewGamePad->LeftStick.y = 1.0f;
                                } break;

                                case 4500:
                                {
                                    NewGamePad->LeftStick.x = 1.0f;
                                    NewGamePad->LeftStick.y = 1.0f;
                                } break;

                                case 9000:
                                {
                                    NewGamePad->LeftStick.x = 1.0f;
                                    NewGamePad->LeftStick.y = 0.0f;
                                } break;

                                case 13500:
                                {
                                    NewGamePad->LeftStick.x = 1.0f;
                                    NewGamePad->LeftStick.y = -1.0f;
                                } break;

                                case 18000:
                                {
                                    NewGamePad->LeftStick.x = 0.0f;
                                    NewGamePad->LeftStick.y = -1.0f;
                                } break;

                                case 22500:
                                {
                                    NewGamePad->LeftStick.x = -1.0f;
                                    NewGamePad->LeftStick.y = -1.0f;
                                } break;

                                case 27000:
                                {
                                    NewGamePad->LeftStick.x = -1.0f;
                                    NewGamePad->LeftStick.y = 0.0f;
                                } break;

                                case 31500:
                                {
                                    NewGamePad->LeftStick.x = -1.0f;
                                    NewGamePad->LeftStick.y = 1.0f;
                                } break;

                                InvalidDefaultCase;
                            }
                        }

                        Assert(NewGamePad->LeftStick.x >= -1.0f);
                        Assert(NewGamePad->LeftStick.x <= 1.0f);
                        Assert(NewGamePad->LeftStick.y >= -1.0f);
                        Assert(NewGamePad->LeftStick.y <= 1.0f);
                        Assert(NewGamePad->RightStick.x >= -1.0f);
                        Assert(NewGamePad->RightStick.x <= 1.0f);
                        Assert(NewGamePad->RightStick.y >= -1.0f);
                        Assert(NewGamePad->RightStick.y <= 1.0f);
                        Assert(NewGamePad->LeftTrigger >= 0.0f);
                        Assert(NewGamePad->LeftTrigger <= 1.0f);
                        Assert(NewGamePad->RightTrigger >= 0.0f);
                        Assert(NewGamePad->RightTrigger <= 1.0f);
                    }
                }
            } break;

            case ControllerType_N64:
            {
                if(Device)
                {
                    n64_input N64Data = {};
                    HRESULT DataFetchResult = Device->GetDeviceState(sizeof(N64Data), &N64Data);
                    if(SUCCEEDED(DataFetchResult))
                    {
                        // NOTE(chris): Analog sticks
                        {
                            N64Data.StickY = 0xFFFF - N64Data.StickY;
                            r32 Center = 0xFFFF / 2.0f;
                            r32 Deadzone = 1500;
                        
                            NewGamePad->LeftStick.x = 0.0f;
                            if(N64Data.StickX < (Center - Deadzone))
                            {
                                NewGamePad->LeftStick.x = -(Center - Deadzone - (r32)N64Data.StickX) / (Center - Deadzone);
                            }
                            else if(N64Data.StickX > (Center + Deadzone))
                            {
                                NewGamePad->LeftStick.x = ((r32)N64Data.StickX - Center - Deadzone) / (Center - Deadzone);
                            }

                            NewGamePad->LeftStick.y = 0.0f;
                            if(N64Data.StickY < (Center - Deadzone))
                            {
                                NewGamePad->LeftStick.y = -(Center - Deadzone - (r32)N64Data.StickY) / (Center - Deadzone);
                            }
                            else if(N64Data.StickY > (Center + Deadzone))
                            {
                                NewGamePad->LeftStick.y = ((r32)N64Data.StickY - Center - Deadzone) / (Center - Deadzone);
                            }

                            NewGamePad->RightStick.x = 0.0f;
                            if(N64Data.CLeft)
                            {
                                NewGamePad->RightStick.x -= 1.0f;
                            }
                            if(N64Data.CRight)
                            {
                                NewGamePad->RightStick.x += 1.0f;
                            }

                            NewGamePad->RightStick.y = 0.0f;
                            if(N64Data.CDown)
                            {
                                NewGamePad->RightStick.y -= 1.0f;
                            }
                            if(N64Data.CUp)
                            {
                                NewGamePad->RightStick.y += 1.0f;
                            }
                        }

                        // NOTE(chris): Buttons
                        {
                            ProcessButton(&OldGamePad->ActionLeft, &NewGamePad->ActionLeft,
                                          N64Data.B);
                            ProcessButton(&OldGamePad->ActionDown, &NewGamePad->ActionDown,
                                          N64Data.A);
                            ProcessButton(&OldGamePad->LeftShoulder1, &NewGamePad->LeftShoulder1,
                                          N64Data.L|N64Data.Z);
                            ProcessButton(&OldGamePad->RightShoulder1, &NewGamePad->RightShoulder1,
                                          N64Data.R);
                            ProcessButton(&OldGamePad->Start, &NewGamePad->Start, N64Data.Start);
                        }

                        // NOTE(chris): DPad
                        {
                            switch(N64Data.DPad)
                            {
                                case 0xFFFFFFFF:
                                {
                                    // NOTE(chris): Centered. Don't interfere with analog sticks.
                                } break;

                                case 0:
                                {
                                    NewGamePad->LeftStick.x = 0.0f;
                                    NewGamePad->LeftStick.y = 1.0f;
                                } break;

                                case 4500:
                                {
                                    NewGamePad->LeftStick.x = 1.0f;
                                    NewGamePad->LeftStick.y = 1.0f;
                                } break;

                                case 9000:
                                {
                                    NewGamePad->LeftStick.x = 1.0f;
                                    NewGamePad->LeftStick.y = 0.0f;
                                } break;

                                case 13500:
                                {
                                    NewGamePad->LeftStick.x = 1.0f;
                                    NewGamePad->LeftStick.y = -1.0f;
                                } break;

                                case 18000:
                                {
                                    NewGamePad->LeftStick.x = 0.0f;
                                    NewGamePad->LeftStick.y = -1.0f;
                                } break;

                                case 22500:
                                {
                                    NewGamePad->LeftStick.x = -1.0f;
                                    NewGamePad->LeftStick.y = -1.0f;
                                } break;

                                case 27000:
                                {
                                    NewGamePad->LeftStick.x = -1.0f;
                                    NewGamePad->LeftStick.y = 0.0f;
                                } break;

                                case 31500:
                                {
                                    NewGamePad->LeftStick.x = -1.0f;
                                    NewGamePad->LeftStick.y = 1.0f;
                                } break;

                                InvalidDefaultCase;
                            }
                        }

                        Assert(NewGamePad->LeftStick.x >= -1.0f);
                        Assert(NewGamePad->LeftStick.x <= 1.0f);
                        Assert(NewGamePad->LeftStick.y >= -1.0f);
                        Assert(NewGamePad->LeftStick.y <= 1.0f);
                        Assert(NewGamePad->RightStick.x >= -1.0f);
                        Assert(NewGamePad->RightStick.x <= 1.0f);
                        Assert(NewGamePad->RightStick.y >= -1.0f);
                        Assert(NewGamePad->RightStick.y <= 1.0f);
                        Assert(NewGamePad->LeftTrigger >= 0.0f);
                        Assert(NewGamePad->LeftTrigger <= 1.0f);
                        Assert(NewGamePad->RightTrigger >= 0.0f);
                        Assert(NewGamePad->RightTrigger <= 1.0f);
                    }
                }
            } break;
        }
    }
}
