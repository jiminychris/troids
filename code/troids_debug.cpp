/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

extern "C" DEBUG_COLLATE(DebugCollate)
{
    debug_frame *Frame = GlobalDebugState->Frames + GlobalDebugState->FrameIndex;
    transient_state *TranState = (transient_state *)GameMemory->TemporaryMemory;
    if(!GlobalDebugState->IsInitialized)
    {
        GlobalDebugState->IsInitialized = true;
    }

    if(!GlobalDebugState->Paused)
    {
        // TODO(chris): Handle events that cross a frame boundary (i.e. threads)
        u32 CodeBeginStackCount = 0;
        debug_event CodeBeginStack[MAX_DEBUG_EVENTS];
        for(u32 EventIndex = 0;
            EventIndex < GlobalDebugState->EventCount;
            ++EventIndex)
        {
            debug_event *Event = GlobalDebugState->Events + EventIndex;
            switch(Event->Type)
            {
                case(DebugEventType_CodeBegin):
                {
                    debug_event *StackEvent = CodeBeginStack + CodeBeginStackCount;
                    *StackEvent = *Event;
                    ++CodeBeginStackCount;
                } break;

                case(DebugEventType_CodeEnd):
                {
                    Assert(CodeBeginStackCount > 0);
                    --CodeBeginStackCount;
                    debug_event *BeginEvent = CodeBeginStack + CodeBeginStackCount;

                    debug_element *Element = PushStruct(&GlobalDebugState->Arena, debug_element);
                    if(Frame->LastElement)
                    {
                        Frame->LastElement = Frame->LastElement->Next = Element;
                    }
                    else
                    {
                        Frame->FirstElement = Element;
                        Frame->LastElement = Element;
                    }

                    Element->Line = BeginEvent->Line;
                    Element->BeginTicks = BeginEvent->Value_u64;
                    Element->EndTicks = Event->Value_u64;
                    Element->File = BeginEvent->File;
                    Element->Name = BeginEvent->Name;
                } break;

                case(DebugEventType_FrameMarker):
                {
                    Frame->ElapsedSeconds = Event->ElapsedSeconds;
                    Frame->EndTicks = Event->Value_u64;
                    GlobalDebugState->LastFrameIndex = GlobalDebugState->FrameIndex;

                    // NOTE(chris): Trick only works if MAX_DEBUG_FRAMES is a power of two.
                    GlobalDebugState->FrameIndex = (GlobalDebugState->FrameIndex+1)&(MAX_DEBUG_FRAMES-1);
                    Frame = GlobalDebugState->Frames + GlobalDebugState->FrameIndex;
                    Frame->BeginTicks = Event->Value_u64;
                    Frame->LastElement = 0;
                } break;
            }
        }
    }
    GlobalDebugState->EventCount = 0;

    u32 ColorIndex = 0;
    v4 Colors[7] =
    {
        V4(1, 0, 0, 1),
        V4(0, 1, 0, 1),
        V4(0, 0, 1, 1),
        V4(1, 1, 0, 1),
        V4(1, 0, 1, 1),
        V4(0, 1, 1, 1),
        V4(0.5, 0.5, 0.5, 1),
    };
    
#if TROIDS_PROFILE
    Assert(TranState->IsInitialized);
    render_buffer *RenderBuffer = &TranState->RenderBuffer;
    temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->RenderBuffer.Arena);
    u32 TextLength;
    char Text[256];
    text_layout Layout;
    Layout.Font = &GameMemory->DebugFont;
    Layout.Scale = 0.3f;
    Layout.P = V2(0, BackBuffer->Height - Layout.Font->Ascent*Layout.Scale);
    Layout.Color = V4(1, 1, 1, 1);
