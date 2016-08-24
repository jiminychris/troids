#if !defined(TROIDS_INPUT_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#define MAX_CONTROLLERS 4
CREATE_GUID(GUID_DEVINTERFACE_HID, 0x4D1E55B2,0xF16F,0x11CF,0x88,0xCB,0x00,0x11,0x11,0x00,0x00,0x30);
CREATE_GUID(GUID_DEVINTERFACE_USB_DEVICE, 0xA5DCBF10,0x6530,0x11D2,0x90,0x1F,0x00,0xC0,0x4F,0xB9,0x51,0xED);

internal void InitializeInput(HINSTANCE Instance);
internal void UnlatchControllers(HWND Window);
internal void LatchControllers(HWND Window);
internal void ProcessControllerInput(game_input *OldController, game_input *NewController);

#define TROIDS_INPUT_H
#endif
