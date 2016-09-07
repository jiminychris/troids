/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

struct debug_node_hash_result
{
    u32 NameLength;
    u32 HashValue;
};

internal debug_node_hash_result
HashNode(char *GUID)
{
    Assert(GUID);
    debug_node_hash_result Result = {};
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
        Result.HashValue += Result.HashValue*65521 + *At;
    }
    return(Result);
}

inline debug_node_hash_result
HashNode(debug_node *Node)
{
    Assert(Node);
    debug_node_hash_result Result = HashNode(Node->GUID);
    return(Result);
}

inline u32
GetNodeNameLength(debug_node *Node)
{
    u32 Result = HashNode(Node).NameLength;

    return(Result);
}

internal debug_node *
GetNode(u32 HashCount, debug_node **NodeHash, char *GUID, debug_event_type Type)
{
    debug_node *Result = 0;
    
    // TODO(chris): Pass in sub arena instead?
    memory_arena *Arena = &GlobalDebugState->Arena;

    u32 HashValue = HashNode(GUID).HashValue;
    u32 Index = HashValue & (HashCount - 1);
    Assert(Index < HashCount);

    debug_node *FirstInHash = NodeHash[Index];
    for(debug_node *Chain = FirstInHash;
        Chain;
        Chain = Chain->NextInHash)
    {
        if(StringsMatch(GUID, Chain->GUID))
        {
            Result = Chain;
            break;
        }
    }
    if(!Result)
    {
        if(GlobalDebugState->FirstFreeNode)
        {
            Result = GlobalDebugState->FirstFreeNode;
            GlobalDebugState->FirstFreeNode = Result->NextFree;
        }
        else
        {
            Result = PushStruct(Arena, debug_node, PushFlag_Zero);
        }
        Result->Type = Type;
        // TODO(chris): Does this need to be copied? Probably only if it's not a literal.
        Result->GUID = GUID;
        Result->NextInHash = FirstInHash;
        NodeHash[Index] = Result;
    }
    return(Result);
}

inline debug_node *
GetNode(u32 HashCount, debug_node **NodeHash, char *GUID)
{
    debug_node *Result = GetNode(HashCount, NodeHash, GUID, DebugEventType_Name);

    return(Result);
}

inline debug_node *
GetNode(u32 HashCount, debug_node **NodeHash, debug_event *Event)
{
    debug_node *Result = GetNode(HashCount, NodeHash, Event->Name, Event->Type);

    return(Result);
}

inline debug_node *
GetNode(debug_frame *Frame, char *GUID)
{
    debug_node *Result = GetNode(ArrayCount(Frame->NodeHash), Frame->NodeHash, GUID);

    return(Result);
}

inline debug_node *
GetNode(debug_frame *Frame, debug_event *Event)
{
    debug_node *Result = GetNode(ArrayCount(Frame->NodeHash), Frame->NodeHash, Event);

    return(Result);
}

inline debug_node *
GetNode(debug_event *Event)
{
    debug_node *Result = GetNode(ArrayCount(GlobalDebugState->NodeHash), GlobalDebugState->NodeHash, Event);

    return(Result);
}

