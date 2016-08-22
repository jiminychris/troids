#if !defined(TROIDS_INPUT_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

CREATE_GUID(GUID_DEVINTERFACE_HID, 0x4D1E55B2,0xF16F,0x11CF,0x88,0xCB,0x00,0x11,0x11,0x00,0x00,0x30);

internal void InitializeInput();
internal void LatchControllers(HINSTANCE Instance, HWND Window);
internal void ProcessControllerInput(u32 ControllerIndex, game_controller *OldController, game_controller *NewController);

#define TROIDS_INPUT_H
#endif
