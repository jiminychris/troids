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
CREATE_GUID(Dualshock4GUID, 0x05c4054c,0x0000,0x0000,0x00,0x00,0x50,0x49,0x44,0x56,0x49,0x44);
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
#define DIRECT_INPUT_CREATE(Name) HRESULT Name(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, LPUNKNOWN punkOuter);
typedef DIRECT_INPUT_CREATE(direct_input_create);

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

enum input_device_type
{
    InputDeviceType_None,
    
    InputDeviceType_Dualshock4,
    InputDeviceType_N64,
};

struct input_device
{
    input_device_type Type;
    LPDIRECTINPUTDEVICE8A Device;
    GUID GUID;
};

// TODO(chris): Access free and active controllers using a linked list, nerf GlobalControllerCount
global_variable u32 GlobalControllerCount = 0;
global_variable input_device GlobalControllers[MAX_CONTROLLERS] = {};
global_variable direct_input_create *DirectInputCreate = 0;
global_variable LPDIRECTINPUT8 GlobalDirectInput = 0;

internal void
InitializeInput(HINSTANCE Instance)
{
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
    device_guids *ControllerGUIDs = (device_guids *)Data;
    // NOTE(chris): The end condition should catch this. First time through this should never trigger.
    Assert(ControllerGUIDs->Count < ArrayCount(ControllerGUIDs->GUIDs));

    ControllerGUIDs->GUIDs[ControllerGUIDs->Count++] = EnumeratedDevice->guidInstance;

    BOOL Result = ((ControllerGUIDs->Count < ArrayCount(ControllerGUIDs->GUIDs)) ?
                   DIENUM_CONTINUE : DIENUM_STOP);
    return(Result);
}

internal void
UnlatchController()
{
    if(GlobalDirectInput)
    {
        device_guids ControllerGUIDs = {};
        if(SUCCEEDED(GlobalDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL,
                                                    EnumDevicesCallback,
                                                    &ControllerGUIDs,
                                                    DIEDFL_ALLDEVICES)))
        {
            for(u32 GlobalControllerIndex = 0;
                GlobalControllerIndex < ArrayCount(GlobalControllers);
                ++GlobalControllerIndex)
            {
                input_device *GlobalController = GlobalControllers + GlobalControllerIndex;
                if(GlobalController->Type != InputDeviceType_None)
                {
                    b32 StillAttached = false;
                    for(u32 DeviceGUIDIndex = 0;
                        DeviceGUIDIndex < ControllerGUIDs.Count;
                        ++DeviceGUIDIndex)
                    {
                        GUID *DeviceGUID = ControllerGUIDs.GUIDs + DeviceGUIDIndex;
                        if(IsEqualGUID(*DeviceGUID, GlobalController->GUID))
                        {
                            StillAttached = true;
                        }
                    }
                    if(!StillAttached)
                    {
                        GlobalController->Type = InputDeviceType_None;
                        --GlobalControllerCount;
                    }
                }
            }
        }
    }
}

