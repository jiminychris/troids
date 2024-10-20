/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

struct guid_hash_result
{
    u32 HashValue;

    u32 NameOffset;
    u32 NameLength;

    u32 FileOffset;
    u32 FileLength;

    u32 LineOffset;
    u32 LineLength;
};

internal guid_hash_result
HashGUID(char *GUID)
{
    Assert(GUID);
    guid_hash_result Result = {};
    Result.NameOffset = 0;
    u32 PipeCount = 0;
    char *At;
    for(At = GUID;
        *At;
        ++At)
    {
        if(*At == '|')
        {
            ++PipeCount;
            if(PipeCount == 1)
            {
                Result.NameLength = (u32)(At - GUID) - Result.NameOffset;
                Result.FileOffset = Result.NameOffset + Result.NameLength + 1;
            }
            if(PipeCount == 2)
            {
                Result.FileLength = (u32)(At - GUID) - Result.FileOffset;
                Result.LineOffset = Result.FileOffset + Result.FileLength + 1;
            }
        }
        Result.HashValue += Result.HashValue*65521 + *At;
    }
    Result.LineLength = (u32)(At - GUID) - Result.LineOffset;
    return(Result);
}

inline guid_hash_result
HashNode(debug_node *Node)
{
    Assert(Node);
    guid_hash_result Result = HashGUID(Node->GUID);
    return(Result);
}

inline u32
GetNodeNameLength(debug_node *Node)
{
    u32 Result = HashNode(Node).NameLength;

    return(Result);
}

internal debug_node *
GetNode(char *GUID, debug_event_type Type, debug_frame *Frame = 0)
{
    debug_node *Result = 0;

    u32 HashCount = ArrayCount(GlobalDebugState->NodeHash);
    debug_node **NodeHash = GlobalDebugState->NodeHash;
    if(Frame)
    {
        HashCount = ArrayCount(Frame->NodeHash);
        NodeHash = Frame->NodeHash;
    }
    
    // TODO(chris): Pass in sub arena instead?
    memory_arena *Arena = &GlobalDebugState->Arena;

    u32 HashValue = HashGUID(GUID).HashValue;
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
        Assert(GetStringFromHash(GUID) == GUID);
        Result->GUID = GUID;
        Result->NextInHash = FirstInHash;
        NodeHash[Index] = Result;
    }
    return(Result);
}

inline debug_node *
GetNode(char *GUID, debug_frame *Frame = 0)
{
    debug_node *Result = GetNode(GUID, DebugEventType_Name, Frame);

    return(Result);
}

inline debug_node *
GetNode(debug_event *Event, debug_frame *Frame = 0)
{
    debug_node *Result = GetNode(Event->GUID, Event->Type, Frame);

    return(Result);
}

