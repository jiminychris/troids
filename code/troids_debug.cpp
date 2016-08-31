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
    else if((Frame->CurrentNode->Type == DebugEventType_GroupBegin ||
             Frame->CurrentNode->Type == DebugEventType_Summary) &&
            !Frame->CurrentNode->Child)
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

struct debug_persistent_hash_result
{
    u32 NameLength;
    debug_persistent_event *Event;
};

internal debug_persistent_hash_result
GetPersistentEvent(char *GUID)
{
    // TODO(chris): Pass in sub arena instead?
    memory_arena *Arena = &GlobalDebugState->Arena;

    debug_persistent_hash_result Result = {};
    u32 HashValue = 0;
    u32 PipeCount = 0;
    for(char *At = GUID;
        *At;
        ++At)
    {
        if(*At == '|')
        {
            ++PipeCount;
            if(PipeCount == 1)
            {
                Result.NameLength = (u32)(At - GUID);
            }
        }
        HashValue += HashValue*65521 + *At;
    }
    u32 Index = HashValue & (ArrayCount(GlobalDebugState->PersistentEventHash) - 1);
    Assert(Index < ArrayCount(GlobalDebugState->PersistentEventHash));
    debug_persistent_event *FirstInHash = GlobalDebugState->PersistentEventHash[Index];
    for(debug_persistent_event *Chain = FirstInHash;
        Chain;
        Chain = Chain->NextInHash)
    {
        if(StringsMatch(GUID, Chain->GUID))
        {
            Result.Event = Chain;
            break;
        }
    }
    if(!Result.Event)
    {
        if(GlobalDebugState->FirstFreePersistentEvent)
        {
            Result.Event = GlobalDebugState->FirstFreePersistentEvent;
            GlobalDebugState->FirstFreePersistentEvent = Result.Event->NextFree;
        }
        else
        {
            Result.Event = PushStruct(Arena, debug_persistent_event, PushFlag_Zero);
        }
        // TODO(chris): Does this need to be copied? Probably only if it's not a literal.
        Result.Event->GUID = GUID;
        Result.Event->NextInHash = FirstInHash;
        GlobalDebugState->PersistentEventHash[Index] = Result.Event;
    }
    return(Result);
}

