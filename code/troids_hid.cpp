/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include "troids_input.h"

// NOTE(chris): This has become a matter of reverse-engineering. It seems like this will only be good
// for input via DirectInput, as DirectInput can't handle output and the HID output is poorly
// documented, and it may even be illegal for me to do this without licensing it.

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

global_variable HANDLE GlobalDualshock4Controllers[4] =
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

static unsigned int stbiw__crc32(unsigned char *buffer, int len)
{
   static unsigned int crc_table[256] =
   {
      0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
      0x0eDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
      0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
      0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
      0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
      0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
      0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
      0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
      0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
      0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
      0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
      0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
      0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
      0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
      0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
      0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
      0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
      0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
      0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
      0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
      0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
      0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
      0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
      0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
      0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
      0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
      0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
      0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
      0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
      0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
      0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
      0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
   };

   unsigned int crc = ~0u;
   int i;
   for (i=0; i < len; ++i)
      crc = (crc >> 8) ^ crc_table[buffer[i] ^ (crc & 0xff)];
   return ~crc;
}

internal void
LatchControllers(HINSTANCE Instance, HWND Window)
{
    // NOTE(chris): Maybe use this if we need to get battery info, set LED color?
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
                        GlobalDualshock4Controllers[0] = Device;

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
}

internal void
InitializeInput()
{
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
}

internal void
ProcessControllerInput(u32 ControllerIndex, game_controller *OldController, game_controller *NewController)
{
    game_controller *Controller = NewController;
    HANDLE Dualshock4Controller = GlobalDualshock4Controllers[ControllerIndex];
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
}