internal void
DrawThread(render_buffer *RenderBuffer, text_layout *Layout, debug_thread *Thread,
           game_input *Input, v2 Dim, v2 P)
{
    b32 FinishedChildren = false;
    u32 Depth = 0;
    u32 MaxDepth = 0;
    r32 LayerHeight = Dim.y/(MaxDepth + 1);
    r32 InverseTotalCycles = 1.0f / (Thread->CurrentElement->EndTicks -
                                     Thread->CurrentElement->BeginTicks);
    u32 ColorIndex = 0;
    v4 ParentColors[] =
        {
            V4(0, 0.2f, 1, 1),
            V4(0, 0.4f, 1, 1),
            V4(0, 0.6f, 1, 1),
            V4(0, 0.8f, 1, 1),
        };
    v4 LeafColors[] =
        {
            V4(0.2f, 0, 0, 1),
            V4(0.4f, 0, 0, 1),
            V4(0.6f, 0, 0, 1),
            V4(0.8f, 0, 0, 1),
        };
    u32 TextLength;
    char Text[256];
    for(profiler_element *Element = Thread->CurrentElement->Child;
        Element && Element != Thread->CurrentElement;
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
                Height = Dim.y;
            }
            else
            {
                Height = LayerHeight;
            }
            u64 CyclesPassed = Element->EndTicks - Element->BeginTicks;

            r32 Left = (P.x +
                        ((Element->BeginTicks-Thread->CurrentElement->BeginTicks)*
                         InverseTotalCycles*Dim.x));
            r32 Width = CyclesPassed*InverseTotalCycles*Dim.x;
            v2 RegionDim = V2(Width, Height-2.0f);
            rectangle2 RegionRect = MinDim(V2(Left, P.y + Depth*Height),
                                           RegionDim);
            v4 *Colors = Element->Child ? ParentColors : LeafColors;
            PushRectangle(RenderBuffer, V3(RegionRect.Min, HUD_Z+1000), RegionDim,
                          Colors[ColorIndex++]);
            if(ColorIndex == ArrayCount(ParentColors))
            {
                ColorIndex = 0;
            }
            guid_hash_result GUIDHash = HashGUID(Element->GUID);
            char *ElementName = Element->GUID + GUIDHash.NameOffset;
            char *ElementFile = Element->GUID + GUIDHash.FileOffset;
            char *ElementLine = Element->GUID + GUIDHash.LineOffset;
            if(Element->Iterations > 1)
            {
                TextLength = snprintf(Text, sizeof(Text), "%.*s %.*s(%.*s) %llucy/%u=%llu",
                                         GUIDHash.NameLength, ElementName,
                                         GUIDHash.FileLength, ElementFile,
                                         GUIDHash.LineLength, ElementLine,
                                         CyclesPassed,
                                         Element->Iterations, CyclesPassed/Element->Iterations);
            }
            else
            {
                TextLength = snprintf(Text, sizeof(Text), "%.*s %.*s(%.*s) %llucy",
                                         GUIDHash.NameLength, ElementName,
                                         GUIDHash.FileLength, ElementFile,
                                         GUIDHash.LineLength, ElementLine,
                                         CyclesPassed);
            }
            Layout->P = V2(Left, RegionRect.Max.y - Layout->Font->Ascent*Layout->Scale);
            if(Inside(RegionRect, Input->MousePosition))
            {
                rectangle2 HoverRect = GetLineRect(
                    DrawText(RenderBuffer, Layout, TextLength, Text, DrawTextFlags_Measure));
                if(HoverRect.Max.x > RenderBuffer->Width)
                {
                    Layout->P.x = RenderBuffer->Width - GetDim(HoverRect).x;
                }
                DrawText(RenderBuffer, Layout, TextLength, Text);
                if(WentDown(Input->LeftMouse) && Element->Child)
                {
                    Thread->NextElement = Element;
                }
            }
                    
            if(Element->Next)
            {
                FinishedChildren = false;
                Element = Element->Next;
            }
            else
            {
                Element = Element->Parent;
                if(Element)
                {
                    FinishedChildren = true;
                    --Depth;
                }
            }
        }
    }
}