internal void
DrawNodes(render_buffer *RenderBuffer, text_layout *Layout, debug_frame *Frame, debug_node *Node,
          game_input *Input)
{
    u32 TextLength;
    char Text[256];
    switch(Node->Type)
    {

        case DebugEventType_Summary:
        case DebugEventType_GroupBegin:
        {
            rectangle2 HitBox;
            debug_persistent_event *Event;
            if(Node->Type == DebugEventType_GroupBegin)
            {
                debug_persistent_hash_result HashResult = GetPersistentEvent(Node->GUID);
                Event = HashResult.Event;
                HitBox = DrawText(RenderBuffer, Layout, HashResult.NameLength, Node->GUID);
            }
            else
            {
                debug_persistent_hash_result HashResult = GetPersistentEvent("Summary");
                Event = HashResult.Event;
                TextLength = _snprintf_s(Text, sizeof(Text), "Frame time: %fms",
                                         Frame->ElapsedSeconds*1000);
                HitBox = DrawText(RenderBuffer, Layout, TextLength, Text);
            }
        
            if(Inside(HitBox, Input->MousePosition) && WentDown(Input->LeftMouse))
            {
                Event->Value_b32 = !Event->Value_b32;
            }
            b32 Expanded = Event->Value_b32;
            if(Node->Child && Expanded)
            {
                r32 XPop = Layout->P.x;
                Layout->P.x += Layout->Font->Height*Layout->Scale;
                DrawNodes(RenderBuffer, Layout, Frame, Node->Child, Input);
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

        case DebugEventType_FrameTimeline:
        {
            r32 XInit = Layout->P.x;
            r32 Ascent = Layout->Font->Ascent*Layout->Scale;
            r32 Descent = (Layout->Font->Height - Layout->Font->Ascent)*Layout->Scale;
            r32 Height = Ascent + Descent;
            r32 LineAdvance = Layout->Font->LineAdvance*Layout->Scale;
            for(u32 FrameIndex = 0;
                FrameIndex < ArrayCount(GlobalDebugState->Frames);
                ++FrameIndex)
            {
                r32 Shade = FrameIndex&1 ? 0.75f : 0.5f;
                v4 Color = V4(Shade, Shade, Shade, 1);
                if(FrameIndex == GlobalDebugState->ViewingFrameIndex)
                {
                    Color = V4(0, 1, 0, 1);
                }
                else if(FrameIndex == GlobalDebugState->FrameIndex)
                {
                    Color = V4(1, 0, 0, 1);
                }
                r32 Width = 10.0f;
                rectangle2 HitBox = PushRectangle(RenderBuffer,
                                                  V3(Layout->P.x, Layout->P.y - Descent, HUD_Z),
                                                  V2(Width, Height), Color);
                if(Inside(HitBox, Input->MousePosition) && Input->LeftMouse.EndedDown)
                {
                    GlobalDebugState->ViewingFrameIndex = FrameIndex;
                }
                Layout->P.x += Width;
            }
            Layout->P.x = XInit;
            Layout->P.y -= LineAdvance;
        } break;

        case DebugEventType_Profiler:
        {
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

            v2 TotalDim = V2(1900, 300);
            rectangle2 ProfileRect = TopLeftDim(V2(Layout->P.x, Layout->P.y), TotalDim);
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
                Layout->P = V2(Left, ProfileRect.Max.y - Layout->Font->Ascent*Layout->Scale);
                if(Inside(RegionRect, Input->MousePosition))
                {
                    DrawText(RenderBuffer, Layout, TextLength, Text);
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
                    PushText(RenderBuffer, Layout, TextLength, Text);

                    rectangle2 ProfileRect = TopLeftDim(V2(10, (r32)BackBuffer->Height - 10), V2(800, 300));
                    for(debug_element *Element = Frame->FirstElement;
                        Element;
                        Element = Element->Next)
                    {
                        u64 CyclesPassed = Element->EndTicks - Element->BeginTicks;

                        TextLength = _snprintf_s(Text, sizeof(Text), "%s %s %u %lu",
                                                 Element->Name, Element->File,
                                                 Element->Line, CyclesPassed);
                        PushText(RenderBuffer, Layout, TextLength, Text);
                    }
                }
            }
#endif
            char *ButtonText;
            if(GlobalDebugState->Paused)
            {
                char Unpause[] = "Unpause";
                ButtonText = Unpause;
                TextLength = sizeof(Unpause);
            }
            else
            {
                char Pause[] = "Pause";
                ButtonText = Pause;
                TextLength = sizeof(Pause);
            }
            Layout->P = V2(ProfileRect.Min.x, ProfileRect.Min.y - Layout->Font->Ascent*Layout->Scale);
            rectangle2 ButtonRect = DrawButton(RenderBuffer, Layout, TextLength, ButtonText);
    
            if(Inside(ButtonRect, Input->MousePosition) && WentDown(Input->LeftMouse))
            {
                GlobalDebugState->Paused = !GlobalDebugState->Paused;
            }
        } break;
    }
    if(Node->Next)
    {
        DrawNodes(RenderBuffer, Layout, Frame, Node->Next, Input);
    }
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
                if(!GlobalDebugState->Paused)
                {
                    Assert(CodeBeginStackCount > 0);
                    --CodeBeginStackCount;
                    debug_event *BeginEvent = CodeBeginStack + CodeBeginStackCount;

                    // TODO(chris): This needs to get freed eventually.
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
                }
            } break;

            case(DebugEventType_FrameMarker):
            {
                if(!GlobalDebugState->Paused)
                {
                    Frame->ElapsedSeconds = Event->ElapsedSeconds;
                    Frame->EndTicks = Event->Value_u64;
                    GlobalDebugState->ViewingFrameIndex = GlobalDebugState->FrameIndex;

                    // NOTE(chris): Trick only works if MAX_DEBUG_FRAMES is a power of two.
                    GlobalDebugState->FrameIndex = (GlobalDebugState->FrameIndex+1)&(MAX_DEBUG_FRAMES-1);
                    Frame = GlobalDebugState->Frames + GlobalDebugState->FrameIndex;
                    Frame->BeginTicks = Event->Value_u64;
                    Frame->LastElement = 0;
                }
                Frame->CurrentNode = Frame->FirstNode =
                    AllocateDebugNode(Frame, DebugEventType_Summary);
            } break;

            case(DebugEventType_GroupBegin):
            case(DebugEventType_Name):
            {
                debug_node *Node = AllocateDebugNode(Frame, Event->Type, Event->Name);
            } break;

            case(DebugEventType_GroupEnd):
            {
                Assert(Frame->CurrentNode);
                Frame->CurrentNode = Frame->CurrentNode->Parent;
            } break;

            case(DebugEventType_Profiler):
            case(DebugEventType_FrameTimeline):
            {
                debug_node *Node = AllocateDebugNode(Frame, Event->Type);
            } break;

            COLLATE_DEBUG_TYPES(Frame, Event)
        }
    }
    GlobalDebugState->EventCount = 0;

    Frame = GlobalDebugState->Frames + GlobalDebugState->ViewingFrameIndex;
    if(Frame->FirstNode)
    {
        text_layout Layout;
        Layout.Font = &GlobalDebugState->Font;
        Layout.Scale = 0.5f;
        Layout.P = V2(0, BackBuffer->Height - Layout.Font->Ascent*Layout.Scale);
        Layout.Color = V4(1, 1, 1, 1);
        DrawNodes(RenderBuffer, &Layout, Frame, Frame->FirstNode, Input);
    }

    RenderBufferToBackBuffer(RenderBuffer, BackBuffer);
    // TODO(chris): This is for preventing recursive debug events. The debug system logs its own
    // draw events, which in turn get drawn and are logged again... etc. Replace this with a better
    // system. Or just ignore all render function logs.
    GlobalDebugState->EventCount = 0;

    EndTemporaryMemory(RenderMemory);
#endif
}
