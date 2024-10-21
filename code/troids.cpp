/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include <stdlib.h>
#include <stdio.h>

#include "troids.h"
#include "troids_physics.cpp"
#include "troids_render.cpp"
#include "troids_play_mode.cpp"
#include "troids_title_screen_mode.cpp"

inline b32
IsEOF(char *String)
{
    b32 Result = (*String == 0);
    return(Result);
}

inline char *
SkipWhitespace(char *String)
{
    char *Result = String;
    while(!IsEOF(Result) &&
          ((*Result == ' ') ||
           (*Result == '\n') ||
           (*Result == '\r') ||
           (*Result == '\t')))
    {
        ++Result;
    }
    return(Result);
}

inline char *
SkipLine(char *String)
{
    char *Result = String;
    while(!IsEOF(Result) && *Result++ != '\n');
    return(Result);
}

internal void
LoadAssetFile(game_assets *Assets, char *FileName, u32 Offset)
{
    // TODO(chris): Allocate memory from arena instead of dynamic platform alloc
    read_file_result ReadResult = PlatformReadFile(FileName, Offset);
    u8 *Cursor = (u8 *)ReadResult.Contents;
    
    Assets->FontCount = *((u32 *)Cursor);
    Cursor += sizeof(u32);
    
    Assets->Fonts = (loaded_font *)Cursor;
    Cursor += sizeof(loaded_font)*Assets->FontCount;

    Assets->BitmapCount = *((u32 *)Cursor);
    Cursor += sizeof(u32);
    
    Assets->Bitmaps = (tra_bitmap *)Cursor;
    Cursor += sizeof(tra_bitmap)*Assets->BitmapCount;

    Assets->Memory = Cursor;
}