internal void
DrawProfiler(debug_frame *Frame, render_buffer *RenderBuffer, text_layout *Layout, game_input *Input)
{
    for(u32 ThreadIndex = 0;
        ThreadIndex < Frame->ThreadCount;
        ++ThreadIndex)
    {
        debug_thread *Thread = Frame->Threads + ThreadIndex;
        Thread->NextElement = Thread->CurrentElement;
    }
    u32 TextLength;
    // TODO(chris): More formatting stuff to stop needing to do this kind of thing?
    r32 PopX = Layout->P.x;
    char *ButtonText;
    if(GlobalDebugState->Paused)
    {
        char Unpause[] = "Unpause";
        ButtonText = Unpause;
        TextLength = sizeof(Unpause)-1;
    }
    else
    {
        char Pause[] = "Pause";
        ButtonText = Pause;
        TextLength = sizeof(Pause)-1;
    }
    r32 ButtonY = Layout->P.y;
    rectangle2 ButtonRect = DEBUGDrawButton(RenderBuffer, Layout, TextLength, ButtonText,
                                       V4(0, 0.5f, 1, 1), 2.0f);
    
    if(Inside(ButtonRect, Input->MousePosition) && WentDown(Input->LeftMouse))
    {
        GlobalDebugState->Paused = !GlobalDebugState->Paused;
    }

    Layout->P.y = ButtonY;
    Layout->P.x = ButtonRect.Max.x + 5.0f;
    char Top[] = "Top";
    ButtonRect = DEBUGDrawButton(RenderBuffer, Layout, sizeof(Top)-1, Top,
                            V4(0, 0.5f, 1, 1), 2.0f);
    if(Inside(ButtonRect, Input->MousePosition) && WentDown(Input->LeftMouse))
    {
        for(u32 ThreadIndex = 0;
            ThreadIndex < Frame->ThreadCount;
            ++ThreadIndex)
        {
            debug_thread *Thread = Frame->Threads + ThreadIndex;
            Thread->NextElement = &Thread->ProfilerSentinel;
        }
    }

    Layout->P.x = PopX;
    v2 TotalDim = V2(RenderBuffer->Width - Layout->P.x - 10.0f, 200);
    rectangle2 ProfileRect = TopLeftDim(V2(Layout->P.x, Layout->P.y), TotalDim);
    PushRectangle(RenderBuffer, V3(ProfileRect.Min, HUD_Z), TotalDim,
                  INVERTED_COLOR);
    r64 TicksToSeconds = Frame->ElapsedSeconds/(Frame->EndTicks - Frame->BeginTicks);
    r64 SecondsToTicks = 1.0f / TicksToSeconds;
    r64 FrameRateTickTarget = (r64)Frame->BeginTicks + Input->dtForFrame*SecondsToTicks;
#if 0
    r32 FrameRateRelativePosition = (r32)Unlerp((r64)Frame->CurrentElement->BeginTicks,
                                                FrameRateTickTarget,
                                                (r64)Frame->CurrentElement->EndTicks);
#else
    r32 FrameRateRelativePosition = (r32)Unlerp((r64)Frame->BeginTicks,
                                                FrameRateTickTarget,
                                                (r64)Frame->EndTicks);
#endif
            
    if(FrameRateRelativePosition < 1.0f)
    {
        v3 FrameRateMarkP = V3(ProfileRect.Min.x + FrameRateRelativePosition*TotalDim.x,
                               ProfileRect.Min.y - 10.0f,
                               HUD_Z+2000);
        PushRectangle(RenderBuffer, FrameRateMarkP, V2(2.0f, TotalDim.y + 20.0f),
                      V4(1, 0, 1, 1));
    }

    r32 ThreadHeight = TotalDim.y / (r32)Frame->ThreadCount;

    for(u32 ThreadIndex = 0;
        ThreadIndex < Frame->ThreadCount;
        ++ThreadIndex)
    {
        debug_thread *Thread = Frame->Threads + ThreadIndex;
        DrawThread(RenderBuffer, Layout, Thread, Input, V2(TotalDim.x, ThreadHeight),
        V2(ProfileRect.Min.x, ProfileRect.Min.y + ThreadIndex*ThreadHeight));
        Thread->CurrentElement = Thread->NextElement;
    }
    Layout->P = V2(PopX, ProfileRect.Min.y - Layout->Font->Ascent*Layout->Scale);
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
                HitBox = GetLineRect(DrawText(RenderBuffer, Layout, NameLength, Node->GUID));
            }
            else
            {
                TextLength = snprintf(Text, sizeof(Text), "Frame time: %fms",
                                         Frame->ElapsedSeconds*1000);
                HitBox = GetLineRect(DrawText(RenderBuffer, Layout, TextLength, Text));
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
                u32 Mask = ArrayCount(GlobalDebugState->Events)-1;
                u32 EventIndex = GlobalDebugState->EventCount & Mask;
                if(GlobalDebugState->EventStart < EventIndex)
                {
                    Used = (r32)(EventIndex - GlobalDebugState->EventStart);
                }
                else
                {
                    Used = (r32)(ArrayCount(GlobalDebugState->Events) -
                                 GlobalDebugState->EventStart + EventIndex);
                }
                Max = (r32)ArrayCount(GlobalDebugState->Events);
                Name = DebugEventsName;
                NameLength = sizeof(DebugEventsName);
            }
            else
            {
                debug_node *FrameNode = GetNode(Node->GUID, Frame);
                Used = FrameNode->Value_v2.x;
                Max = FrameNode->Value_v2.y;
                Name = Node->Name;
            }

            r32 XInit = Layout->P.x;
            TextLength = snprintf(Text, sizeof(Text), "%.*s: ", NameLength, Name);
            DrawText(RenderBuffer, Layout, TextLength, Text, DrawTextFlags_NoLineAdvance);
            DEBUGDrawFillBar(RenderBuffer, Layout, Used, Max);
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
                debug_node *FrameNode = GetNode(Node->GUID, Frame);
                Arena = &FrameNode->Value_memory_arena;
                Name = Node->Name;
            }
            r32 XInit = Layout->P.x;
            TextLength = snprintf(Text, sizeof(Text), "%.*s: ", NameLength, Name);
            DrawText(RenderBuffer, Layout, TextLength, Text, DrawTextFlags_NoLineAdvance);
            DEBUGDrawFillBar(RenderBuffer, Layout, Arena->Used, Arena->Size);
            Layout->P.x = XInit;
        } break;

        case DebugEventType_v2:
        {
            debug_node *FrameNode = GetNode(Node->GUID, Frame);
            TextLength = snprintf(Text, sizeof(Text), "%.*s: <%f, %f>", NameLength, Node->Name,
                                     FrameNode->Value_v2.x, FrameNode->Value_v2.y);
            DrawText(RenderBuffer, Layout, TextLength, Text);
        } break;

        case DebugEventType_v2i:
        {
            debug_node *FrameNode = GetNode(Node->GUID, Frame);
            TextLength = snprintf(Text, sizeof(Text), "%.*s: <%d, %d>", NameLength, Node->Name,
                                     FrameNode->Value_v2i.x, FrameNode->Value_v2i.y);
            DrawText(RenderBuffer, Layout, TextLength, Text);
        } break;

        case DebugEventType_b32:
        {
            debug_node *FrameNode = GetNode(Node->GUID, Frame);
            TextLength = snprintf(Text, sizeof(Text), "%.*s: %s", NameLength, Node->Name,
                                     FrameNode->Value_b32 ? "true" : "false");
            DrawText(RenderBuffer, Layout, TextLength, Text);
        } break;

        case DebugEventType_s32:
        {
            debug_node *FrameNode = GetNode(Node->GUID, Frame);
            TextLength = snprintf(Text, sizeof(Text), "%.*s: %d", NameLength, Node->Name,
                                     FrameNode->Value_s32);
            DrawText(RenderBuffer, Layout, TextLength, Text);
        } break;

        case DebugEventType_r32:
        {
            debug_node *FrameNode = GetNode(Node->GUID, Frame);
            TextLength = snprintf(Text, sizeof(Text), "%.*s: %f", NameLength, Node->Name,
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
            DrawProfiler(Frame, RenderBuffer, Layout, Input);
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

inline debug_thread *
GetThread(debug_frame *Frame, u64 ThreadID)
{
    debug_thread *Thread = 0;
    b32 Found = false;
    for(u32 ThreadIndex = 0;
        ThreadIndex < Frame->ThreadCount;
        ++ThreadIndex)
    {
        Thread = Frame->Threads + ThreadIndex;
        if(Thread->ID == ThreadID)
        {
            Found = true;
            break;
        }
    }
    if(!Found)
    {
        Assert(Frame->ThreadCount < ArrayCount(Frame->Threads));
        Thread = Frame->Threads + Frame->ThreadCount++;
        Thread->CollatingElement = Thread->CurrentElement = &Thread->ProfilerSentinel;
        Thread->ProfilerSentinel.EndTicks = 0;
        Thread->ID = ThreadID;
    }
    return(Thread);
}

extern "C" DEBUG_COLLATE(DebugCollate)
{
#if TROIDS_INTERNAL
    TIMED_FUNCTION();

    game_state *State = (game_state *)GameMemory->PermanentMemory;
    transient_state *TranState = (transient_state *)GameMemory->TemporaryMemory;
    if(!GlobalDebugState->IsInitialized)
    {
        GlobalDebugState->IsInitialized = true;
    }
    debug_frame *Frame = GlobalDebugState->Frames + GlobalDebugState->CollatingFrameIndex;
    memory_arena *DebugArena = &GlobalDebugState->Arena;
    Assert(TranState->IsInitialized);
    render_buffer *RenderBuffer = &TranState->RenderBuffer;
    RenderBuffer->Projection = Projection_None;
    temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->RenderBuffer.Arena);
    u32 EventIndex = GlobalDebugState->EventStart;
    // TODO(chris): Handle events that cross a frame boundary (i.e. threads)
    {
        TIMED_BLOCK("CollateEvents");
        u32 GroupBeginStackCount = 0;
        debug_node *GroupBeginStack[MAX_DEBUG_EVENTS];
        debug_node *PrevNode = &GlobalDebugState->NodeSentinel;

        u32 Mask = ArrayCount(GlobalDebugState->Events)-1;
        // NOTE(chris): IMPORTANT There must be no other threads working during this collation
        PlatformWaitForAllThreadWork();
        u32 EventEnd = GlobalDebugState->EventCount & Mask;
        for(;
            EventIndex != EventEnd;
            EventIndex = ((EventIndex+1) & Mask))
        {
            debug_event *Event = GlobalDebugState->Events + EventIndex;
            if(Event->Ignored) continue;
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
                    
                        Element->Iterations = Event->Iterations;
                        Element->BeginTicks = Event->Value_u64;
                        Element->EndTicks = 0;
                        Element->GUID = Event->GUID;
                        Element->Next = 0;
                        Element->Child = 0;

                        debug_thread *Thread = GetThread(Frame, Event->ThreadID);
                        if(Thread->CollatingElement->EndTicks)
                        {
                            Thread->CollatingElement->Next = Element;
                            Element->Parent = Thread->CollatingElement->Parent;
                        }
                        else
                        {
                            Thread->CollatingElement->Child = Element;
                            Element->Parent = Thread->CollatingElement;
                        }
                        Thread->CollatingElement = Element;
                    }
                } break;

                case(DebugEventType_CodeEnd):
                {
                    if(!GlobalDebugState->Paused)
                    {
                        for(u32 BackIndex = 0;
                            BackIndex < 5;
                            ++BackIndex)
                        {
                            u32 BackFrameIndex = (GlobalDebugState->CollatingFrameIndex - BackIndex)&(MAX_DEBUG_FRAMES-1);
                            debug_frame *BackFrame = GlobalDebugState->Frames + BackFrameIndex;
                            
                            debug_thread *Thread = GetThread(BackFrame, Event->ThreadID);
                            Assert(Thread->CollatingElement);

                            profiler_element *BeginElement = Thread->CollatingElement;
                            if(BeginElement != &Thread->ProfilerSentinel)
                            {
                                if(BeginElement->EndTicks)
                                {
                                    Assert(BeginElement->Parent);
                                    BeginElement = BeginElement->Parent;
                                }
                                if(StringsMatch(BeginElement->GUID, Event->GUID))
                                {
                                    BeginElement->EndTicks = Event->Value_u64;
                                    Thread->CollatingElement = BeginElement;
                                    break;
                                }
                            }
                        }
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
                        debug_frame *NewFrame = (GlobalDebugState->Frames +
                                                 GlobalDebugState->CollatingFrameIndex);
                        for(u32 NodeIndex = 0;
                            NodeIndex < ArrayCount(NewFrame->NodeHash);
                            ++NodeIndex)
                        {
                            for(debug_node *Chain = NewFrame->NodeHash[NodeIndex];
                                Chain;
                                )
                            {
                                debug_node *Next = Chain->NextInHash;
                                Chain->NextFree = GlobalDebugState->FirstFreeNode;
                                GlobalDebugState->FirstFreeNode = Chain;
                                Chain = Next;
                            }
                            NewFrame->NodeHash[NodeIndex] = 0;
                        }
                        for(u32 ThreadIndex = 0;
                            ThreadIndex < NewFrame->ThreadCount;
                            ++ThreadIndex)
                        {
                            debug_thread *Thread = NewFrame->Threads + ThreadIndex;
                            for(profiler_element *Element = Thread->ProfilerSentinel.Child;
                                Element && Element != &Thread->ProfilerSentinel;
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
                        }
                        NewFrame->ThreadCount = 0;
                        for(u32 ThreadIndex = 0;
                            ThreadIndex < Frame->ThreadCount;
                            ++ThreadIndex)
                        {
                            debug_thread *OldThread = Frame->Threads + ThreadIndex;
                            
                            debug_thread *NewThread = GetThread(NewFrame, OldThread->ID);

                            OldThread->CurrentElement->BeginTicks = Frame->BeginTicks;
                            OldThread->CurrentElement->EndTicks = Frame->EndTicks;
                        }
                        NewFrame->BeginTicks = Event->Value_u64;
                        Frame = NewFrame;
                    }
                    Assert(GroupBeginStackCount == 0);
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
                    debug_node *FrameNode = GetNode(Event, Frame);
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
            Layout.Assets = &TranState->Assets;
            Layout.Scale = 21.0f / Layout.Font->Height;
            Layout.DropShadow = true;
            Layout.P = V2(0, RendererState->BackBuffer.Height - Layout.Font->Ascent*Layout.Scale);
            Layout.Color = V4(1, 1, 1, 1);
#if TROIDS_DEBUG_DISPLAY
            DrawNodes(RenderBuffer, &Layout, Frame, GlobalDebugState->NodeSentinel.Next, Input);
#endif
        }
    }

    GlobalDebugState->EventStart = EventIndex;

#if 0
    PushBitmap(RenderBuffer, &State->StructuredArt, V3(0, 0, 0), V2(1, 0), V2(0, 1), V2(14, 14));
#endif

    BEGIN_TIMED_BLOCK(GUID, "DEBUGRender");
    RenderBufferToBackBuffer(RendererState, RenderBuffer);
    END_TIMED_BLOCK(GUID);

    EndTemporaryMemory(RenderMemory);
    RenderBuffer->Projection = RenderBuffer->DefaultProjection;
#endif
}
