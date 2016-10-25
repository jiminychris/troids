/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include "win32_troids.h"

inline HMONITOR
GetPrimaryMonitor()
{
    POINT Zero = {0, 0};
    HMONITOR Result = MonitorFromPoint(Zero, MONITOR_DEFAULTTOPRIMARY);
    return(Result);
}

inline void
ToggleFullscreen(HWND Window, rectangle2i MonitorRect)
{
    // NOTE(chris): Taken from Raymond Chen's blog.
    DWORD WindowStyle = GetWindowLong(Window, GWL_STYLE);
    if(WindowStyle & WS_OVERLAPPEDWINDOW)
    {
        if(GetWindowPlacement(Window, &PreviousWindowPlacement))
        {
            SetWindowLong(Window, GWL_STYLE, WindowStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         MonitorRect.Min.x, MonitorRect.Min.y,
                         MonitorRect.Max.x - MonitorRect.Min.x,
                         MonitorRect.Max.y - MonitorRect.Min.y,
                         SWP_NOOWNERZORDER|SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, WindowStyle|WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &PreviousWindowPlacement);
        SetWindowPos(Window, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_FRAMECHANGED);
    }
}

enum render_mode
{
    RenderMode_GDI,
    RenderMode_OpenGL,
};

global_variable render_mode GlobalRenderMode;

inline void
CopyBackBufferToWindow(HDC DeviceContext, win32_backbuffer *BackBuffer)
{
    TIMED_FUNCTION();
    if(GlobalRenderMode == RenderMode_GDI)
    {
        TIMED_BLOCK("StretchDIBits");
        StretchDIBits(DeviceContext,
                      0,
                      0,
                      GlobalBackBuffer.MonitorWidth,
                      GlobalBackBuffer.MonitorHeight,
                      0,
                      0,
                      GlobalBackBuffer.Width,
                      GlobalBackBuffer.Height,
                      GlobalBackBuffer.Memory,
                      &GlobalBackBuffer.Info,
                      DIB_RGB_COLORS,
                      SRCCOPY);
    }
    else
    {
        {
            TIMED_BLOCK("GLCommands");
            {
                TIMED_BLOCK("glViewport");
                glViewport(0, 0, BackBuffer->MonitorWidth, BackBuffer->MonitorHeight);
            }

            {
                TIMED_BLOCK("GL_TEXTURE_2D");
                {
                    TIMED_BLOCK("glBindTexture");
                    glBindTexture(GL_TEXTURE_2D, 1);
                }
                {
                    TIMED_BLOCK("glTexImage2D");
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BackBuffer->Pitch/sizeof(u32), BackBuffer->Height,
                                 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, BackBuffer->Memory);
                }
                {
                    TIMED_BLOCK("glTexParameteri(MAG)");
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                }
                {
                    TIMED_BLOCK("glTexParameteri(MIN)");
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                }

                {
                    TIMED_BLOCK("glEnable");
                    glEnable(GL_TEXTURE_2D);
                }
            }

            {
                TIMED_BLOCK("GL_TRIANGLES");
                glBegin(GL_TRIANGLES);

                // NOTE(chris): Lower triangle
                glTexCoord2f(0.0f, 0.0f);
                glVertex2f(-1.0f, -1.0f);

                glTexCoord2f(1.0f, 0.0f);
                glVertex2f(1.0f, -1.0f);

                glTexCoord2f(0.0f, 1.0f);
                glVertex2f(-1.0f, 1.0f);

                // NOTE(chris): Upper triangle
                glTexCoord2f(1.0f, 1.0f);
                glVertex2f(1.0f, 1.0f);

                glTexCoord2f(0.0f, 1.0f);
                glVertex2f(-1.0f, 1.0f);

                glTexCoord2f(1.0f, 0.0f);
                glVertex2f(1.0f, -1.0f);

                glEnd();
            }

            {
                TIMED_BLOCK("glBindTexture");
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }
        {
            TIMED_BLOCK("SwapBuffers");
            SwapBuffers(DeviceContext);
        }
    }
}

inline void
ProcessButtonInput(game_button *Button)
{
    Button->EndedDown = !Button->EndedDown;
    ++Button->HalfTransitionCount;
}

inline void
ProcessAnalogInput(r32 *AnalogInput, b32 WentDown, r32 Value)
{
    if(!WentDown)
    {
        Value = -Value;
    }
    *AnalogInput += Value;
}

LRESULT WindowProc(HWND Window,
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam)
{
    LRESULT Result = 0;
    switch(Message) 
    { 
        case WM_ACTIVATEAPP:
        {
        } break;

        case WM_CLOSE:
        case WM_QUIT:
        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT PaintStruct;
            HDC DeviceContext = BeginPaint(Window, &PaintStruct);
            CopyBackBufferToWindow(DeviceContext, &GlobalBackBuffer);
            EndPaint(Window, &PaintStruct);
        } break;

        case WM_DEVICECHANGE:
        {
            switch(WParam)
            {
                case DBT_DEVICEARRIVAL:
                {
                    DEV_BROADCAST_HDR *Header = (DEV_BROADCAST_HDR *)LParam;
                    if(Header)
                    {
                        switch(Header->dbch_devicetype)
                        {
                            case DBT_DEVTYP_DEVICEINTERFACE:
                            {
                                DEV_BROADCAST_DEVICEINTERFACE *Interface = (DEV_BROADCAST_DEVICEINTERFACE *)LParam;
                                if(IsEqualGUID(Interface->dbcc_classguid, GUID_DEVINTERFACE_HID))
                                {
                                    LatchGamePads(Window);
                                }
                            } break;
                        }
                    }
                } break;

                case DBT_DEVICEREMOVECOMPLETE:
                {
                    DEV_BROADCAST_HDR *Header = (DEV_BROADCAST_HDR *)LParam;
                    if(Header)
                    {
                        switch(Header->dbch_devicetype)
                        {
                            case DBT_DEVTYP_DEVICEINTERFACE:
                            {
                                DEV_BROADCAST_DEVICEINTERFACE *Interface = (DEV_BROADCAST_DEVICEINTERFACE *)LParam;
                                if(IsEqualGUID(Interface->dbcc_classguid, GUID_DEVINTERFACE_HID))
                                {
                                    UnlatchGamePads();
                                }
                            } break;
                        }
                    }
                    
                } break;

#if 0
                default:
                {
                    char Text[256];
                    _snprintf_s(Text, sizeof(Text),
                                "Something else happened: %d\n",
                                WParam);
                    OutputDebugStringA(Text);
                } break;
#endif
            }
        } break;

        case WM_MOUSEMOVE:
        {
            GlobalMousePosition = {(r32)(LParam & 0xFFFF),
                                   (r32)((LParam >> 16) & 0xFFFF)};
        } break;

        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
        {
            ProcessButtonInput(&GlobalLeftMouse);
        } break;

        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam); 
        } break;
    }
    return(Result);
}