internal void
DrawNodes(render_buffer *RenderBuffer, text_layout *Layout, debug_frame *Frame, debug_node *Node,
          game_input *Input)
{
    u32 TextLength;
    char Text[256];
    if(!Input->LeftMouse.EndedDown)
    {
        GlobalDebugState->HotNode = 0;
    }
    u32 NameLength = GetNodeNameLength(Node);
    switch(Node->Type)
    {
        // TODO(chris): Better way to do this?
        case DebugEventType_Summary:
        case DebugEventType_GroupBegin:
        {
            rectangle2 HitBox;
            if(Node->Type == DebugEventType_GroupBegin)
            {
                HitBox = DrawText(RenderBuffer, Layout, NameLength, Node->GUID);
            }
            else
            {
                TextLength = _snprintf_s(Text, sizeof(Text), "Frame time: %fms",
                                         Frame->ElapsedSeconds*1000);
                HitBox = DrawText(RenderBuffer, Layout, TextLength, Text);
            }
            if(Inside(HitBox, Input->MousePosition) && WentDown(Input->LeftMouse))
            {
                Node->Value_b32 = !Node->Value_b32;
            }
            b32 Expanded = Node->Value_b32;
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
            DrawText(RenderBuffer, Layout, NameLength, Node->Name);
        } break;

        case DebugEventType_FillBar:
        case DebugEventType_DebugEvents:
        {
            r32 Used, Max;
            char *Name;
            char DebugEventsName[] = "Debug Events";
            if(Node->Type == DebugEventType_DebugEvents)
            {
                Used = (r32)GlobalDebugState->EventCount;
                Max = (r32)ArrayCount(GlobalDebugState->Events);
                Name = DebugEventsName;
                NameLength = sizeof(DebugEventsName);
            }
            else
            {
                debug_node *FrameNode = GetNode(Frame, Node->GUID);
                Used = FrameNode->Value_v2.x;
                Max = FrameNode->Value_v2.y;
                Name = Node->Name;
            }

            r32 XInit = Layout->P.x;
            TextLength = _snprintf_s(Text, sizeof(Text), "%.*s: ", NameLength, Name);
            DrawText(RenderBuffer, Layout, TextLength, Text, DrawTextFlags_NoLineAdvance);
            DrawFillBar(RenderBuffer, Layout, Used, Max);
            Layout->P.x = XInit;
        } break;

        case DebugEventType_DebugMemory:
        case DebugEventType_memory_arena:
        {
            memory_arena *Arena;
            char *Name;
            char DebugArenaName[] = "Debug Arena";
            if(Node->Type == DebugEventType_DebugMemory)
            {
                Arena = &GlobalDebugState->Arena;
                Name = DebugArenaName;
                NameLength = sizeof(DebugArenaName);
            }
            else
            {
                debug_node *FrameNode = GetNode(Frame, Node->GUID);
                Arena = &FrameNode->Value_memory_arena;
                Name = Node->Name;
            }
            r32 XInit = Layout->P.x;
            TextLength = _snprintf_s(Text, sizeof(Text), "%.*s: ", NameLength, Name);
            DrawText(RenderBuffer, Layout, TextLength, Text, DrawTextFlags_NoLineAdvance);
            DrawFillBar(RenderBuffer, Layout, Arena->Used, Arena->Size);
            Layout->P.x = XInit;
        } break;

        case DebugEventType_v2:
        {
            debug_node *FrameNode = GetNode(Frame, Node->GUID);
            TextLength = _snprintf_s(Text, sizeof(Text), "%.*s: <%f, %f>", NameLength, Node->Name,
                                     FrameNode->Value_v2.x, FrameNode->Value_v2.y);
            DrawText(RenderBuffer, Layout, TextLength, Text);
        } break;

        case DebugEventType_b32:
        {
            debug_node *FrameNode = GetNode(Frame, Node->GUID);
            TextLength = _snprintf_s(Text, sizeof(Text), "%.*s: %s", NameLength, Node->Name,
                                     FrameNode->Value_b32 ? "true" : "false");
            DrawText(RenderBuffer, Layout, TextLength, Text);
        } break;

        case DebugEventType_r32:
        {
            debug_node *FrameNode = GetNode(Frame, Node->GUID);
            TextLength = _snprintf_s(Text, sizeof(Text), "%.*s: %f", NameLength, Node->Name,
                                     FrameNode->Value_r32);
            DrawText(RenderBuffer, Layout, TextLength, Text);
        } break;

        case DebugEventType_FrameTimeline:
        {
            r32 XInit = Layout->P.x;
            r32 Ascent = Layout->Font->Ascent*Layout->Scale;
            r32 Descent = (Layout->Font->Height - Layout->Font->Ascent)*Layout->Scale;
            r32 Height = Ascent + Descent;
            r32 LineAdvance = Layout->Font->LineAdvance*Layout->Scale;
            r32 TotalWidth = RenderBuffer->Width - XInit - 10.0f;
            r32 Width = TotalWidth / (r32)ArrayCount(GlobalDebugState->Frames);

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
            rectangle2 ButtonRect = DrawButton(RenderBuffer, Layout, TextLength, ButtonText,
                                               V4(0, 0.5f, 1, 1));
    
            if(Inside(ButtonRect, Input->MousePosition) && WentDown(Input->LeftMouse))
            {
                GlobalDebugState->Paused = !GlobalDebugState->Paused;
            }
            
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
                else if(FrameIndex == GlobalDebugState->CollatingFrameIndex)
                {
                    Color = V4(1, 0, 0, 1);
                }
                rectangle2 HitBox = PushRectangle(RenderBuffer,
                                                  V3(Layout->P.x, Layout->P.y - Descent, HUD_Z),
                                                  V2(Width, Height), Color);
                if((GlobalDebugState->HotNode == Node) && InsideX(HitBox, Input->MousePosition))
                {
                    if(FrameIndex != GlobalDebugState->CollatingFrameIndex)
                    {
                        GlobalDebugState->ViewingFrameIndex = FrameIndex;
                    }
                }
                else if(WentDown(Input->LeftMouse) && Inside(HitBox, Input->MousePosition))
                {
                    GlobalDebugState->HotNode = Node;
                    if(FrameIndex != GlobalDebugState->CollatingFrameIndex)
                    {
                        GlobalDebugState->ViewingFrameIndex = FrameIndex;
                    }
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
                    V4(0.5f, 0, 1, 1),
                    V4(0, 1, 1, 1),
                    V4(1, 0.5f, 0, 1),
                };
#if 1

            v2 TotalDim = V2(RenderBuffer->Width - Layout->P.x - 10.0f, 200);
            rectangle2 ProfileRect = TopLeftDim(V2(Layout->P.x, Layout->P.y), TotalDim);
            PushRectangle(RenderBuffer, V3(ProfileRect.Min, HUD_Z), TotalDim,
                          INVERTED_COLOR);
            r32 InverseTotalCycles = 1.0f / (Frame->EndTicks - Frame->BeginTicks);
            u32 Depth = 0;
            u32 MaxDepth = 1;
            b32 FinishedChildren = false;
            r32 LayerHeight = TotalDim.y/(MaxDepth + 1);
            r32 FrameRatePosition = Input->dtForFrame / Frame->ElapsedSeconds;
            if(FrameRatePosition < 1.0f)
            {
                v3 FrameRateMarkP = V3(ProfileRect.Min.x + FrameRatePosition*TotalDim.x,
                                       ProfileRect.Min.y - 10.0f,
                                       HUD_Z+2000);
                PushRectangle(RenderBuffer, FrameRateMarkP, V2(2.0f, TotalDim.y + 20.0f),
                              V4(1, 0, 1, 1));
            }
            for(profiler_element *Element = Frame->FirstElement;
                Element;
                )
            {
                if(!FinishedChildren && Element->Child && (Depth < MaxDepth))
                {
                    Element = Element->Child;
                    ++Depth;
                }
                else
                {
                    r32 Height;
                    if(!Depth && !Element->Child)
                    {
                        Height = TotalDim.y;
                    }
                    else
                    {
                        Height = LayerHeight;
                    }
                    u64 CyclesPassed = Element->EndTicks - Element->BeginTicks;

                    r32 Left = ProfileRect.Min.x + (Element->BeginTicks-Frame->BeginTicks)*InverseTotalCycles*TotalDim.x;
                    r32 Width = CyclesPassed*InverseTotalCycles*TotalDim.x;
                    v2 RegionDim = V2(Width, Height);
                    rectangle2 RegionRect = MinDim(V2(Left, ProfileRect.Min.y + Depth*Height),
                                                   RegionDim);
                    PushRectangle(RenderBuffer, V3(RegionRect.Min, HUD_Z+1000), RegionDim,
                                  Colors[ColorIndex++]);
                    if(ColorIndex == ArrayCount(Colors))
                    {
                        ColorIndex = 0;
                    }
                    TextLength = _snprintf_s(Text, sizeof(Text), "%s %s %u %lu",
                                             Element->Name, Element->File,
                                             Element->Line, CyclesPassed);
                    Layout->P = V2(Left, RegionRect.Max.y - Layout->Font->Ascent*Layout->Scale);
                    if(Inside(RegionRect, Input->MousePosition))
                    {
                        DrawText(RenderBuffer, Layout, TextLength, Text);
                    }
                    
                    if(Element->Next)
                    {
                        FinishedChildren = false;
                        Element = Element->Next;
                    }
                    else
                    {
                        Element = Element->Parent;
                        FinishedChildren = true;
                        --Depth;
                    }

                }
            }

            // TODO(chris): Code block hierarchy!
#else
            for(u32 FrameIndex = 0;
                FrameIndex < MAX_DEBUG_FRAMES;
                ++FrameIndex)
            {
                if(FrameIndex == GlobalDebugState->CollatingFrameIndex)
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
            Layout->P = V2(ProfileRect.Min.x, ProfileRect.Min.y - Layout->Font->Ascent*Layout->Scale);
        } break;
    }
    if(Node->Next)
    {
        DrawNodes(RenderBuffer, Layout, Frame, Node->Next, Input);
    }
}

inline void
LinkNode(debug_node *Node, debug_node **Prev, debug_node **GroupBeginStack, u32 GroupBeginStackCount)
{
    if(GroupBeginStackCount)
    {
        debug_node *Parent = GroupBeginStack[GroupBeginStackCount-1];
        if(!Parent->Child)
        {
            Parent->Child = Node;
        }
    }
    if(*Prev) (*Prev)->Next = Node;
    *Prev = Node;
    Node->Next = Node->Child = 0;

}

extern "C" DEBUG_COLLATE(DebugCollate)
{
    BEGIN_TIMED_BLOCK(DEBUGCollation);
    debug_frame *Frame = GlobalDebugState->Frames + GlobalDebugState->CollatingFrameIndex;
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
    
    u32 GroupBeginStackCount = 0;
    debug_node *GroupBeginStack[MAX_DEBUG_EVENTS];
    debug_node *PrevNode = &GlobalDebugState->NodeSentinel;

    for(u32 EventIndex = 0;
        EventIndex < GlobalDebugState->EventCount;
        ++EventIndex)
    {
        debug_event *Event = GlobalDebugState->Events + EventIndex;
        switch(Event->Type)
        {
            case(DebugEventType_CodeBegin):
            {
                if(!GlobalDebugState->Paused)
                {
                    profiler_element *Element = GlobalDebugState->FirstFreeProfilerElement;
                    if(Element)
                    {
                        GlobalDebugState->FirstFreeProfilerElement = Element->NextFree;
                    }
                    else
                    {
                        Element = PushStruct(DebugArena, profiler_element);
                    }
                    
                    Element->Line = Event->Line;
                    Element->BeginTicks = Event->Value_u64;
                    Element->EndTicks = 0;
                    Element->File = Event->File;
                    Element->Name = Event->Name;
                    Element->Next = 0;
                    Element->Child = 0;

                    if(!Frame->CurrentElement)
                    {
                        Frame->FirstElement = Element;
                    }
                    else if(Frame->CurrentElement->EndTicks)
                    {
                        Frame->CurrentElement->Next = Element;
                        Element->Parent = Frame->CurrentElement->Parent;
                    }
                    else
                    {
                        Frame->CurrentElement->Child = Element;
                        Element->Parent = Frame->CurrentElement;
                    }
                    Frame->CurrentElement = Element;
                }
            } break;

            case(DebugEventType_CodeEnd):
            {
                if(!GlobalDebugState->Paused)
                {
                    Assert(Frame->CurrentElement);
                    if(Frame->CurrentElement->EndTicks)
                    {
                        Frame->CurrentElement = Frame->CurrentElement->Parent;
                    }
                    Frame->CurrentElement->EndTicks = Event->Value_u64;
                }
            } break;

            case(DebugEventType_FrameMarker):
            {
                if(!GlobalDebugState->Paused)
                {
                    Frame->ElapsedSeconds = Event->ElapsedSeconds;
                    Frame->EndTicks = Event->Value_u64;
                    GlobalDebugState->ViewingFrameIndex = GlobalDebugState->CollatingFrameIndex;

                    // NOTE(chris): Trick only works if MAX_DEBUG_FRAMES is a power of two.
                    GlobalDebugState->CollatingFrameIndex = (GlobalDebugState->CollatingFrameIndex+1)&(MAX_DEBUG_FRAMES-1);
                    Frame = GlobalDebugState->Frames + GlobalDebugState->CollatingFrameIndex;
                    for(u32 NodeIndex = 0;
                        NodeIndex < ArrayCount(Frame->NodeHash);
                        ++NodeIndex)
                    {
                        for(debug_node *Chain = Frame->NodeHash[NodeIndex];
                            Chain;
                            )
                        {
                            debug_node *Next = Chain->NextInHash;
                            Chain->NextFree = GlobalDebugState->FirstFreeNode;
                            GlobalDebugState->FirstFreeNode = Chain;
                            Chain = Next;
                        }
                        Frame->NodeHash[NodeIndex] = 0;
                    }
                    for(profiler_element *Element = Frame->FirstElement;
                        Element;
                        )
                    {
                        if(Element->Child)
                        {
                            Element = Element->Child;
                        }
                        else
                        {
                            profiler_element *Next = Element->Next;
                            profiler_element *Parent = Element->Parent;
                            Element->NextFree = GlobalDebugState->FirstFreeProfilerElement;
                            GlobalDebugState->FirstFreeProfilerElement = Element;
                            if(Next)
                            {
                                Element = Next;
                            }
                            else
                            {
                                Element = Parent;
                                if(Parent)
                                {
                                    Parent->Child = 0;
                                }
                            }
                        }
                    }
                    Frame->BeginTicks = Event->Value_u64;
                    Frame->CurrentElement = 0;
                }
                GroupBeginStackCount = 0;
                PrevNode = &GlobalDebugState->NodeSentinel;
            } break;

            case(DebugEventType_GroupBegin):
            case(DebugEventType_Summary):
            {
                debug_node *Node = GetNode(Event);
                LinkNode(Node, &PrevNode, GroupBeginStack, GroupBeginStackCount);
                PrevNode = 0;

                Assert(GroupBeginStackCount < ArrayCount(GroupBeginStack));
                GroupBeginStack[GroupBeginStackCount++] = Node;
            } break;

            case(DebugEventType_GroupEnd):
            {
                Assert(GroupBeginStackCount);
                PrevNode = GroupBeginStack[--GroupBeginStackCount];
            } break;

            case(DebugEventType_FillBar):
            {
                debug_node *Node = GetNode(Event);
                LinkNode(Node, &PrevNode, GroupBeginStack, GroupBeginStackCount);
                debug_node *FrameNode = GetNode(Frame, Event);
                FrameNode->Value_v2 = Event->Value_v2;
            } break;

            COLLATE_BLANK_TYPES(Event, PrevNode, GroupBeginStackCount, GroupBeginStack);
            COLLATE_VALUE_TYPES(Frame, Event, PrevNode, GroupBeginStackCount, GroupBeginStack);
        }
    }

    Frame = GlobalDebugState->Frames + GlobalDebugState->ViewingFrameIndex;
    if(GlobalDebugState->NodeSentinel.Next)
    {
        text_layout Layout;
        Layout.Font = &GlobalDebugState->Font;
        Layout.Scale = 0.5f;
        Layout.P = V2(0, BackBuffer->Height - Layout.Font->Ascent*Layout.Scale);
        Layout.Color = V4(1, 1, 1, 1);
        DrawNodes(RenderBuffer, &Layout, Frame, GlobalDebugState->NodeSentinel.Next, Input);
    }

#if 0
    for(GlobalDebugState->EventCount = 0;
        GlobalDebugState->EventCount < CodeBeginStackCount;
        ++GlobalDebugState->EventCount)
    {
        GlobalDebugState->Events[GlobalDebugState->EventCount] =
            GlobalDebugState->Events[CodeBeginIndexStack[GlobalDebugState->EventCount]];
    }
#else
    GlobalDebugState->EventCount = 0;
#endif
    END_TIMED_BLOCK();

    BEGIN_TIMED_BLOCK(DEBUGRender);
    u32 EventCount = GlobalDebugState->EventCount;
    RenderBufferToBackBuffer(RenderBuffer, BackBuffer);
    // TODO(chris): This is for preventing recursive debug events. The debug system logs its own
    // draw events, which in turn get drawn and are logged again... etc. Replace this with a better
    // system. Or just ignore all render function logs.
    GlobalDebugState->EventCount = EventCount;
    END_TIMED_BLOCK();

    EndTemporaryMemory(RenderMemory);
#endif
}
