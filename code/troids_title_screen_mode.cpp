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

    game_controller *ShipController = Input->Controllers + Input->MostRecentlyUsedController;

    RenderBuffer->Projection = Projection_None;
    v2 Center = 0.5f*V2i(RenderBuffer->Width, RenderBuffer->Height);
    r32 FadeInDuration = 3.0f;
    r32 FlickerPeriod = 0.5f;

    r32 tTitle = State->FadeInTimer / FadeInDuration;
    r32 tFlicker = State->FlickerTimer / (2*FlickerPeriod);
    b32 FlickerOn = (tFlicker >= 0.5f);
    if(tTitle >= 1.0f)
    {
        tTitle = 1.0f;
        State->FlickerTimer += Input->dtForFrame;
        if(tFlicker >= 1.0f)
        {
            State->FlickerTimer = 0.0f;
        }
    }
    else
    {
        State->FadeInTimer += Input->dtForFrame;
    }
    r32 TitleAlpha = State->PressedStart ? 1.0f : Cube(tTitle);

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

    if(State->PressedStart)
    {
        Layout.Scale = 0.5f;

        v4 SelectedColor = V4(1, 1, 1, 1);
        v4 NotSelectedColor = V4(0.2f, 0.2f, 0.2f, 1.0f);

        char JourneyText[] = "JOURNEY";
        u32 JourneyTextLength = sizeof(JourneyText) - 1;
        text_measurement JourneyMeasurement = DrawText(RenderBuffer, &Layout,
                                                         JourneyTextLength, JourneyText,
                                                         DrawTextFlags_Measure);
        Layout.P = V2(Center.x, 0.52f*RenderBuffer->Height) + GetTightAlignmentOffset(JourneyMeasurement);
        Layout.Color = State->Selection == TitleScreenSelection_Journey ? SelectedColor : NotSelectedColor;
        DrawText(RenderBuffer, &Layout, JourneyTextLength, JourneyText);
        
        char ArcadeText[] = "ARCADE";
        u32 ArcadeTextLength = sizeof(ArcadeText) - 1;
        
        text_measurement ArcadeMeasurement = DrawText(RenderBuffer, &Layout,
                                                         ArcadeTextLength, ArcadeText,
                                                         DrawTextFlags_Measure);
        Layout.P = V2(Center.x, 0.45f*RenderBuffer->Height) + GetTightAlignmentOffset(ArcadeMeasurement);
        Layout.Color = State->Selection == TitleScreenSelection_Arcade ? SelectedColor : NotSelectedColor;
        DrawText(RenderBuffer, &Layout, ArcadeTextLength, ArcadeText);
        
        char QuitText[] = "QUIT";
        u32 QuitTextLength = sizeof(QuitText) - 1;
        
        text_measurement QuitMeasurement = DrawText(RenderBuffer, &Layout,
                                                    QuitTextLength, QuitText,
                                                    DrawTextFlags_Measure);
        Layout.P = V2(Center.x, 0.38f*RenderBuffer->Height) + GetTightAlignmentOffset(QuitMeasurement);
        Layout.Color = State->Selection == TitleScreenSelection_Quit ? SelectedColor : NotSelectedColor;
        DrawText(RenderBuffer, &Layout, QuitTextLength, QuitText);

        if(!State->Debounce)
        {
            if(ShipController->LeftStick.y <= -0.5f)
            {
                State->Selection = (title_screen_selection)(State->Selection + 1);
                if(State->Selection == PlayType_Terminator)
                {
                    State->Selection = (title_screen_selection)0;
                }
                State->Debounce = true;
            }
            else if(ShipController->LeftStick.y >= 0.5f)
            {
                if(State->Selection == (title_screen_selection)0)
                {
                    State->Selection = (title_screen_selection)(PlayType_Terminator - 1);
                }
                else
                {
                    State->Selection = (title_screen_selection)(State->Selection - 1);
                }
                State->Debounce = true;
            }
        }
        else if(-0.5f < ShipController->LeftStick.y && ShipController->LeftStick.y < 0.5f)
        {
            State->Debounce = false;
        }
        
        if(WentDown(ShipController->ActionDown) || WentDown(ShipController->Start))
        {
            if(State->Selection == TitleScreenSelection_Quit)
            {
                Input->Quit = true;
            }
            else
            {
                GameState->NextMode = GameMode_Play;
            }
        }
    }
    else
    {
        if(FlickerOn)
        {
            Layout.Scale = 0.5f;
            // TODO(chris): Change this text for keyboard/controller?
            u32 PushStartTextLength;
            char *PushStartText;
            switch(ShipController->Type)
            {
                case ControllerType_Keyboard:
                {
                    char Text[] = "PRESS ENTER";
                    PushStartText = Text;
                    PushStartTextLength = sizeof(Text)-1;
                } break;

                case ControllerType_Dualshock4:
                {
                    char Text[] = "PRESS OPTIONS";
                    PushStartText = Text;
                    PushStartTextLength = sizeof(Text)-1;
                } break;

                case ControllerType_XboxOne:
                {
                    char Text[] = "PRESS MENU";
                    PushStartText = Text;
                    PushStartTextLength = sizeof(Text)-1;
                } break;

                case ControllerType_Xbox360:
                case ControllerType_N64:
                default:
                {
                    char Text[] = "PRESS START";
                    PushStartText = Text;
                    PushStartTextLength = sizeof(Text)-1;
                } break;
            }

            text_measurement PushStartMeasurement = DrawText(RenderBuffer, &Layout,
                                                             PushStartTextLength, PushStartText,
                                                             DrawTextFlags_Measure);
            Layout.P = V2(Center.x, 0.52f*RenderBuffer->Height) + GetTightAlignmentOffset(PushStartMeasurement);
            DrawText(RenderBuffer, &Layout, PushStartTextLength, PushStartText);
        }
        if(WentDown(ShipController->Start))
        {
            State->PressedStart = true;
        }
    }

    {
        TIMED_BLOCK("Render Game");
        RenderBufferToBackBuffer(RendererState, RenderBuffer, RenderFlags_UsePipeline);
    }
    
    {DEBUG_GROUP("Title Screen Mode");
        DEBUG_VALUE("Render Arena", TranState->RenderBuffer.Arena);
    }
    EndTemporaryMemory(RenderMemory);
}