internal read_file_result
Win32ReadFile(char *FileName, u32 Offset)
{
    read_file_result Result = {};

    HANDLE File = GlobalEXEFile;
    if(FileName)
    {
        File = CreateFile(FileName,
                          GENERIC_READ,
                          0, 0,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, 0);
    }
    
    if(INVALID_HANDLE_VALUE == File)
    {
        // TODO(chris): Logging
    }
    else
    {
        Result.ContentsSize = GetFileSize(File, 0) - Offset;
        Result.Contents = VirtualAlloc(0, Result.ContentsSize,
                                       MEM_COMMIT|MEM_RESERVE,
                                       PAGE_READWRITE);
        DWORD BytesRead;
        OVERLAPPED Overlapped = {};
        Overlapped.Offset = Offset;
        BOOL ReadFileResult = ReadFile(File, Result.Contents,
                                       (u32)Result.ContentsSize,
                                       &BytesRead, &Overlapped);

        Assert(ReadFileResult);
    }

    return(Result);
}

internal b32
InitializeOpenGL(HDC DeviceContext)
{
    b32 Result = false;
    HGLRC OpenGLRC = wglCreateContext(DeviceContext);
    if(wglMakeCurrent(DeviceContext, OpenGLRC))
    {
        GlobalRenderMode = RenderMode_OpenGL;
        char *Version = (char *)glGetString(GL_VERSION);
        wgl_swap_interval_ext *wglSwapIntervalEXT = (wgl_swap_interval_ext *)wglGetProcAddress("wglSwapIntervalEXT");
        if(wglSwapIntervalEXT)
        {
            if(wglSwapIntervalEXT(1))
            {
                Result = true;
            }
        }
    }
    else
    {
        GlobalRenderMode = RenderMode_GDI;
    }
    return(Result);
}

internal b32
FileExists(char *Path)
{
    b32 Result = (GetFileAttributes(Path) != INVALID_FILE_ATTRIBUTES);

    return(Result);
}

inline void
Win32PushThreadWork(thread_callback *Callback, void *Params,
                    thread_progress *Progress = 0)
{
    TIMED_FUNCTION();
    if(!Progress)
    {
        Progress = &GlobalNullProgress;
    }
    Progress->Finished = false;
    u32 WorkIndex = (++GlobalThreadQueueNextIndex &
                     (ArrayCount(GlobalThreadQueue)-1));
    thread_work *Work = GlobalThreadQueue + WorkIndex;
    while(!Work->Free);
    Work->Callback = Callback;
    Work->Params = Params;
    Work->Progress = Progress;
    Work->Free = false;
    ReleaseSemaphore(GlobalThreadQueueSemaphore, 1, 0);
}

DWORD WINAPI
ThreadProc(void *Parameter)
{
    thread_context *Context = (thread_context *)Parameter;
    DWORD Result = 0;

    while(!Result)
    {
        Context->Active = false;
        WaitForSingleObject(GlobalThreadQueueSemaphore, INFINITE);
        Context->Active = true;
        u32 WorkIndex = (InterlockedIncrement(&GlobalThreadQueueStartIndex) &
                         (ArrayCount(GlobalThreadQueue)-1));
        thread_work Work = GlobalThreadQueue[WorkIndex];
        GlobalThreadQueue[WorkIndex].Free = true;

        Work.Callback(Work.Params);
        Work.Progress->Finished = true;
    }

    return(Result);
}

inline void
Win32WaitForAllThreadWork()
{
    TIMED_FUNCTION();
    {
        b32 AllWorkFreed;
        do
        {
            AllWorkFreed = true;
            for(u32 WorkIndex = 0;
                WorkIndex < ArrayCount(GlobalThreadQueue);
                ++WorkIndex)
            {
                thread_work *Work = GlobalThreadQueue + WorkIndex;
                AllWorkFreed &= Work->Free;
            }
        } while(!AllWorkFreed);
    }

    {
        b32 AllThreadsInactive;
        do
        {
            AllThreadsInactive = true;
            for(u32 ThreadIndex = 0;
                ThreadIndex < GlobalThreadCount;
                ++ThreadIndex)
            {
                thread_context *Context = GlobalThreadContext + ThreadIndex;
                AllThreadsInactive &= !Context->Active;
            }
        } while(!AllThreadsInactive);
    }
}

int WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int Show)
{
    // TODO(chris): CreateMutex?
    GlobalRunning = true;

    SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);

    GlobalThreadQueueSemaphore = CreateSemaphore(0, 0, ArrayCount(GlobalThreadQueue), 0);

    for(u32 ThreadWorkIndex = 0;
        ThreadWorkIndex < ArrayCount(GlobalThreadQueue);
        ++ThreadWorkIndex)
    {
        thread_work *Work = GlobalThreadQueue + ThreadWorkIndex;
        Work->Free = true;
    }
    
    MSG Message;
    Message.wParam = 0;
    WNDCLASS WindowClass;

    WindowClass.style = CS_OWNDC;
    WindowClass.lpfnWndProc = WindowProc;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = Instance;
    WindowClass.hIcon = 0;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    WindowClass.hbrBackground = 0;
    WindowClass.lpszMenuName = 0;
    WindowClass.lpszClassName = "TroidsWindowClass";

    if(RegisterClassA(&WindowClass))
    {
        HMONITOR Monitor = GetPrimaryMonitor();
        MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };

        rectangle2i MonitorRect = {0, 0, 1920, 1080};
        if(GetMonitorInfo(Monitor, &MonitorInfo))
        {
            MonitorRect.Min.x = MonitorInfo.rcMonitor.left;
            MonitorRect.Min.y = MonitorInfo.rcMonitor.top;
            MonitorRect.Max.x = MonitorInfo.rcMonitor.right;
            MonitorRect.Max.y = MonitorInfo.rcMonitor.bottom;
        }
