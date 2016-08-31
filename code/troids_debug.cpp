/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

inline debug_node *
AllocateDebugNode(debug_frame *Frame, debug_event_type Type)
{
    memory_arena *DebugArena = &GlobalDebugState->Arena;
    debug_node *Node = PushStruct(DebugArena, debug_node);
    if(!Frame->CurrentNode)
    {
        Frame->FirstNode = Node;
        Node->Parent = 0;
    }
    else if(Frame->CurrentNode->Type == DebugEventType_GroupBegin && !Frame->CurrentNode->Child)
    {
        Frame->CurrentNode->Child = Node;
        Node->Parent = Frame->CurrentNode;
    }
    else
    {
        Frame->CurrentNode->Next = Node;
        Node->Parent = Frame->CurrentNode->Parent;
    }
    Node->Type = Type;

    Node->Next = Node->Child = 0;
    Frame->CurrentNode = Node;

    return(Node);
}

inline debug_node *
AllocateDebugNode(debug_frame *Frame, debug_event_type Type, char *GUID)
{
    debug_node *Node = AllocateDebugNode(Frame, Type);
    Node->GUID = GUID;

    return(Node);
}

internal void
DrawNodes(render_buffer *RenderBuffer, text_layout *Layout, debug_node *Node)
{
    u32 TextLength;
    char Text[256];
    switch(Node->Type)
    {
        case DebugEventType_GroupBegin:
        {
            TextLength = _snprintf_s(Text, sizeof(Text), "%s", Node->GUID);
            DrawText(RenderBuffer, Layout, TextLength, Text);
            // TODO(chris): Get hashed persistent event
            b32 Expanded = false;
            if(Node->Child && Expanded)
            {
                r32 XPop = Layout->P.x;
                Layout->P.x += Layout->Font->Height*Layout->Scale;
                DrawNodes(RenderBuffer, Layout, Node->Child);
                Layout->P.x = XPop;
            }
        } break;

        case DebugEventType_Name:
        {
            TextLength = _snprintf_s(Text, sizeof(Text), "%s", Node->Name);
            DrawText(RenderBuffer, Layout, TextLength, Text);
        } break;

        case DebugEventType_v2:
        {
            TextLength = _snprintf_s(Text, sizeof(Text), "%s: <%f, %f>", Node->Name,
                                     Node->Value_v2.x, Node->Value_v2.y);
            DrawText(RenderBuffer, Layout, TextLength, Text);
        } break;

        case DebugEventType_b32:
        {
            TextLength = _snprintf_s(Text, sizeof(Text), "%s: %s", Node->Name,
                                     Node->Value_b32 ? "true" : "false");
            DrawText(RenderBuffer, Layout, TextLength, Text);
        } break;
    }
    if(Node->Next)
    {
        DrawNodes(RenderBuffer, Layout, Node->Next);
    }

#if 0
    u32 ColorIndex = 0;
    v4 Colors[7] =
        {
            V4(1, 0, 0, 1),
            V4(0, 1, 0, 1),
            V4(0, 0, 1, 1),
            V4(1, 1, 0, 1),
            V4(1, 0, 1, 1),
            V4(0, 1, 1, 1),
            V4(1, 0.5, 0, 1),
        };
    
#if 1
    Frame = GlobalDebugState->Frames + GlobalDebugState->LastFrameIndex;
//    if(DEBUG_GROUP(Profiler))
    {
        TextLength = _snprintf_s(Text, sizeof(Text), "Frame time: %fms",
                                 Frame->ElapsedSeconds*1000);
        DrawText(RenderBuffer, &Layout, TextLength, Text);

        char *ButtonText;
        if(GlobalDebugState->Paused)
        {
            char Pause[] = "Pause";
            ButtonText = Pause;
            TextLength = sizeof(Pause);
        }
        else
        {
            char Unpause[] = "Unpause";
            ButtonText = Unpause;
            TextLength = sizeof(Unpause);
        }
        rectangle2 ButtonRect = DrawButton(RenderBuffer, &Layout, TextLength, ButtonText);
    
        if(Inside(ButtonRect, Input->MousePosition) && WentDown(Input->LeftMouse))
        {
            GlobalDebugState->Paused = !GlobalDebugState->Paused;
        }

        v2 TotalDim = V2(1900, 300);
        v2 Margin = V2(10, 10 + Layout.Font->LineAdvance*Layout.Scale);
        rectangle2 ProfileRect = TopLeftDim(V2(Margin.x, (r32)BackBuffer->Height - Margin.y),
                                            TotalDim);
        PushRectangle(RenderBuffer, V3(ProfileRect.Min, HUD_Z), TotalDim, V4(0, 0, 0, 0.1f));
        r32 InverseTotalCycles = 1.0f / (Frame->EndTicks - Frame->BeginTicks);
        for(profiler_element *Element = Frame->FirstElement;
            Element;
            Element = Element->Next)
        {
            u64 CyclesPassed = Element->EndTicks - Element->BeginTicks;

            r32 Left = ProfileRect.Min.x + (Element->BeginTicks-Frame->BeginTicks)*InverseTotalCycles*TotalDim.x;
            r32 Width = CyclesPassed*InverseTotalCycles*TotalDim.x;
            v2 RegionDim = V2(Width, TotalDim.y);
            rectangle2 RegionRect = TopLeftDim(V2(Left, ProfileRect.Max.y), RegionDim);
            PushRectangle(RenderBuffer, V3(RegionRect.Min, HUD_Z+1.0f), RegionDim, Colors[ColorIndex++]);
            TextLength = _snprintf_s(Text, sizeof(Text), "%s %s %u %lu",
                                     Element->Name, Element->File,
                                     Element->Line, CyclesPassed);
            Layout.P = V2(Left, ProfileRect.Max.y - Layout.Font->Ascent*Layout.Scale);
            if(Inside(RegionRect, Input->MousePosition))
            {
                DrawText(RenderBuffer, &Layout, TextLength, Text);
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
    }
#endif
}

extern "C" DEBUG_COLLATE(DebugCollate)
{
    debug_frame *Frame = GlobalDebugState->Frames + GlobalDebugState->FrameIndex;
    transient_state *TranState = (transient_state *)GameMemory->TemporaryMemory;
    if(!GlobalDebugState->IsInitialized)
    {
        GlobalDebugState->IsInitialized = true;
    }

#if TROIDS_PROFILE
    memory_arena *DebugArena = &GlobalDebugState->Arena;
    Assert(TranState->IsInitialized);
    render_buffer *RenderBuffer = &TranState->RenderBuffer;
    temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->RenderBuffer.Arena);
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

                profiler_element *Element = PushStruct(DebugArena, profiler_element);
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
                Frame->CurrentNode = Frame->FirstNode = 0;
            } break;

            case(DebugEventType_GroupBegin):
            {
                debug_node *Node = AllocateDebugNode(Frame, Event->Type, Event->Name);
            } break;

            case(DebugEventType_GroupEnd):
            {
                Assert(Frame->CurrentNode);
                Frame->CurrentNode = Frame->CurrentNode->Parent;
            } break;

            case(DebugEventType_Name):
            {
                debug_node *Node = AllocateDebugNode(Frame, Event->Type, Event->Name);
            } break;

            case(DebugEventType_Profiler):
            {
                debug_node *Node = AllocateDebugNode(Frame, Event->Type, Event->Name);
            } break;

            COLLATE_DEBUG_TYPES(Frame, Event)
        }
    }
    GlobalDebugState->EventCount = 0;

    Frame = GlobalDebugState->Frames + GlobalDebugState->LastFrameIndex;
    if(Frame->FirstNode)
    {
        text_layout Layout;
        Layout.Font = &GlobalDebugState->Font;
        Layout.Scale = 0.5f;
        Layout.P = V2(0, BackBuffer->Height - Layout.Font->Ascent*Layout.Scale);
        Layout.Color = V4(1, 1, 1, 1);
        DrawNodes(RenderBuffer, &Layout, Frame->FirstNode);
    }

    // TODO(chris): This is for preventing recursive debug events. The debug system logs its own
    // draw events, which in turn get drawn and are logged again... etc. Replace this with a better
    // system. Or just ignore all render function logs.
    GlobalDebugState->EventCount = 0;

    RenderBufferToBackBuffer(RenderBuffer, BackBuffer);
    EndTemporaryMemory(RenderMemory);
#endif
}
