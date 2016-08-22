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

global_variable LPDIRECTINPUTDEVICE8A GlobalDualshock4Controllers[4] = {};
global_variable direct_input_create *DirectInputCreate = 0;

internal void
InitializeInput()
{
    HMODULE DInputCode = LoadLibrary("dinput8.dll");
    if(DInputCode)
    {
        DirectInputCreate = (direct_input_create *)GetProcAddress(DInputCode, "DirectInput8Create");
    }
}

internal void
LatchControllers(HINSTANCE Instance, HWND Window)
{
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
                                GlobalDualshock4Controllers[0] = Device;
                            }
                        }
                    }
                }
            }
        }
    }
}

inline void
ProcessDualshock4Button(game_button *OldButton, game_button *NewButton, u8 State)
{
    NewButton->EndedDown = State;
    NewButton->HalfTransitionCount = (OldButton->EndedDown == NewButton->EndedDown) ? 0 : 1;
}

internal void
ProcessControllerInput(u32 ControllerIndex, game_controller *OldController, game_controller *NewController)
{
    LPDIRECTINPUTDEVICE8A Dualshock4Controller = GlobalDualshock4Controllers[ControllerIndex];
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
                ProcessDualshock4Button(&OldController->ActionLeft, &NewController->ActionLeft, Dualshock4Data.Square);
                ProcessDualshock4Button(&OldController->ActionDown, &NewController->ActionDown, Dualshock4Data.Cross);
                ProcessDualshock4Button(&OldController->ActionRight, &NewController->ActionRight, Dualshock4Data.Circle);
                ProcessDualshock4Button(&OldController->ActionUp, &NewController->ActionUp, Dualshock4Data.Triangle);
                ProcessDualshock4Button(&OldController->LeftShoulder1, &NewController->LeftShoulder1, Dualshock4Data.L1);
                ProcessDualshock4Button(&OldController->RightShoulder1, &NewController->RightShoulder1, Dualshock4Data.R1);
                ProcessDualshock4Button(&OldController->LeftShoulder2, &NewController->LeftShoulder2, Dualshock4Data.L2);
                ProcessDualshock4Button(&OldController->RightShoulder2, &NewController->RightShoulder2, Dualshock4Data.R2);
                ProcessDualshock4Button(&OldController->Select, &NewController->Select, Dualshock4Data.Share);
                ProcessDualshock4Button(&OldController->Start, &NewController->Start, Dualshock4Data.Options);
                ProcessDualshock4Button(&OldController->LeftClick, &NewController->LeftClick, Dualshock4Data.L3);
                ProcessDualshock4Button(&OldController->RightClick, &NewController->RightClick, Dualshock4Data.R3);
                ProcessDualshock4Button(&OldController->Power, &NewController->Power, Dualshock4Data.PS);
                ProcessDualshock4Button(&OldController->CenterClick, &NewController->CenterClick, Dualshock4Data.Touchpad);
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
}