//        MonitorRect = {0, 0, 4096, 2160};
        s32 MonitorWidth = MonitorRect.Max.x - MonitorRect.Min.x;
        s32 MonitorHeight = MonitorRect.Max.y - MonitorRect.Min.y;

        s32 BackBufferWidth = MonitorWidth;
        s32 BackBufferHeight = MonitorHeight;
        while(BackBufferWidth*BackBufferHeight > 1920*1080)
        {
            BackBufferWidth /= 2;
            BackBufferHeight /= 2;
        }
        s32 WindowWidth = MonitorWidth/2;
        s32 WindowHeight = MonitorHeight/2;
        GlobalWindow = CreateWindowExA(0,
                                       WindowClass.lpszClassName,
                                       "TROIDS",
                                       WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                       CW_USEDEFAULT,
                                       CW_USEDEFAULT,
                                       WindowWidth,
                                       WindowHeight,
                                       0,
                                       0,
                                       0,
                                       0);
        if(GlobalWindow)
        {
#if !TROIDS_INTERNAL
            ShowCursor(false);
#endif
            
            u32 PathLength;
            char Path[MAX_PATH];

            char *DataPath = DATA_PATH;
            PathLength = GetCurrentDirectoryA(sizeof(Path), Path);
            b32 ChangeDirectorySucceeded = SetCurrentDirectory(DataPath);
            Assert(ChangeDirectorySucceeded);
            PathLength = GetCurrentDirectoryA(sizeof(Path), Path);
            
            GlobalBackBuffer.Width = BackBufferWidth;
            GlobalBackBuffer.Height = BackBufferHeight;
            GlobalBackBuffer.MonitorWidth = MonitorWidth;
            GlobalBackBuffer.MonitorHeight = MonitorHeight;
            GlobalBackBuffer.Info.bmiHeader.biSize = sizeof(GlobalBackBuffer.Info.bmiHeader);
            GlobalBackBuffer.Info.bmiHeader.biWidth = ((GlobalBackBuffer.Width+3)/4)*4;
            GlobalBackBuffer.Info.bmiHeader.biHeight = GlobalBackBuffer.Height;
            GlobalBackBuffer.Info.bmiHeader.biPlanes = 1;
            GlobalBackBuffer.Info.bmiHeader.biBitCount = 32;
            GlobalBackBuffer.Info.bmiHeader.biCompression = BI_RGB;
            GlobalBackBuffer.Info.bmiHeader.biSizeImage = 0;
            GlobalBackBuffer.Info.bmiHeader.biXPelsPerMeter = 1;
            GlobalBackBuffer.Info.bmiHeader.biYPelsPerMeter = 1;
            GlobalBackBuffer.Info.bmiHeader.biClrUsed = 0;
            GlobalBackBuffer.Info.bmiHeader.biClrImportant = 0;
            GlobalBackBuffer.Pitch = GlobalBackBuffer.Info.bmiHeader.biWidth*sizeof(u32);

            HDC DeviceContext = GetDC(GlobalWindow);
            CreateDIBSection(DeviceContext, &GlobalBackBuffer.Info, DIB_RGB_COLORS,
                             &GlobalBackBuffer.Memory, 0, 0);

            PIXELFORMATDESCRIPTOR PixelFormat = {};
            PixelFormat.nSize = sizeof(PixelFormat);
            PixelFormat.nVersion = 1;
            PixelFormat.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
            PixelFormat.iPixelType = PFD_TYPE_RGBA;
            BYTE  cColorBits = 24;
            BYTE  cAlphaBits = 8;
            BYTE  iLayerType = PFD_MAIN_PLANE;
            int SuggestedPixelFormatIndex = ChoosePixelFormat(DeviceContext, &PixelFormat);
            PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
            DescribePixelFormat(DeviceContext, SuggestedPixelFormatIndex,
                                sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
            SetPixelFormat(DeviceContext, SuggestedPixelFormatIndex, &SuggestedPixelFormat);

            b32 VSYNC = InitializeOpenGL(DeviceContext);

            renderer_state RendererState;
            RendererState.BackBuffer.Width = GlobalBackBuffer.Width;
            RendererState.BackBuffer.Height = GlobalBackBuffer.Height;
            RendererState.BackBuffer.Pitch = GlobalBackBuffer.Pitch;
            RendererState.BackBuffer.Memory = GlobalBackBuffer.Memory;
            RendererState.CoverageBuffer.Width = GlobalBackBuffer.Width;
            RendererState.CoverageBuffer.Height = GlobalBackBuffer.Height;
            RendererState.CoverageBuffer.Pitch = GlobalBackBuffer.Pitch;
            RendererState.SampleBuffer.Width = SAMPLE_COUNT*GlobalBackBuffer.Width;
            RendererState.SampleBuffer.Height = GlobalBackBuffer.Height;
            RendererState.SampleBuffer.Pitch = SAMPLE_COUNT*sizeof(u32)*RendererState.SampleBuffer.Width;

            memory_size CoverageBufferMemorySize = RendererState.CoverageBuffer.Height*RendererState.CoverageBuffer.Pitch;
            memory_size SampleBufferMemorySize = RendererState.SampleBuffer.Height*RendererState.SampleBuffer.Pitch;

            game_memory GameMemory;
            GameMemory.PermanentMemorySize = Megabytes(2);
            GameMemory.TemporaryMemorySize = Megabytes(2);
#if TROIDS_INTERNAL
            GameMemory.DebugMemorySize = Gigabytes(2);
#endif
            u64 TotalMemorySize = (GameMemory.PermanentMemorySize +
                                   GameMemory.TemporaryMemorySize +
                                   CoverageBufferMemorySize + SampleBufferMemorySize
                                   
#if TROIDS_INTERNAL
                                   + GameMemory.DebugMemorySize
#endif
                                   );
            GameMemory.PermanentMemory = VirtualAlloc(0, TotalMemorySize,
                                                      MEM_COMMIT|MEM_RESERVE,
                                                      PAGE_READWRITE);
            GameMemory.TemporaryMemory = (u8 *)GameMemory.PermanentMemory + GameMemory.PermanentMemorySize;
            RendererState.CoverageBuffer.Memory = (u8 *)GameMemory.TemporaryMemory + GameMemory.TemporaryMemorySize;
            RendererState.SampleBuffer.Memory = (u8 *)RendererState.CoverageBuffer.Memory + CoverageBufferMemorySize;
#if TROIDS_INTERNAL
            GameMemory.DebugMemory = (u8 *)RendererState.SampleBuffer.Memory + SampleBufferMemorySize;
            GlobalDebugState = (debug_state *)GameMemory.DebugMemory;
            InitializeArena(&GlobalDebugState->Arena,
                            GameMemory.DebugMemorySize - sizeof(debug_state),
                            (u8 *)GameMemory.DebugMemory + sizeof(debug_state));
            GetThreadStorage(GetCurrentThreadID());
#endif

            GlobalThreadCount = Minimum(MAX_THREAD_COUNT, SystemInfo.dwNumberOfProcessors - 1);
            for(u32 ProcessorIndex = 0;
                ProcessorIndex < GlobalThreadCount;
                ++ProcessorIndex)
            {
                thread_context *Context = GlobalThreadContext + ProcessorIndex;
                CreateThread(0, 0, ThreadProc, Context, 0, &Context->ID);
#if TROIDS_INTERNAL
                GetThreadStorage(Context->ID);
#endif
            }
            
            RendererState.RenderChunkCount = GlobalThreadCount + 1;
            for(u32 RenderChunkIndex = 0;
                RenderChunkIndex < RendererState.RenderChunkCount;
                ++RenderChunkIndex)
            {
                render_chunk *RenderChunk = RendererState.RenderChunks + RenderChunkIndex;
                RenderChunk->Used = false;
                RenderChunk->BackBuffer.Pitch = RendererState.BackBuffer.Pitch;
                RenderChunk->BackBuffer.Memory = RendererState.BackBuffer.Memory;
                RenderChunk->CoverageBuffer.Pitch = RendererState.CoverageBuffer.Pitch;
                RenderChunk->CoverageBuffer.Memory = RendererState.CoverageBuffer.Memory;
                RenderChunk->SampleBuffer.Pitch = RendererState.SampleBuffer.Pitch;
                RenderChunk->SampleBuffer.Memory = RendererState.SampleBuffer.Memory;
            }

#if TROIDS_INTERNAL
            Win32LoadFont(&GlobalDebugState->Font, DeviceContext, "Courier New",
                          RoundU32((r32)BackBufferHeight*42.0f/1080.0f), FW_BOLD, true);
#endif
            Win32LoadFont(&GameMemory.Font, DeviceContext, "Arial",
                          RoundU32((r32)BackBufferHeight*128.0f/1080.0f), FW_NORMAL, false);

            GameMemory.PlatformReadFile = Win32ReadFile;
            GameMemory.PlatformPushThreadWork = Win32PushThreadWork;
            GameMemory.PlatformWaitForAllThreadWork = Win32WaitForAllThreadWork;

            // TODO(chris): Query monitor refresh rate
            r32 dtForFrame = 1.0f / 60.0f;

            LPDIRECTSOUND DirectSound;
            LPDIRECTSOUNDBUFFER PrimarySoundBuffer = 0;
            LPDIRECTSOUNDBUFFER SecondarySoundBuffer = 0;
            game_sound_buffer GameSoundBuffer = {};

            HMODULE DSoundCode = LoadLibrary("dsound.dll");
            direct_sound_create *DirectSoundCreate = 0;
            if(DSoundCode)
            {
                DirectSoundCreate =
                    (direct_sound_create *)GetProcAddress(DSoundCode, "DirectSoundCreate");
            }
            
            if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
            {
                if(SUCCEEDED(DirectSound->SetCooperativeLevel(GlobalWindow, DSSCL_PRIORITY)))
                {
                    // NOTE(chris): Primary and secondary buffer need the same format
                    WAVEFORMATEX WaveFormat = {};
                    WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
                    WaveFormat.nChannels = 2;
                    WaveFormat.nSamplesPerSec = 44100;
                    WaveFormat.wBitsPerSample = 16;
                    WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
                    WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
                    DSBUFFERDESC PrimaryBufferDescription = {};
                    PrimaryBufferDescription.dwSize = sizeof(PrimaryBufferDescription); 
                    PrimaryBufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                    if(SUCCEEDED(DirectSound->CreateSoundBuffer(&PrimaryBufferDescription, &PrimarySoundBuffer, 0)))
                    {
                        // NOTE(chris): Set the primary buffer format.
                        PrimarySoundBuffer->SetFormat(&WaveFormat);
                    }
                    else
                    {
                        // TODO(chris): Logging
                    }
                
                    DSBUFFERDESC SecondaryBufferDescription = {};
                    SecondaryBufferDescription.dwSize = sizeof(SecondaryBufferDescription); 
                    SecondaryBufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
                    // NOTE(chris): Secondary sound buffer is 2s long.
                    SecondaryBufferDescription.dwBufferBytes = WaveFormat.nAvgBytesPerSec*2;
                    SecondaryBufferDescription.lpwfxFormat = &WaveFormat;
                    if(SUCCEEDED(DirectSound->CreateSoundBuffer(&SecondaryBufferDescription, &SecondarySoundBuffer, 0)))
                    {
                        GameSoundBuffer.SamplesPerSecond = WaveFormat.nSamplesPerSec;
                        GameSoundBuffer.Channels = WaveFormat.nChannels;
                        GameSoundBuffer.BitsPerSample = WaveFormat.wBitsPerSample;
                        GameSoundBuffer.Size = SecondaryBufferDescription.dwBufferBytes;
                        b32 PlayResult = SecondarySoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
                        Assert(SUCCEEDED(PlayResult));
                    }
                    else
                    {
                        // TODO(chris): Logging
                    }
                }
                else
                {
                    // TODO(chris): Logging
                }
            }
            else
            {
                // TODO(chris): Logging
            }

            InitializeInput(Instance);
            LatchGamePads(GlobalWindow);
            
            DEV_BROADCAST_DEVICEINTERFACE Filter = {};
            Filter.dbcc_size = sizeof(Filter);
            Filter.dbcc_devicetype =  DBT_DEVTYP_DEVICEINTERFACE;
            Filter.dbcc_classguid = GUID_DEVINTERFACE_HID;
//            Filter->dbcc_name[1]
            HDEVNOTIFY DeviceNotification = RegisterDeviceNotificationA(GlobalWindow, &Filter,
                                                                       DEVICE_NOTIFY_WINDOW_HANDLE);
            char EXEFileName[MAX_PATH];
            u32 EXEFileNameLength = GetModuleFileName(0, EXEFileName, sizeof(EXEFileName));
            GameMemory.EXEFileName = EXEFileName;
            GlobalEXEFile = CreateFile(EXEFileName,
                                       GENERIC_READ,
                                       0, 0,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL, 0);
            u32 EXEFileSize = GetFileSize(GlobalEXEFile, 0);
            u32 AssetSize;
            u32 FooterSize = sizeof(AssetSize);
            u32 FooterOffset = EXEFileSize - FooterSize;

            OVERLAPPED Overlapped = {};
            Overlapped.Offset = FooterOffset;
            BOOL ReadFileResult = ReadFile(GlobalEXEFile, &AssetSize,
                                           FooterSize,
                                           0, &Overlapped);

            GameMemory.AssetOffset = EXEFileSize-FooterSize-AssetSize;
#if !ONE_FILE
            game_update_and_render *GameUpdateAndRender = 0;
            game_get_sound_samples *GameGetSoundSamples = 0;
#if TROIDS_INTERNAL
            debug_collate *DebugCollate = 0;
#endif
            // TODO(chris): Is MAX_PATH really correct?
            char BuildPath[MAX_PATH];
            char DLLPath[MAX_PATH];
            char PDBLockPath[MAX_PATH];
            u32 EXEPathLength = GetModuleFileName(0, BuildPath, sizeof(BuildPath));
            u64 CodeWriteTime = 0;
            HMODULE GameCode = 0;
            
            // TODO(chris): Check for truncated path
            if(EXEPathLength)
            {
                char DLLName[] = "troids.dll";
                char PDBLockName[] = "pdb.lock";
                
                char *BuildPathEnd;
                for(BuildPathEnd = BuildPath + EXEPathLength - 1;
                    BuildPathEnd > BuildPath;
                    --BuildPathEnd)
                {
                    if(*(BuildPathEnd - 1) == '\\')
                    {
                        break;
                    }
                }
                u32 BuildPathLength = (u32)(BuildPathEnd - BuildPath);

                u32 DLLPathLength = CatStrings(sizeof(DLLPath), DLLPath,
                                               BuildPathLength, BuildPath, DLLName);
                u32 PDBLockPathLength = CatStrings(sizeof(PDBLockPath), PDBLockPath,
                                                   BuildPathLength, BuildPath, PDBLockName);

                Assert(DLLPathLength == (BuildPathLength + sizeof(DLLName) - 1));
                Assert(PDBLockPathLength == (BuildPathLength + sizeof(PDBLockName) - 1));
            }
            else
            {
                // TODO(chris): Logging
            }
#endif

            TIMECAPS TimeCaps;
            timeGetDevCaps(&TimeCaps, sizeof(TimeCaps));
            if(TimeCaps.wPeriodMin != 1)
            {
                Assert(!"Couldn't fix timer granularity");
                // TODO(chris): Logging
            }
            if(TIMERR_NOCANDO == timeBeginPeriod(TimeCaps.wPeriodMin))
            {
                Assert(!"Couldn't fix timer granularity");
                // TODO(chris): Logging
            }

            r32 ClocksToSeconds = 0.0f;
            LARGE_INTEGER Freq, LastCounter, Counter;
            if(QueryPerformanceFrequency(&Freq))
            {
                ClocksToSeconds = 1.0f / Freq.QuadPart;
            }
            else
            {
                // TODO(chris): Logging
            }

            // TODO(chris): Allocate some memory for the game state.

#if TROIDS_INTERNAL
            recording_state RecordingState = RecordingState_None;
            // TODO(chris): Memory-map file? What happens when game state gets huge?
            HANDLE RecordFile = CreateFile("record.trc",
                                           GENERIC_READ|GENERIC_WRITE,
                                           0, 0,
                                           CREATE_ALWAYS,
                                           FILE_ATTRIBUTE_NORMAL, 0);
#endif
            
            DWORD ByteToLock = 0;
            // TODO(chris): Maybe initialize this with starting controller state?
            game_input GameInput[3] = {};
            game_input *OldInput = GameInput;
            game_input *NewInput = GameInput + 1;

            u32 FrameSecondsIndex = 0;
            r32 FrameSecondsBuffer[4] =
            {
                dtForFrame,
                dtForFrame,
                dtForFrame,
                dtForFrame,
            };

            NewInput->MostRecentlyUsedController = LatchedGamePadCount() ? 1 : 0;
            ToggleFullscreen(GlobalWindow, MonitorRect);
            QueryPerformanceCounter(&LastCounter);
            while(GlobalRunning)
            {
                BEGIN_TIMED_BLOCK(FrameIndex, "Platform");
                {
                    TIMED_BLOCK("Input");

                    game_input *Temp = NewInput;
                    NewInput = OldInput;
                    OldInput = Temp;
                    *NewInput = {};
                    NewInput->dtForFrame = dtForFrame;
                    NewInput->SeedValue = LastCounter.QuadPart;
                    NewInput->MostRecentlyUsedController = OldInput->MostRecentlyUsedController;

                    game_controller *NewKeyboard = &NewInput->Keyboard;
                    game_controller *OldKeyboard = &OldInput->Keyboard;
                    for(u32 ButtonIndex = 0;
                        ButtonIndex < ArrayCount(NewKeyboard->Buttons);
                        ++ButtonIndex)
                    {
                        game_button *NewButton = NewKeyboard->Buttons + ButtonIndex;
                        game_button *OldButton = OldKeyboard->Buttons + ButtonIndex;
                        NewButton->EndedDown = OldButton->EndedDown;
                    }
                    NewKeyboard->LeftStick.x = OldKeyboard->LeftStick.x;
                    NewKeyboard->LeftStick.y = OldKeyboard->LeftStick.y;
                    NewKeyboard->RightStick.x = OldKeyboard->RightStick.x;
                    NewKeyboard->RightStick.y = OldKeyboard->RightStick.y;
                    NewKeyboard->Type = ControllerType_Keyboard;
                    GlobalLeftMouse.HalfTransitionCount = 0;

                    while(PeekMessageA(&Message, GlobalWindow, 0, 0, PM_REMOVE))
                    {
                        switch(Message.message)
                        {                            
                            case WM_KEYDOWN:
                            case WM_KEYUP:
                            case WM_SYSKEYDOWN:
                            case WM_SYSKEYUP:
                            {
                                NewInput->MostRecentlyUsedController = 0;
                                b32 AlreadyDown = (Message.lParam & (1 << 30));
                                b32 WentDown = !(Message.lParam & (1 << 31));
                                if(!(AlreadyDown && WentDown))
                                {
                                    switch(Message.wParam)
                                    {
                                        case VK_SPACE:
                                        {
                                            ProcessButtonInput(&NewKeyboard->ActionDown);
                                        } break;

                                        case VK_RETURN:
                                        {
                                            ProcessButtonInput(&NewKeyboard->Start);
                                        } break;

                                        case VK_ESCAPE:
                                        {
                                            ProcessButtonInput(&NewKeyboard->Select);
                                        } break;

                                        case 'W':
                                        {
                                            ProcessAnalogInput(&NewKeyboard->LeftStick.y, WentDown, 1.0f);
                                        } break;

                                        case 'A':
                                        {
                                            ProcessAnalogInput(&NewKeyboard->LeftStick.x, WentDown, -1.0f);
                                        } break;

                                        case 'S':
                                        {
                                            ProcessAnalogInput(&NewKeyboard->LeftStick.y, WentDown, -1.0f);
                                        } break;

                                        case 'D':
                                        {
                                            ProcessAnalogInput(&NewKeyboard->LeftStick.x, WentDown, 1.0f);
                                        } break;
                                    }
                                    if(WentDown)
                                    {
                                        b32 AltIsDown = (Message.lParam & (1 << 29));
                                        switch(Message.wParam)
                                        {
                                            case VK_RETURN:
                                            {
                                                if(AltIsDown)
                                                {
                                                    ToggleFullscreen(GlobalWindow, MonitorRect);
                                                }
                                            } break;

                                            case VK_F4:
                                            {
                                                if(AltIsDown)
                                                {
                                                    GlobalRunning = false;
                                                }
                                            } break;

#if TROIDS_INTERNAL
                                            case 'R':
                                            {
                                                switch(RecordingState)
                                                {
                                                    case RecordingState_None:
                                                    {
                                                        RecordingState = RecordingState_Recording;
                                                        if(RecordFile)
                                                        {
                                                            DWORD BytesWritten;
                                                            OVERLAPPED Overlapped = {};
                                                            BOOL Result = WriteFile(RecordFile, GameMemory.PermanentMemory,
                                                                                    (u32)GameMemory.PermanentMemorySize,
                                                                                    &BytesWritten, &Overlapped);
                                                            Assert(Result && (BytesWritten == GameMemory.PermanentMemorySize));
                                                        }
                                                    } break;

                                                    case RecordingState_Recording:
                                                    {
                                                        RecordingState = RecordingState_PlayingRecord;
                                                        if(RecordFile)
                                                        {
                                                            DWORD BytesRead;
                                                            OVERLAPPED Overlapped = {};
                                                            BOOL Result = ReadFile(RecordFile, GameMemory.PermanentMemory,
                                                                                   (u32)GameMemory.PermanentMemorySize,
                                                                                   &BytesRead, &Overlapped);
                                                            Assert(Result && (BytesRead == GameMemory.PermanentMemorySize));
                                                        }
                                                    } break;

                                                    case RecordingState_PlayingRecord:
                                                    {
                                                        RecordingState = RecordingState_None;
                                                        // TODO(chris): Do something to reset controller
                                                        // state so we don't have errant inputs.
                                                    } break;
                                                }
                                            }
#endif
                                        }
                                    }
                                }
                            } break;

                            default:
                            {
                                TranslateMessage(&Message);
                                DispatchMessageA(&Message);
                            } break;
                        }
                    }

                    ProcessGamePadInput(OldInput, NewInput);

                    for(u32 ControllerIndex = 1;
                        ControllerIndex < ArrayCount(NewInput->Controllers);
                        ++ControllerIndex)
                    {
                        game_controller *Controller = NewInput->Controllers + ControllerIndex;
                        b32 AnyButtonsActive = false;
                        for(u32 ButtonIndex = 0;
                            ButtonIndex < ArrayCount(Controller->Buttons);
                            ++ButtonIndex)
                        {
                            game_button *Button = Controller->Buttons + ButtonIndex;
                            AnyButtonsActive |= (Button->EndedDown || Button->HalfTransitionCount);
                        }
                        if(Controller->LeftStick.x != 0.0f ||
                           Controller->LeftStick.y != 0.0f ||
                           Controller->RightStick.x != 0.0f ||
                           Controller->RightStick.y != 0.0f ||
                           Controller->LeftTrigger != 0.0f ||
                           Controller->RightTrigger != 0.0f ||
                           AnyButtonsActive)
                        {
                            NewInput->MostRecentlyUsedController = ControllerIndex;
                        }
                    }
                    NewInput->MousePosition = {GlobalMousePosition.x,
                                               MonitorHeight - GlobalMousePosition.y};
                    NewInput->LeftMouse = GlobalLeftMouse;
                }

#if TROIDS_INTERNAL
                {
                    TIMED_BLOCK("Recording");
                    switch(RecordingState)
                    {
                        case RecordingState_Recording:
                        {
                            DWORD BytesWritten;
                            BOOL Result = WriteFile(RecordFile, NewInput,
                                                    sizeof(*NewInput),
                                                    &BytesWritten, 0);
                            Assert(Result && (BytesWritten == sizeof(*NewInput)));
                        } break;
                    
                        case RecordingState_PlayingRecord:
                        {
                            DWORD BytesRead;
                            BOOL Result = ReadFile(RecordFile, NewInput,
                                                   sizeof(*NewInput),
                                                   &BytesRead, 0);
                            if(Result && (BytesRead == 0))
                            {
                                OVERLAPPED Overlapped = {};
                                Result = ReadFile(RecordFile, GameMemory.PermanentMemory,
                                                  (u32)GameMemory.PermanentMemorySize,
                                                  &BytesRead, &Overlapped);
                                Assert(Result && (BytesRead == GameMemory.PermanentMemorySize));
                                Result = ReadFile(RecordFile, NewInput,
                                                  sizeof(*NewInput),
                                                  &BytesRead, 0);
                            }
                            Assert(Result && (BytesRead == sizeof(*NewInput)));
                        } break;
                    }
                }
#endif

#if !ONE_FILE
                {
                    TIMED_BLOCK("DLL Reload");
                    WIN32_FILE_ATTRIBUTE_DATA FileInfo;
                    if(GetFileAttributesEx(DLLPath, GetFileExInfoStandard, &FileInfo))
                    {
                        u64 LastWriteTime = (((u64)FileInfo.ftLastWriteTime.dwHighDateTime << 32) |
                                             (FileInfo.ftLastWriteTime.dwLowDateTime << 0));
                        if((LastWriteTime > CodeWriteTime) && !FileExists(PDBLockPath))
                        {
                            Win32WaitForAllThreadWork();
                            if(GameCode)
                            {
                                // TODO(chris): Is this necessary?
                                FreeLibrary(GameCode);
                            }
                            GameCode = LoadLibrary(DLLPath);
                            if(GameCode)
                            {
                                CodeWriteTime = LastWriteTime;
                                GameUpdateAndRender =
                                    (game_update_and_render *)GetProcAddress(GameCode, "GameUpdateAndRender");
                                GameGetSoundSamples =
                                    (game_get_sound_samples *)GetProcAddress(GameCode, "GameGetSoundSamples");
#if TROIDS_INTERNAL
                                DebugCollate =
                                    (debug_collate *)GetProcAddress(GameCode, "DebugCollate");
#endif
                            }
                        }
                    }
                }
#endif
                END_TIMED_BLOCK(FrameIndex);

#if !ONE_FILE
                if(GameUpdateAndRender)
#endif
                {
                    GameUpdateAndRender(&GameMemory, NewInput, &RendererState);
                    GlobalRunning &= !NewInput->Quit;
                }
#if !ONE_FILE
                if(GameGetSoundSamples && SecondarySoundBuffer)
#endif
                {
                    TIMED_BLOCK("Platform Sound");
                    DWORD PlayCursor;
                    DWORD WriteCursor;
                    SecondarySoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
                    DWORD BytesToWrite;
                    u32 SampleSize = GameSoundBuffer.Channels*GameSoundBuffer.BitsPerSample / 8;
                    if(PlayCursor > ByteToLock)
                    {
                        BytesToWrite = PlayCursor - ByteToLock;
                    }
                    else
                    {
                        BytesToWrite = GameSoundBuffer.Size - ByteToLock;
                        BytesToWrite += PlayCursor;
                    }
                    BytesToWrite = Minimum(8192, BytesToWrite);
                    b32 LockResult = SecondarySoundBuffer->Lock(ByteToLock, BytesToWrite,
                                                                &GameSoundBuffer.Region1, (LPDWORD)&GameSoundBuffer.Region1Size,
                                                                &GameSoundBuffer.Region2, (LPDWORD)&GameSoundBuffer.Region2Size,
                                                                0);
                    Assert(SUCCEEDED(LockResult));
                    GameGetSoundSamples(&GameMemory, GameInput, &GameSoundBuffer);
                    b32 UnlockResult = SecondarySoundBuffer->Unlock(GameSoundBuffer.Region1, GameSoundBuffer.Region1Size,
                                                                    GameSoundBuffer.Region2, GameSoundBuffer.Region2Size);
                    Assert(SUCCEEDED(UnlockResult));
                    ByteToLock += BytesToWrite;
                    if(ByteToLock >= GameSoundBuffer.Size)
                    {
                        ByteToLock -= GameSoundBuffer.Size;
                    }
                }


#if TROIDS_INTERNAL
                {
                    TIMED_BLOCK("Draw Recording");
                    loaded_bitmap *GameBackBuffer = &RendererState.BackBuffer;
                    u32 RecordIconMargin = 10;
                    u32 RecordIconRadius = 10;
                    switch(RecordingState)
                    {
                        case RecordingState_Recording:
                        {
                            u32 RecordIconRadius = 10;
                            u8 *PixelRow = ((u8 *)GameBackBuffer->Memory +
                                            GameBackBuffer->Pitch*(GameBackBuffer->Height -
                                                                  (RecordIconMargin + 1)));
                            for(u32 Y = RecordIconMargin;
                                Y < RecordIconMargin + RecordIconRadius*2;
                                ++Y)
                            {
                                u32 *Pixel = (u32 *)PixelRow + RecordIconMargin;
                                for(u32 X = RecordIconMargin;
                                    X < RecordIconMargin + RecordIconRadius*2;
                                    ++X)
                                {
                                    *Pixel++ = 0xFFFF0000;
                                }
                                PixelRow -= GameBackBuffer->Pitch;
                            }
                        } break;

                        case RecordingState_PlayingRecord:
                        {
                            u8 *PixelRow = ((u8 *)GameBackBuffer->Memory +
                                            GameBackBuffer->Pitch*(GameBackBuffer->Height -
                                                                  (RecordIconMargin + 1)));
                            for(u32 Y = RecordIconMargin;
                                Y < RecordIconMargin + RecordIconRadius;
                                ++Y)
                            {
                                u32 *Pixel = (u32 *)PixelRow + RecordIconMargin;
                                u32 Width = (Y - RecordIconMargin)*2;
                                for(u32 X = RecordIconMargin;
                                    X < RecordIconMargin + Width;
                                    ++X)
                                {
                                    *Pixel++ = 0xFFFFFFFF;
                                }
                                PixelRow -= GameBackBuffer->Pitch;
                            }
                            for(u32 Y = RecordIconMargin + RecordIconRadius;
                                Y < RecordIconMargin + 2*RecordIconRadius;
                                ++Y)
                            {
                                u32 *Pixel = (u32 *)PixelRow + RecordIconMargin;
                                u32 Width = (RecordIconMargin + 2*RecordIconRadius - Y)*2;
                                for(u32 X = RecordIconMargin;
                                    X < RecordIconMargin + Width;
                                    ++X)
                                {
                                    *Pixel++ = 0xFFFFFFFF;
                                }
                                PixelRow -= GameBackBuffer->Pitch;
                            }
                        } break;
                    }
                }
#endif

#if TROIDS_INTERNAL
#if !ONE_FILE
                if(DebugCollate)
#endif
                {
                    DebugCollate(&GameMemory, NewInput, &RendererState);
                }
#endif

#if 0
                GlobalRenderMode = RenderMode_GDI;
                VSYNC = false;
#endif
                CopyBackBufferToWindow(DeviceContext, &GlobalBackBuffer);
                if(!VSYNC)
                {
                    QueryPerformanceCounter(&Counter);
                    r32 ElapsedSeconds = (Counter.QuadPart - LastCounter.QuadPart)*ClocksToSeconds;

                    // TODO(chris): Use as backup if VSYNC fails.
                    if(ElapsedSeconds < dtForFrame)
                    {
                        s32 MSToSleep  = (u32)((dtForFrame - ElapsedSeconds)*1000.0f) - 1;
                        if(MSToSleep > 0)
                        {
                            Sleep(MSToSleep);
                        }
                        do
                        {
                            QueryPerformanceCounter(&Counter);
                            ElapsedSeconds = (Counter.QuadPart - LastCounter.QuadPart)*ClocksToSeconds;
                        } while (ElapsedSeconds < dtForFrame);
                    }
                    else if (ElapsedSeconds > dtForFrame)
                    {
#if TROIDS_DEBUG_DISPLAY
                        OutputDebugStringA("Missed framerate!\n");
#endif
                    }
                }

                QueryPerformanceCounter(&Counter);
                r32 FrameSeconds = (Counter.QuadPart - LastCounter.QuadPart)*ClocksToSeconds;

                FrameSecondsBuffer[FrameSecondsIndex] = FrameSeconds;
                FrameSecondsIndex = ((FrameSecondsIndex + 1) & 3);
                // NOTE(chris): Because querying the VSYNC rate is impossible on Windows?
                r32 AverageFrameHz = 4.0f/(FrameSecondsBuffer[0] + FrameSecondsBuffer[1] +
                                           FrameSecondsBuffer[2] + FrameSecondsBuffer[3]);
                if(AverageFrameHz > 45.0f)
                {
                    dtForFrame = 1.0f / 60.0f;
                }
                else if(AverageFrameHz > 25.0f)
                {
                    dtForFrame = 1.0f / 30.0f;
                }
                else if(AverageFrameHz > 17.5f)
                {
                    dtForFrame = 1.0f / 20.0f;
                }
                else if(AverageFrameHz > 13.5f)
                {
                    dtForFrame = 1.0f / 15.0f;
                }
                else if(AverageFrameHz > 11.0f)
                {
                    dtForFrame = 1.0f / 12.0f;
                }
                else
                {
                    dtForFrame = 1.0f / 10.0f;
                }
                FRAME_MARKER(FrameSeconds);
                LastCounter = Counter;
            }
            timeEndPeriod(TimeCaps.wPeriodMin);
        }
        else
        {
            // TODO(chris): Logging
        }
    }
    else
    {
        // TODO(chris): Logging
    }
    return((int)Message.wParam);
}
