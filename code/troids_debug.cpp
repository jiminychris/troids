/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

extern "C" DEBUG_COLLATE(DebugCollate)
{
#if TROIDS_PROFILE
    GlobalDebugState = (debug_state *)GameMemory->DebugMemory;
    transient_state *TranState = (transient_state *)GameMemory->TemporaryMemory;
    if(!GlobalDebugState->IsInitialized)
    {
        GlobalDebugState->IsInitialized = true;
    }
    loaded_font *Font = &GameMemory->DebugFont;
    Assert(TranState->IsInitialized);
    render_buffer *RenderBuffer = &TranState->RenderBuffer;
    temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->RenderBuffer.Arena);
    temporary_memory CollateMemory = BeginTemporaryMemory(&GlobalDebugState->Arena);
    
    u32 TextLength;
    char Text[256];
    
    u32 CodeBeginStackCount = 0;
    debug_event CodeBeginStack[MAX_DEBUG_EVENTS];
    
    u32 StringCount = 0;
    u32 StringLengths[MAX_DEBUG_EVENTS];

    memory_arena StringArena = SubArena(&GlobalDebugState->Arena, Megabytes(1));

    r32 FontScale = 0.3f;
    v2 P = V2(0, BackBuffer->Height - Font->Ascent*FontScale);
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
                u64 CyclesPassed = Event->Value_u64 - BeginEvent->Value_u64;

                memory_size Remaining = StringArena.Size-StringArena.Used;
                char *CurrentString = (char *)StringArena.Memory + StringArena.Used;
                TextLength = _snprintf_s(CurrentString, Remaining, _TRUNCATE, "%s %s %u %lu",
                                         BeginEvent->Name, BeginEvent->File,
                                         BeginEvent->Line, CyclesPassed);
                StringLengths[StringCount++] = TextLength;

                PushArray(&StringArena, TextLength, char);
            } break;

            case(DebugEventType_FrameMarker):
            {
                TextLength = _snprintf_s(Text, sizeof(Text), "Frame time: %fms",
                                         Event->Value_r32*1000);
                PushText(RenderBuffer, Font, TextLength, Text, &P, FontScale);
            } break;
        }
    }

    char *String = (char *)StringArena.Memory;
    for(u32 StringIndex = 0;
        StringIndex < StringCount;
        ++StringIndex)
    {
        u32 StringLength = StringLengths[StringIndex];
        PushText(RenderBuffer, Font, StringLength, String, &P, FontScale);
        String += StringLength;
    }
    GlobalDebugState->EventCount = 0;
    EndTemporaryMemory(CollateMemory);

    RenderBufferToBackBuffer(RenderBuffer, BackBuffer);
    EndTemporaryMemory(RenderMemory);
#endif
}