internal loaded_obj
LoadObj(char *FileName, memory_arena *Arena)
{
    loaded_obj Result = {};
    Result.TextureVertexCount = 1;
    Result.VertexNormalCount = 1;
    Result.VertexCount = 1;
    Result.FaceCount = 1;
    // TODO(chris): Allocate memory from arena instead of dynamic platform alloc
    read_file_result ReadResult = PlatformReadFile(FileName, 0);

    char *At = (char *)ReadResult.Contents;
    // NOTE(chris): Just do two passes over the file for now.
    while(!IsEOF(At))
    {
        switch(*At++)
        {
            case 'v':
            {
                if(*At == 't')
                {
                    ++Result.TextureVertexCount;
                }
                else if(*At == 'n')
                {
                    ++Result.VertexNormalCount;
                }
                else
                {
                    ++Result.VertexCount;
                }
            } break;

            case 'f':
            {
                ++Result.FaceCount;
            } break;
        }
        At = SkipLine(At);
    }

    Result.Vertices = PushArray(Arena, Result.VertexCount, v4);
    Result.TextureVertices = PushArray(Arena, Result.TextureVertexCount, v3);
    Result.VertexNormals = PushArray(Arena, Result.VertexNormalCount, v3);
    Result.Faces = PushArray(Arena, Result.FaceCount, obj_face);

    v4 *Vertices = Result.Vertices + 1;
    v3 *TextureVertices = Result.TextureVertices + 1;
    v3 *VertexNormals = Result.VertexNormals + 1;
    obj_face *Faces = Result.Faces + 1;
    
    At = (char *)ReadResult.Contents;
    while(!IsEOF(At))
    {
        switch(*At++)
        {
            case 'v':
            {
                if(*At == 't')
                {
                    ++At;
                    TextureVertices->u = strtof(At, &At);
                    TextureVertices->v = strtof(At, &At);
                    // TODO(chris): Don't read optional w value
                    TextureVertices->w = strtof(At, &At);

                    ++TextureVertices;
                }
                else if(*At == 'n')
                {
                    ++At;
                    VertexNormals->x = strtof(At, &At);
                    VertexNormals->y = strtof(At, &At);
                    VertexNormals->z = strtof(At, &At);

                    ++VertexNormals;
                }
                else
                {
                    Vertices->x = strtof(At, &At);
                    Vertices->y = strtof(At, &At);
                    Vertices->z = strtof(At, &At);
                    // TODO(chris): Read optional w value
                    Vertices->w = 1.0f;

                    ++Vertices;
                }
            } break;

            case 'f':
            {
                Faces->VertexIndexA = strtoul(At, &At, 10);
                ++At;
                Faces->TextureVertexIndexA = strtoul(At, &At, 10);
                ++At;
                Faces->VertexNormalIndexA = strtoul(At, &At, 10);

                Faces->VertexIndexB = strtoul(At, &At, 10);
                ++At;
                Faces->TextureVertexIndexB = strtoul(At, &At, 10);
                ++At;
                Faces->VertexNormalIndexB = strtoul(At, &At, 10);

                Faces->VertexIndexC = strtoul(At, &At, 10);
                ++At;
                Faces->TextureVertexIndexC = strtoul(At, &At, 10);
                ++At;
                Faces->VertexNormalIndexC = strtoul(At, &At, 10);

                ++Faces;
            } break;

            case '#':
            {
                // NOTE(chris): This is a comment in the .obj file
                At = SkipLine(At);
            } break;

            case 'g':
            {
                // TODO(chris): Implement .obj groups
                At = SkipLine(At);
            } break;

            case 's':
            {
                // TODO(chris): Implement .obj smoothing
                At = SkipLine(At);
            } break;

            InvalidDefaultCase;
        }
        At = SkipWhitespace(At);
    }

    return(Result);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
#if TROIDS_INTERNAL
    GlobalDebugState = (debug_state *)GameMemory->DebugMemory;
#endif
    loaded_bitmap *BackBuffer = &RendererState->BackBuffer;
    TIMED_FUNCTION();
    game_state *State = (game_state *)GameMemory->PermanentMemory;
    PlatformReadFile = GameMemory->PlatformReadFile;
    PlatformPushThreadWork = GameMemory->PlatformPushThreadWork;
    PlatformWaitForAllThreadWork = GameMemory->PlatformWaitForAllThreadWork;

    SplitWorkIntoSquares(RendererState->RenderChunks, RendererState->RenderChunkCount,
                         RendererState->BackBuffer.Width,
                         RendererState->BackBuffer.Height,
                         0, 0);

    if(!State->IsInitialized)
    {
        State->IsInitialized = true;
        //State->Mode = State->NextMode = GameMode_Play;

        InitializeArena(&State->Arena,
                        GameMemory->PermanentMemorySize - sizeof(game_state),
                        (u8 *)GameMemory->PermanentMemory + sizeof(game_state));

        State->MetersToPixels = 3674.9418959066769192359305459154f;
    }

    transient_state *TranState = (transient_state *)GameMemory->TemporaryMemory;
    if(!TranState->IsInitialized)
    {
        InitializeArena(&TranState->Arena,
                        GameMemory->TemporaryMemorySize - sizeof(transient_state),
                        (u8 *)GameMemory->TemporaryMemory + sizeof(transient_state));

            
        TranState->Assets.Arena = SubArena(&TranState->Arena, Megabytes(0));
#if ONE_FILE
        LoadAssetFile(&TranState->Assets, 0, GameMemory->AssetOffset);
#else
        LoadAssetFile(&TranState->Assets, "assets.tra", 0);
#endif

        GameMemory->Font = TranState->Assets.Fonts[1];
#if TROIDS_INTERNAL
        GlobalDebugState->Font = TranState->Assets.Fonts[2];
#endif

        TranState->RenderBuffer.Arena = SubArena(&TranState->Arena, Megabytes(1));
        TranState->RenderBuffer.Width = BackBuffer->Width;
        TranState->RenderBuffer.Height = BackBuffer->Height;
        TranState->RenderBuffer.MetersToPixels = State->MetersToPixels;
        TranState->RenderBuffer.DefaultProjection = Projection_Perspective;
        TranState->RenderBuffer.Projection = TranState->RenderBuffer.DefaultProjection;
        TranState->RenderBuffer.CameraP = ZERO(v3);
        
//        State->HeadMesh = LoadObj("head.obj", &TranState->Arena);

        TranState->IsInitialized = true;
    }
    TranState->RenderBuffer.NearZ = 0.1f;

    DEBUG_SUMMARY();
    {DEBUG_GROUP("Global");
        {DEBUG_GROUP("Debug System");
            DEBUG_MEMORY();
            DEBUG_EVENTS();
        }   
        {DEBUG_GROUP("Profiler");
            DEBUG_FRAME_TIMELINE();
            DEBUG_PROFILER();
        }
        {DEBUG_GROUP("Controllers");
            {DEBUG_GROUP("Keyboard");
                LOG_CONTROLLER(&Input->Keyboard);
            }
            {DEBUG_GROUP("Game Pad 0");
                LOG_CONTROLLER(Input->GamePads + 0);
            }
            {DEBUG_GROUP("Game Pad 1");
                LOG_CONTROLLER(Input->GamePads + 1);
            }
            {DEBUG_GROUP("Game Pad 2");
                LOG_CONTROLLER(Input->GamePads + 2);
            }
            {DEBUG_GROUP("Game Pad 3");
                LOG_CONTROLLER(Input->GamePads + 3);
            }
        }
        {DEBUG_GROUP("Memory");
            DEBUG_VALUE("Permanent Arena", State->Arena);
            DEBUG_VALUE("Tran Arena", TranState->Arena);
            DEBUG_VALUE("Thread 0 String Arena", GlobalDebugState->ThreadStorage[0].StringArena);
            DEBUG_VALUE("Thread 1 String Arena", GlobalDebugState->ThreadStorage[1].StringArena);
            DEBUG_VALUE("Thread 2 String Arena", GlobalDebugState->ThreadStorage[2].StringArena);
            DEBUG_VALUE("Thread 3 String Arena", GlobalDebugState->ThreadStorage[3].StringArena);
            DEBUG_VALUE("Thread 4 String Arena", GlobalDebugState->ThreadStorage[4].StringArena);
            DEBUG_VALUE("Thread 5 String Arena", GlobalDebugState->ThreadStorage[5].StringArena);
            DEBUG_VALUE("Thread 6 String Arena", GlobalDebugState->ThreadStorage[6].StringArena);
            DEBUG_VALUE("Thread 7 String Arena", GlobalDebugState->ThreadStorage[7].StringArena);
        }
        {DEBUG_GROUP("Monitor");
            v2i Dimensions{RendererState->BackBuffer.Width, RendererState->BackBuffer.Height};
            DEBUG_VALUE("Dimensions", Dimensions);
        }
    }

    if(State->Mode != State->NextMode)
    {
        switch(State->NextMode)
        {
            case GameMode_TitleScreen:
            {
                State->TitleScreenState = ZERO(title_screen_state);
            } break;

            case GameMode_Play:
            {
                play_type PlayType = PlayType_Arcade;
                if(State->Mode == GameMode_TitleScreen)
                {
                    PlayType = (play_type)State->TitleScreenState.Selection;
                }
                State->PlayState = ZERO(play_state);
                State->PlayState.PlayType = PlayType;
            } break;
        }
        State->Mode = State->NextMode;
        State->Arena.Used = 0;
    }
    
    switch(State->Mode)
    {
        case GameMode_TitleScreen:
        {
            TitleScreenMode(GameMemory, Input, RendererState);
        } break;

        case GameMode_Play:
        {
            PlayMode(GameMemory, Input, RendererState);
        } break;
    }

    // NOTE(chris): This now happens while rendering the samples.
//    ClearAllBuffers(RendererState);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
#if 0
    r32 Frequency = 440.0f;
    u32 SamplesPerPeriod = (u32)((r32)SoundBuffer->SamplesPerSecond / Frequency + 0.5f);
    r32 HalfSamplesPerPeriod = 0.5f*SamplesPerPeriod;
    u32 SampleSize = SoundBuffer->Channels*SoundBuffer->BitsPerSample / 8;
    u32 BufferSizeInSamples = SoundBuffer->Size/SampleSize;

    // NOTE(chris): Assumes sample size
    s16 Amplitude = 400;
    s16 *Sample = (s16 *)SoundBuffer->Region1;
    for(u32 SampleIndex = 0;
        SampleIndex < SoundBuffer->Region1Size;
        SampleIndex += SampleSize)
    {
        s16 Value = (State->RunningSampleCount++ < HalfSamplesPerPeriod) ? Amplitude : -Amplitude;
        if(State->RunningSampleCount == SamplesPerPeriod)
        {
            State->RunningSampleCount = 0;
        }
        for(u8 Channel = 0;
            Channel < SoundBuffer->Channels;
            ++Channel)
        {
            *Sample++ = Value;
        }
    }
    Sample = (s16 *)SoundBuffer->Region2;
    for(u32 SampleIndex = 0;
        SampleIndex < SoundBuffer->Region2Size;
        SampleIndex += SampleSize)
    {
        s16 Value = (State->RunningSampleCount++ < HalfSamplesPerPeriod) ? Amplitude : -Amplitude;
        if(State->RunningSampleCount == SamplesPerPeriod)
        {
            State->RunningSampleCount = 0;
        }
        for(u8 Channel = 0;
            Channel < SoundBuffer->Channels;
            ++Channel)
        {
            *Sample++ = Value;
        }
    }
#endif
}

#if TROIDS_INTERNAL
#include "troids_debug.cpp"
#endif