#if 1
    Frame = GlobalDebugState->Frames + GlobalDebugState->LastFrameIndex;
    TextLength = _snprintf_s(Text, sizeof(Text), "Frame time: %fms",
                             Frame->ElapsedSeconds*1000);
    PushText(RenderBuffer, &Layout, TextLength, Text);
    
    rectangle2 ButtonRect = TopLeftDim(V2(Layout.P.x, Layout.P.y+Layout.Font->Ascent*Layout.Scale),
                                       V2(100, Layout.Font->Height*Layout.Scale));
    TextLength = _snprintf_s(Text, sizeof(Text), "%s",
                             GlobalDebugState->Paused ? "Unpause" : "Pause");
    PushRectangle(RenderBuffer, ButtonRect, V4(1, 0, 1, 1));
    PushText(RenderBuffer, &Layout, TextLength, Text);
    if(Inside(ButtonRect, Input->MousePosition) && WentDown(Input->LeftMouse))
    {
        GlobalDebugState->Paused = !GlobalDebugState->Paused;
    }

    r32 TotalWidth = 1900;
    r32 TotalHeight = 300;
    v2 Margin = V2(10, 10 + Layout.Font->LineAdvance*Layout.Scale);
    rectangle2 ProfileRect = TopLeftDim(V2(Margin.x, (r32)BackBuffer->Height - Margin.y),
                                        V2(TotalWidth, TotalHeight));
    PushRectangle(RenderBuffer, ProfileRect, V4(0, 0, 0, 0.1f));
    r32 InverseTotalCycles = 1.0f / (Frame->EndTicks - Frame->BeginTicks);
    for(debug_element *Element = Frame->FirstElement;
        Element;
        Element = Element->Next)
    {
        u64 CyclesPassed = Element->EndTicks - Element->BeginTicks;

        r32 Left = ProfileRect.Min.x + (Element->BeginTicks-Frame->BeginTicks)*InverseTotalCycles*TotalWidth;
        r32 Width = CyclesPassed*InverseTotalCycles*TotalWidth;
        rectangle2 RegionRect = TopLeftDim(V2(Left, ProfileRect.Max.y), V2(Width, TotalHeight));
        PushRectangle(RenderBuffer, RegionRect, Colors[ColorIndex++]);
        TextLength = _snprintf_s(Text, sizeof(Text), "%s %s %u %lu",
                                 Element->Name, Element->File,
                                 Element->Line, CyclesPassed);
        Layout.P = V2(Left, ProfileRect.Max.y - Layout.Font->Ascent*Layout.Scale);
        if(Inside(RegionRect, Input->MousePosition))
        {
            PushText(RenderBuffer, &Layout, TextLength, Text);
        }
    }
#else
    for(u32 FrameIndex = 0;
        FrameIndex < MAX_DEBUG_FRAMES;
        ++FrameIndex)
    {
        if(FrameIndex == GlobalDebugState->FrameIndex)
        {
            // TODO(chris): This frame is being populated right now.
        }
        else
        {
            Frame = GlobalDebugState->Frames + FrameIndex;
            TextLength = _snprintf_s(Text, sizeof(Text), "Frame time: %fms",
                                     Frame->ElapsedSeconds*1000);
            PushText(RenderBuffer, &Layout, TextLength, Text);

            rectangle2 ProfileRect = TopLeftDim(V2(10, (r32)BackBuffer->Height - 10), V2(800, 300));
            for(debug_element *Element = Frame->FirstElement;
                Element;
                Element = Element->Next)
            {
                u64 CyclesPassed = Element->EndTicks - Element->BeginTicks;

                TextLength = _snprintf_s(Text, sizeof(Text), "%s %s %u %lu",
                                         Element->Name, Element->File,
                                         Element->Line, CyclesPassed);
                PushText(RenderBuffer, &Layout, TextLength, Text);
            }
        }
    }
#endif
    RenderBufferToBackBuffer(RenderBuffer, BackBuffer);
    EndTemporaryMemory(RenderMemory);

    // TODO(chris): This is for preventing recursive debug events. The debug system logs its own
    // draw events, which in turn get drawn and are logged again... etc. Replace this with a better
    // system. Or just ignore all render function logs.
    GlobalDebugState->EventCount = 0;
#endif
}