internal void
LatchControllers(HWND Window)
{
    if(GlobalDirectInput)
    {
        device_guids ControllerGUIDs = {};
        if(SUCCEEDED(GlobalDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL,
                                                    EnumDevicesCallback,
                                                    &ControllerGUIDs,
                                                    DIEDFL_ALLDEVICES)))
        {
            for(u32 ControllerGUIDIndex = 0;
                (ControllerGUIDIndex < ControllerGUIDs.Count) && (GlobalControllerCount < ArrayCount(GlobalControllers));
                ++ControllerGUIDIndex)
            {
                GUID *ControllerGUID = ControllerGUIDs.GUIDs + ControllerGUIDIndex;
                b32 AlreadyLatched = false;
                for(u32 ControllerIndex = 0;
                    ControllerIndex < ArrayCount(GlobalControllers);
                    ++ControllerIndex)
                {
                    input_device *Controller = GlobalControllers + ControllerIndex;
                    if((Controller->Type != InputDeviceType_None) &&
                       (IsEqualGUID(Controller->GUID, *ControllerGUID)))
                    {
                        AlreadyLatched = true;
                        break;
                    }
                }

                if(!AlreadyLatched)
                {
                    input_device *NewController = 0;
                    for(u32 ControllerIndex = 0;
                        ControllerIndex < ArrayCount(GlobalControllers);
                        ++ControllerIndex)
                    {
                        input_device *Controller = GlobalControllers + ControllerIndex;
                        if(Controller->Type == InputDeviceType_None)
                        {
                            NewController = Controller;
                            break;
                        }
                    }
                    NewController->GUID = *ControllerGUID;
                    LPDIRECTINPUTDEVICE8A Device;
                    if(SUCCEEDED(GlobalDirectInput->CreateDevice(*ControllerGUID, &Device, 0)) &&
                       SUCCEEDED(Device->SetCooperativeLevel(Window, DISCL_BACKGROUND|DISCL_NONEXCLUSIVE)))
                    {
                        DIDEVICEINSTANCE DeviceInfo;
                        DeviceInfo.dwSize = sizeof(DeviceInfo);
                        if(SUCCEEDED(Device->GetDeviceInfo(&DeviceInfo)))
                        {
                            if(IsEqualGUID(Dualshock4GUID, DeviceInfo.guidProduct))
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
                                        NewController->Type = InputDeviceType_Dualshock4;
                                        NewController->Device = Device;
                                        ++GlobalControllerCount;
                                    }
                                }
                            }
                            else if(IsEqualGUID(N64GUID, DeviceInfo.guidProduct))
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
                                        NewController->Type = InputDeviceType_N64;
                                        NewController->Device = Device;
                                        ++GlobalControllerCount;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

inline void
ProcessButton(game_button *OldButton, game_button *NewButton, u8 State)
{
    NewButton->EndedDown = State;
    NewButton->HalfTransitionCount = (OldButton->EndedDown == NewButton->EndedDown) ? 0 : 1;
}

internal void
ProcessControllerInput(u32 ControllerIndex, game_controller *OldController, game_controller *NewController)
{
    input_device Controller = GlobalControllers[ControllerIndex];
    switch(Controller.Type)
    {
        case InputDeviceType_Dualshock4:
        {
            LPDIRECTINPUTDEVICE8A Dualshock4Controller = Controller.Device;
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
                        
                        NewController->LeftStickX = 0.0f;
                        if(Dualshock4Data.LeftStickX < (Center - Deadzone))
                        {
                            NewController->LeftStickX = (r32)(-(Center - Deadzone) + Dualshock4Data.LeftStickX) / (r32)(Center - Deadzone);
                        }
                        else if(Dualshock4Data.LeftStickX > (Center + Deadzone))
                        {
                            NewController->LeftStickX = (r32)(Dualshock4Data.LeftStickX - Center - Deadzone) / (r32)(Center - Deadzone);
                        }

                        NewController->LeftStickY = 0.0f;
                        if(Dualshock4Data.LeftStickY < (Center - Deadzone))
                        {
                            NewController->LeftStickY = (r32)(-(Center - Deadzone) + Dualshock4Data.LeftStickY) / (r32)(Center - Deadzone);
                        }
                        else if(Dualshock4Data.LeftStickY > (Center + Deadzone))
                        {
                            NewController->LeftStickY = (r32)(Dualshock4Data.LeftStickY - Center - Deadzone) / (r32)(Center - Deadzone);
                        }

                        NewController->RightStickX = 0.0f;
                        if(Dualshock4Data.RightStickX < (Center - Deadzone))
                        {
                            NewController->RightStickX = (r32)(-(Center - Deadzone) + Dualshock4Data.RightStickX) / (r32)(Center - Deadzone);
                        }
                        else if(Dualshock4Data.RightStickX > (Center + Deadzone))
                        {
                            NewController->RightStickX = (r32)(Dualshock4Data.RightStickX - Center - Deadzone) / (r32)(Center - Deadzone);
                        }

                        NewController->RightStickY = 0.0f;
                        if(Dualshock4Data.RightStickY < (Center - Deadzone))
                        {
                            NewController->RightStickY = (r32)(-(Center - Deadzone) + Dualshock4Data.RightStickY) / (r32)(Center - Deadzone);
                        }
                        else if(Dualshock4Data.RightStickY > (Center + Deadzone))
                        {
                            NewController->RightStickY = (r32)(Dualshock4Data.RightStickY - Center - Deadzone) / (r32)(Center - Deadzone);
                        }
                    }

                    // NOTE(chris): Triggers
                    {
                        NewController->LeftTrigger = (r32)Dualshock4Data.LeftTrigger / 0xFFFF;
                        NewController->RightTrigger = (r32)Dualshock4Data.RightTrigger / 0xFFFF;
                    }

                    // NOTE(chris): Buttons
                    {
                        ProcessButton(&OldController->ActionLeft, &NewController->ActionLeft, Dualshock4Data.Square);
                        ProcessButton(&OldController->ActionDown, &NewController->ActionDown, Dualshock4Data.Cross);
                        ProcessButton(&OldController->ActionRight, &NewController->ActionRight, Dualshock4Data.Circle);
                        ProcessButton(&OldController->ActionUp, &NewController->ActionUp, Dualshock4Data.Triangle);
                        ProcessButton(&OldController->LeftShoulder1, &NewController->LeftShoulder1, Dualshock4Data.L1);
                        ProcessButton(&OldController->RightShoulder1, &NewController->RightShoulder1, Dualshock4Data.R1);
                        ProcessButton(&OldController->LeftShoulder2, &NewController->LeftShoulder2, Dualshock4Data.L2);
                        ProcessButton(&OldController->RightShoulder2, &NewController->RightShoulder2, Dualshock4Data.R2);
                        ProcessButton(&OldController->Select, &NewController->Select, Dualshock4Data.Share);
                        ProcessButton(&OldController->Start, &NewController->Start, Dualshock4Data.Options);
                        ProcessButton(&OldController->LeftClick, &NewController->LeftClick, Dualshock4Data.L3);
                        ProcessButton(&OldController->RightClick, &NewController->RightClick, Dualshock4Data.R3);
                        ProcessButton(&OldController->Power, &NewController->Power, Dualshock4Data.PS);
                        ProcessButton(&OldController->CenterClick, &NewController->CenterClick, Dualshock4Data.Touchpad);
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
                                NewController->LeftStickX = 0.0f;
                                NewController->LeftStickY = 1.0f;
                            } break;

                            case 4500:
                            {
                                NewController->LeftStickX = 1.0f;
                                NewController->LeftStickY = 1.0f;
                            } break;

                            case 9000:
                            {
                                NewController->LeftStickX = 1.0f;
                                NewController->LeftStickY = 0.0f;
                            } break;

                            case 13500:
                            {
                                NewController->LeftStickX = 1.0f;
                                NewController->LeftStickY = -1.0f;
                            } break;

                            case 18000:
                            {
                                NewController->LeftStickX = 0.0f;
                                NewController->LeftStickY = -1.0f;
                            } break;

                            case 22500:
                            {
                                NewController->LeftStickX = -1.0f;
                                NewController->LeftStickY = -1.0f;
                            } break;

                            case 27000:
                            {
                                NewController->LeftStickX = -1.0f;
                                NewController->LeftStickY = 0.0f;
                            } break;

                            case 31500:
                            {
                                NewController->LeftStickX = -1.0f;
                                NewController->LeftStickY = 1.0f;
                            } break;

                            InvalidDefaultCase;
                        }
                    }

                    Assert(NewController->LeftStickX >= -1.0f);
                    Assert(NewController->LeftStickX <= 1.0f);
                    Assert(NewController->LeftStickY >= -1.0f);
                    Assert(NewController->LeftStickY <= 1.0f);
                    Assert(NewController->RightStickX >= -1.0f);
                    Assert(NewController->RightStickX <= 1.0f);
                    Assert(NewController->RightStickY >= -1.0f);
                    Assert(NewController->RightStickY <= 1.0f);
                    Assert(NewController->LeftTrigger >= 0.0f);
                    Assert(NewController->LeftTrigger <= 1.0f);
                    Assert(NewController->RightTrigger >= 0.0f);
                    Assert(NewController->RightTrigger <= 1.0f);
                }
            }
        } break;

        case InputDeviceType_N64:
        {
            LPDIRECTINPUTDEVICE8A N64Controller = Controller.Device;
            if(N64Controller)
            {
                n64_input N64Data = {};
                HRESULT DataFetchResult = N64Controller->GetDeviceState(sizeof(N64Data), &N64Data);
                if(SUCCEEDED(DataFetchResult))
                {
                    // NOTE(chris): Analog sticks
                    {
                        N64Data.StickY = 0xFFFF - N64Data.StickY;
                        r32 Center = 0xFFFF / 2.0f;
                        s32 Deadzone = 1500;
                        
                        NewController->LeftStickX = 0.0f;
                        if(N64Data.StickX < (Center - Deadzone))
                        {
                            NewController->LeftStickX = (r32)(-(Center - Deadzone) + N64Data.StickX) / (r32)(Center - Deadzone);
                        }
                        else if(N64Data.StickX > (Center + Deadzone))
                        {
                            NewController->LeftStickX = (r32)(N64Data.StickX - Center - Deadzone) / (r32)(Center - Deadzone);
                        }

                        NewController->LeftStickY = 0.0f;
                        if(N64Data.StickY < (Center - Deadzone))
                        {
                            NewController->LeftStickY = (r32)(-(Center - Deadzone) + N64Data.StickY) / (r32)(Center - Deadzone);
                        }
                        else if(N64Data.StickY > (Center + Deadzone))
                        {
                            NewController->LeftStickY = (r32)(N64Data.StickY - Center - Deadzone) / (r32)(Center - Deadzone);
                        }

                        NewController->RightStickX = 0.0f;
                        if(N64Data.CLeft)
                        {
                            NewController->RightStickX -= 1.0f;
                        }
                        if(N64Data.CRight)
                        {
                            NewController->RightStickX += 1.0f;
                        }

                        NewController->RightStickY = 0.0f;
                        if(N64Data.CDown)
                        {
                            NewController->RightStickY -= 1.0f;
                        }
                        if(N64Data.CUp)
                        {
                            NewController->RightStickY += 1.0f;
                        }
                    }

                    // NOTE(chris): Buttons
                    {
                        ProcessButton(&OldController->ActionLeft, &NewController->ActionLeft,
                                         N64Data.B);
                        ProcessButton(&OldController->ActionDown, &NewController->ActionDown,
                                         N64Data.A);
                        ProcessButton(&OldController->LeftShoulder1, &NewController->LeftShoulder1,
                                         N64Data.L|N64Data.Z);
                        ProcessButton(&OldController->RightShoulder1, &NewController->RightShoulder1,
                                         N64Data.R);
                        ProcessButton(&OldController->Start, &NewController->Start, N64Data.Start);
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
                                NewController->LeftStickX = 0.0f;
                                NewController->LeftStickY = 1.0f;
                            } break;

                            case 4500:
                            {
                                NewController->LeftStickX = 1.0f;
                                NewController->LeftStickY = 1.0f;
                            } break;

                            case 9000:
                            {
                                NewController->LeftStickX = 1.0f;
                                NewController->LeftStickY = 0.0f;
                            } break;

                            case 13500:
                            {
                                NewController->LeftStickX = 1.0f;
                                NewController->LeftStickY = -1.0f;
                            } break;

                            case 18000:
                            {
                                NewController->LeftStickX = 0.0f;
                                NewController->LeftStickY = -1.0f;
                            } break;

                            case 22500:
                            {
                                NewController->LeftStickX = -1.0f;
                                NewController->LeftStickY = -1.0f;
                            } break;

                            case 27000:
                            {
                                NewController->LeftStickX = -1.0f;
                                NewController->LeftStickY = 0.0f;
                            } break;

                            case 31500:
                            {
                                NewController->LeftStickX = -1.0f;
                                NewController->LeftStickY = 1.0f;
                            } break;

                            InvalidDefaultCase;
                        }
                    }

                    Assert(NewController->LeftStickX >= -1.0f);
                    Assert(NewController->LeftStickX <= 1.0f);
                    Assert(NewController->LeftStickY >= -1.0f);
                    Assert(NewController->LeftStickY <= 1.0f);
                    Assert(NewController->RightStickX >= -1.0f);
                    Assert(NewController->RightStickX <= 1.0f);
                    Assert(NewController->RightStickY >= -1.0f);
                    Assert(NewController->RightStickY <= 1.0f);
                    Assert(NewController->LeftTrigger >= 0.0f);
                    Assert(NewController->LeftTrigger <= 1.0f);
                    Assert(NewController->RightTrigger >= 0.0f);
                    Assert(NewController->RightTrigger <= 1.0f);
                }
            }
        } break;
    }
}
