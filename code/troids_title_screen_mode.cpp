/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

internal void
TitleScreenMode(game_memory *GameMemory, game_input *Input, renderer_state *RendererState)
{
    game_state *GameState = (game_state *)GameMemory->PermanentMemory;
    title_screen_state *State = &GameState->TitleScreenState;
    transient_state *TranState = (transient_state *)GameMemory->TemporaryMemory;
    render_buffer *RenderBuffer = &TranState->RenderBuffer;
    temporary_memory RenderMemory = BeginTemporaryMemory(&RenderBuffer->Arena);
    PushClear(RendererState, RenderBuffer, V3(0.0f, 0.0f, 0.0f));

    game_controller *Keyboard = &Input->Keyboard;
    game_controller *ShipController = Input->GamePads + 0;

    RenderBuffer->Projection = Projection_None;
    v2 Center = 0.5f*V2i(RenderBuffer->Width, RenderBuffer->Height);
    r32 FadeInDuration = 3.0f;
    r32 FlickerPeriod = 0.5f;

    r32 tTitle = (State->FadeInTicks*Input->dtForFrame) / FadeInDuration;
    r32 TitleAlpha = Cube(tTitle);
    r32 tFlicker = (State->FlickerTicks*Input->dtForFrame) / (2*FlickerPeriod);
    b32 FlickerOn = (tFlicker >= 0.5f);
    if(tTitle >= 1.0f)
    {
        ++State->FlickerTicks;
        if(tFlicker >= 1.0f)
        {
            State->FlickerTicks = 0;
        }
    }
    else
    {
        ++State->FadeInTicks;
    }

    text_layout Layout = {};
    Layout.Font = &GameMemory->Font;
    Layout.Color = V4(1, 1, 1, TitleAlpha);
    Layout.Scale = 1.0f;
    char TroidsText[] = "TROIDS";
    text_measurement TroidsMeasurement = DrawText(RenderBuffer, &Layout,
                                                  sizeof(TroidsText)-1, TroidsText,
                                                  DrawTextFlags_Measure);
    Layout.P = V2(Center.x, 0.65f*RenderBuffer->Height) + GetTightAlignmentOffset(TroidsMeasurement);
    DrawText(RenderBuffer, &Layout, sizeof(TroidsText)-1, TroidsText);

    if(FlickerOn)
    {
        Layout.Scale = 0.5f;
        // TODO(chris): Change this text for keyboard/controller?
        char PushStartText[] = "PUSH START";
        text_measurement PushStartMeasurement = DrawText(RenderBuffer, &Layout,
                                                         sizeof(PushStartText)-1, PushStartText,
                                                         DrawTextFlags_Measure);
        Layout.P = V2(Center.x, 0.55f*RenderBuffer->Height) + GetTightAlignmentOffset(PushStartMeasurement);
        DrawText(RenderBuffer, &Layout, sizeof(PushStartText)-1, PushStartText);
    }

    if(WentDown(ShipController->Start))
    {
        GameState->NextMode = GameMode_Play;
    }
    
    {
        TIMED_BLOCK("Render Game");
        RenderBufferToBackBuffer(RendererState, RenderBuffer, RenderFlags_UsePipeline);
    }
    EndTemporaryMemory(RenderMemory);
}
