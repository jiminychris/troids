/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include <windows.h>
#include <stdio.h>
#include <dbt.h>
#include <gl/gl.h>

#include "troids_opengl.h"
#include "troids_platform.h"
#include "troids_intrinsics.h"
#include "troids_debug.h"

#if TROIDS_INTERNAL
#define DATA_PATH "..\\troids\\data"
#else
#define DATA_PATH "data"
#endif

typedef BOOL WINAPI wgl_swap_interval_ext(int);
    
#include <dsound.h>
#define DIRECT_SOUND_CREATE(Name) HRESULT WINAPI Name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

#define CREATE_GUID(Name, x, y, z, a, b, c, d, e, f, g, h) const GUID Name = {x, y, z, {a, b, c, d, e, f, g, h}}
#include "troids_input.h"
#if 1
#include "troids_dinput.cpp"
#else
#include "troids_hid.cpp"
#endif

inline u32
Minimum(u32 A, u32 B)
{
    if(B < A)
    {
        A = B;
    }
    return(A);
}

struct win32_backbuffer
{
    s32 Width;
    s32 Height;
    s32 Pitch;
    BITMAPINFO Info;
    void *Memory;
};

global_variable win32_backbuffer GlobalBackBuffer;
global_variable b32 GlobalRunning;

global_variable WINDOWPLACEMENT PreviousWindowPlacement = { sizeof(PreviousWindowPlacement) };

inline void
ToggleFullscreen(HWND Window)
{
    // NOTE(chris): Taken from Raymond Chen's blog.
    DWORD WindowStyle = GetWindowLong(Window, GWL_STYLE);
    if(WindowStyle & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
        if(GetWindowPlacement(Window, &PreviousWindowPlacement) &&
            GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, WindowStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
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

inline void
CopyBackBufferToWindow(HDC DeviceContext, win32_backbuffer *BackBuffer)
{
#if 0
    StretchDIBits(DeviceContext,
                  0,
                  0,
                  GlobalBackBuffer.Width,
                  GlobalBackBuffer.Height,
                  0,
                  0,
                  GlobalBackBuffer.Width,
                  GlobalBackBuffer.Height,
                  GlobalBackBuffer.Memory,
                  &GlobalBackBuffer.Info,
                  DIB_RGB_COLORS,
                  SRCCOPY);
#else

#if 0
//    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
//    glClear(GL_COLOR_BUFFER_BIT);
    glDrawPixels(BackBuffer->Width, BackBuffer->Height, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
                 BackBuffer->Memory);
#else
    glViewport(0, 0, BackBuffer->Width, BackBuffer->Height);

    glBindTexture(GL_TEXTURE_2D, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BackBuffer->Width, BackBuffer->Height,
                 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, BackBuffer->Memory);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glEnable(GL_TEXTURE_2D);
    
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

    glBindTexture(GL_TEXTURE_2D, 0);
    
#endif
    SwapBuffers(DeviceContext);
#endif
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
                                    LatchControllers(Window);
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
                                    UnlatchController();
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
        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam); 
        } break;
    }
    return(Result);
}

enum recording_state
{
    RecordingState_None,
    RecordingState_Recording,
    RecordingState_PlayingRecord,
};

inline void
ProcessKeyboardMessage(game_button *Button)
{
    Button->EndedDown = !Button->EndedDown;
    ++Button->HalfTransitionCount;
}

inline void
ProcessKeyboardMessage(r32 *AnalogInput, b32 WentDown, r32 Value)
{
    if(!WentDown)
    {
        Value = -Value;
    }
    *AnalogInput += Value;
}

internal read_file_result
Win32ReadFile(char *FileName)
{
    read_file_result Result = {};
    
    HANDLE File = CreateFile(FileName,
                             GENERIC_READ,
                             0, 0,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, 0);
    if(INVALID_HANDLE_VALUE == File)
    {
        // TODO(chris): Logging
    }
    else
    {
        Result.ContentsSize = GetFileSize(File, 0);
        Result.Contents = VirtualAlloc(0, Result.ContentsSize,
                                       MEM_COMMIT|MEM_RESERVE,
                                       PAGE_READWRITE);
        DWORD BytesRead;
        BOOL ReadFileResult = ReadFile(File, Result.Contents,
                                       (u32)Result.ContentsSize,
                                       &BytesRead, 0);

        Assert(ReadFileResult);
    }

    return(Result);
}

internal void
InitializeOpenGL(HDC DeviceContext)
{
    HGLRC OpenGLRC = wglCreateContext(DeviceContext);
    if(wglMakeCurrent(DeviceContext, OpenGLRC))
    {
        char *Version = (char *)glGetString(GL_VERSION);
        wgl_swap_interval_ext *wglSwapIntervalEXT = (wgl_swap_interval_ext *)wglGetProcAddress("wglSwapIntervalEXT");
        if(wglSwapIntervalEXT)
        {
            if(wglSwapIntervalEXT(1))
            {
                int A = 0;
                // NOTE(chris): VSYNC!
            }
        }
    }
    else
    {
        InvalidCodePath;
        // TODO(chris): Diagnostic
    }
}

internal b32
FileExists(char *Path)
{
    b32 Result = (GetFileAttributes(Path) != INVALID_FILE_ATTRIBUTES);

    return(Result);
}

int WinMain(HINSTANCE Instance,
            HINSTANCE PrevInstance,
            LPSTR     CommandLine,
            int       Show)
{
    // TODO(chris): CreateMutex?
    
    MSG Message;
    Message.wParam = 0;
    WNDCLASS WindowClass;

    WindowClass.style = CS_OWNDC;
    WindowClass.lpfnWndProc = WindowProc;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = Instance;
    WindowClass.hIcon = 0;
    WindowClass.hCursor = 0;
    WindowClass.hbrBackground = 0;
    WindowClass.lpszMenuName = 0;
    WindowClass.lpszClassName = "TroidsWindowClass";

    if(RegisterClassA(&WindowClass))
    {
        s32 BackBufferWidth = 1920;
        s32 BackBufferHeight = 1080;
        s32 WindowWidth = 1920/2;
        s32 WindowHeight = 1080/2;
        HWND Window = CreateWindowExA(0,
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
        if(Window)
        {
            u32 PathLength;
            char Path[MAX_PATH];

            char *DataPath = DATA_PATH;
            PathLength = GetCurrentDirectoryA(sizeof(Path), Path);
            Assert(SetCurrentDirectory(DataPath));
            PathLength = GetCurrentDirectoryA(sizeof(Path), Path);
            
            GlobalBackBuffer.Width = BackBufferWidth;
            GlobalBackBuffer.Height = BackBufferHeight;
            GlobalBackBuffer.Pitch = GlobalBackBuffer.Width*sizeof(u32);
            GlobalBackBuffer.Info.bmiHeader.biSize = sizeof(GlobalBackBuffer.Info.bmiHeader);
            GlobalBackBuffer.Info.bmiHeader.biWidth = GlobalBackBuffer.Width;
            GlobalBackBuffer.Info.bmiHeader.biHeight = GlobalBackBuffer.Height;
            GlobalBackBuffer.Info.bmiHeader.biPlanes = 1;
            GlobalBackBuffer.Info.bmiHeader.biBitCount = 32;
            GlobalBackBuffer.Info.bmiHeader.biCompression = BI_RGB;
            GlobalBackBuffer.Info.bmiHeader.biSizeImage = 0;
            GlobalBackBuffer.Info.bmiHeader.biXPelsPerMeter = 1;
            GlobalBackBuffer.Info.bmiHeader.biYPelsPerMeter = 1;
            GlobalBackBuffer.Info.bmiHeader.biClrUsed = 0;
            GlobalBackBuffer.Info.bmiHeader.biClrImportant = 0;

            HDC DeviceContext = GetDC(Window);
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

            InitializeOpenGL(DeviceContext);

            game_memory GameMemory;
            for(u32 GlyphIndex = 0;
                GlyphIndex < ArrayCount(GameMemory.DebugGlyphs);
                ++GlyphIndex)
            {
                GameMemory.DebugGlyphs[GlyphIndex] = {};
            }
            // TODO(chris): All font stuff should move into a packed asset file sometime.
            {
                win32_backbuffer FontBuffer;
                FontBuffer.Height = 1024;
                FontBuffer.Width = 1024;
                FontBuffer.Pitch = FontBuffer.Width*sizeof(u32);
                FontBuffer.Info.bmiHeader.biSize = sizeof(FontBuffer.Info.bmiHeader);
                FontBuffer.Info.bmiHeader.biWidth = FontBuffer.Width;
                FontBuffer.Info.bmiHeader.biHeight = FontBuffer.Height;
                FontBuffer.Info.bmiHeader.biPlanes = 1;
                FontBuffer.Info.bmiHeader.biBitCount = 32;
                FontBuffer.Info.bmiHeader.biCompression = BI_RGB;
                FontBuffer.Info.bmiHeader.biSizeImage = 0;
                FontBuffer.Info.bmiHeader.biXPelsPerMeter = 1;
                FontBuffer.Info.bmiHeader.biYPelsPerMeter = 1;
                FontBuffer.Info.bmiHeader.biClrUsed = 0;
                FontBuffer.Info.bmiHeader.biClrImportant = 0;

                HDC FontDC = CreateCompatibleDC(DeviceContext);
                
                HBITMAP FontBitmap = CreateDIBSection(FontDC, &FontBuffer.Info, DIB_RGB_COLORS,
                                                      &FontBuffer.Memory, 0, 0);
                SelectObject(FontDC, FontBitmap);
                HFONT DebugFont = CreateFont(128, 0, // NOTE(chris): Height/Width
                                             0, // NOTE(chris): Escapement
                                             0, // NOTE(chris): Orientation
                                             0, // NOTE(chris): Weight
                                             0, // NOTE(chris): Italic
                                             0, // NOTE(chris): Underline
                                             0, // NOTE(chris): Strikeout
                                             ANSI_CHARSET,
                                             OUT_DEFAULT_PRECIS,
                                             CLIP_DEFAULT_PRECIS,
                                             ANTIALIASED_QUALITY,
                                             DEFAULT_PITCH|FF_DONTCARE,
                                             "Courier New");
                SelectObject(FontDC, DebugFont);
                SetBkColor(FontDC, RGB(0, 0, 0));
                SetTextColor(FontDC, RGB(255, 255, 255));

                TEXTMETRIC Metrics;
                GetTextMetrics(FontDC, &Metrics);
                GameMemory.DebugFontHeight = (r32)Metrics.tmHeight;

                for(char GlyphIndex = '!';
                    GlyphIndex <= '~';
                    ++GlyphIndex)
                {
                    SIZE GlyphSize;
                    Assert(GetTextExtentPoint32(FontDC, &GlyphIndex, 1, &GlyphSize));
                    Assert(TextOutA(FontDC, 128, 0, &GlyphIndex, 1));

                    s32 MinX = 10000;
                    s32 MaxX = -10000;
                    s32 MinY = 10000;
                    s32 MaxY = -10000;

                    u8 *ScanRow = (u8 *)FontBuffer.Memory;
                    for(s32 Y = 0;
                        Y < FontBuffer.Height;
                        ++Y)
                    {
                        u32 *Scan = (u32 *)ScanRow;
                        for(s32 X = 0;
                            X < FontBuffer.Width;
                            ++X)
                        {
                            if(*Scan++)
                            {
                                MinX = Minimum(X, MinX);
                                MaxX = Maximum(X, MaxX);
                                MinY = Minimum(Y, MinY);
                                MaxY = Maximum(Y, MaxY);
                            }
                        }
                        ScanRow += FontBuffer.Pitch;
                    }

                    loaded_bitmap *DebugGlyph = GameMemory.DebugGlyphs + GlyphIndex;
                    DebugGlyph->Height = MaxY - MinY + 3;
                    DebugGlyph->Width = MaxX - MinX + 3;
                    DebugGlyph->Align = {0.0f, 0.0f};
                    DebugGlyph->Pitch = DebugGlyph->Width*sizeof(u32);
                    // TODO(chris): Don't do this. This should allocate from a custom asset
                    // virtual memory system.
                    DebugGlyph->Memory = VirtualAlloc(0,
                                                      sizeof(u32)*DebugGlyph->Height*DebugGlyph->Width,
                                                      MEM_COMMIT|MEM_RESERVE,
                                                      PAGE_READWRITE);

                    u8 *SourceRow = (u8 *)FontBuffer.Memory + FontBuffer.Pitch*MinY;
                    u8 *DestRow = (u8 *)DebugGlyph->Memory;
                    for(s32 Y = 1;
                        Y < DebugGlyph->Height - 1;
                        ++Y)
                    {
                        u32 *Source = (u32 *)SourceRow + MinX;
                        u32 *Dest = (u32 *)DestRow;
                        for(s32 X = 1;
                            X < DebugGlyph->Width - 1;
                            ++X)
                        {
                            u32 Alpha = (*Source & 0xFF);
                            *Dest++ = ((Alpha << 24) |
                                     (Alpha << 16) |
                                     (Alpha << 8)  |
                                     (Alpha << 0));
                            *Source++ = 0;
                        }
                        SourceRow += FontBuffer.Pitch;
                        DestRow += DebugGlyph->Pitch;
                    }
                }
                
                DeleteObject(FontBitmap);
                DeleteDC(FontDC);
            }
            
            ReleaseDC(Window, DeviceContext);

            GameMemory.PermanentMemorySize = Megabytes(256);
            GameMemory.TemporaryMemorySize = Gigabytes(2);
            GameMemory.DebugMemorySize = Gigabytes(2);
            u64 TotalMemorySize = (GameMemory.PermanentMemorySize +
                                   GameMemory.TemporaryMemorySize +
                                   GameMemory.DebugMemorySize);
            GameMemory.PermanentMemory = VirtualAlloc(0, TotalMemorySize,
                                                      MEM_COMMIT|MEM_RESERVE,
                                                      PAGE_READWRITE);
            GameMemory.TemporaryMemory = (u8 *)GameMemory.PermanentMemory + GameMemory.PermanentMemorySize;
            GameMemory.DebugMemory = (u8 *)GameMemory.TemporaryMemory + GameMemory.TemporaryMemorySize;
            GlobalDebugState = (debug_state *)GameMemory.DebugMemory;
            InitializeArena(&GlobalDebugState->Arena,
                            GameMemory.DebugMemorySize - sizeof(debug_state),
                            (u8 *)GameMemory.DebugMemory + sizeof(debug_state));

            GameMemory.PlatformReadFile = Win32ReadFile;
            
            // TODO(chris): Query monitor refresh rate
            r32 dtForFrame = 1.0f / 60.0f;
            
            game_backbuffer GameBackBuffer;
            GameBackBuffer.Width = GlobalBackBuffer.Width;
            GameBackBuffer.Height = GlobalBackBuffer.Height;
            GameBackBuffer.Pitch = GlobalBackBuffer.Pitch;
            GameBackBuffer.Memory = GlobalBackBuffer.Memory;

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
                if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
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
                        Assert(SUCCEEDED(SecondarySoundBuffer->Play(0, 0, DSBPLAY_LOOPING)));
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
            LatchControllers(Window);
            
            DEV_BROADCAST_DEVICEINTERFACE Filter = {};
            Filter.dbcc_size = sizeof(Filter);
            Filter.dbcc_devicetype =  DBT_DEVTYP_DEVICEINTERFACE;
            Filter.dbcc_classguid = GUID_DEVINTERFACE_HID;
//            Filter->dbcc_name[1]
            HDEVNOTIFY DeviceNotification = RegisterDeviceNotificationA(Window, &Filter,
                                                                       DEVICE_NOTIFY_WINDOW_HANDLE);

            game_update_and_render *GameUpdateAndRender = 0;
            game_get_sound_samples *GameGetSoundSamples = 0;
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
            
            recording_state RecordingState = RecordingState_None;
            // TODO(chris): Memory-map file? What happens when game state gets huge?
            HANDLE RecordFile = CreateFile("record.trc",
                                           GENERIC_READ|GENERIC_WRITE,
                                           0, 0,
                                           CREATE_ALWAYS,
                                           FILE_ATTRIBUTE_NORMAL, 0);
            DWORD ByteToLock = 0;
            GlobalRunning = true;
            // TODO(chris): Maybe initialize this with starting controller state?
            game_input GameInput[3] = {};
            game_input *OldInput = GameInput;
            game_input *NewInput = GameInput + 1;

            QueryPerformanceCounter(&LastCounter);
            while(GlobalRunning)
            {                
                {
                    TIMED_BLOCK(Input);

                    game_input *Temp = NewInput;
                    NewInput = OldInput;
                    OldInput = Temp;
                    *NewInput = {};
                    NewInput->dtForFrame = dtForFrame;

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
                    NewKeyboard->LeftStickX = OldKeyboard->LeftStickX;
                    NewKeyboard->LeftStickY = OldKeyboard->LeftStickY;
                    NewKeyboard->RightStickX = OldKeyboard->RightStickX;
                    NewKeyboard->RightStickY = OldKeyboard->RightStickY;

                    while(PeekMessageA(&Message, Window, 0, 0, PM_REMOVE))
                    {
                        switch(Message.message)
                        {
                            case WM_KEYDOWN:
                            case WM_KEYUP:
                            case WM_SYSKEYDOWN:
                            case WM_SYSKEYUP:
                            {
                                b32 AlreadyDown = (Message.lParam & (1 << 30));
                                b32 WentDown = !(Message.lParam & (1 << 31));
                                if(!(AlreadyDown && WentDown))
                                {
                                    switch(Message.wParam)
                                    {
                                        case VK_UP:
                                        {
                                            ProcessKeyboardMessage(&NewKeyboard->ActionUp);
                                        } break;

                                        case VK_LEFT:
                                        {
                                            ProcessKeyboardMessage(&NewKeyboard->ActionLeft);
                                        } break;

                                        case VK_DOWN:
                                        {
                                            ProcessKeyboardMessage(&NewKeyboard->ActionDown);
                                        } break;

                                        case VK_RIGHT:
                                        {
                                            ProcessKeyboardMessage(&NewKeyboard->ActionRight);
                                        } break;

                                        case VK_SPACE:
                                        {
                                            ProcessKeyboardMessage(&NewKeyboard->RightShoulder1);
                                        } break;

                                        case 'W':
                                        {
                                            ProcessKeyboardMessage(&NewKeyboard->LeftStickY, WentDown, 1.0f);
                                        } break;

                                        case 'A':
                                        {
                                            ProcessKeyboardMessage(&NewKeyboard->LeftStickX, WentDown, -1.0f);
                                        } break;

                                        case 'S':
                                        {
                                            ProcessKeyboardMessage(&NewKeyboard->LeftStickY, WentDown, -1.0f);
                                        } break;

                                        case 'D':
                                        {
                                            ProcessKeyboardMessage(&NewKeyboard->LeftStickX, WentDown, 1.0f);
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
                                                    ToggleFullscreen(Window);
                                                }
                                            } break;

                                            case VK_F4:
                                            {
                                                if(AltIsDown)
                                                {
                                                    GlobalRunning = false;
                                                }
                                            } break;

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
                                                            BOOL Result = WriteFile(RecordFile, &GameMemory,
                                                                                    sizeof(GameMemory),
                                                                                    &BytesWritten, &Overlapped);
                                                            Assert(Result && (BytesWritten == sizeof(GameMemory)));
                                                        }
                                                    } break;

                                                    case RecordingState_Recording:
                                                    {
                                                        RecordingState = RecordingState_PlayingRecord;
                                                        if(RecordFile)
                                                        {
                                                            DWORD BytesRead;
                                                            OVERLAPPED Overlapped = {};
                                                            BOOL Result = ReadFile(RecordFile, &GameMemory,
                                                                                   sizeof(GameMemory),
                                                                                   &BytesRead, &Overlapped);
                                                            Assert(Result && (BytesRead == sizeof(GameMemory)));
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

                    ProcessControllerInput(OldInput, NewInput);
                }

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
                            Result = ReadFile(RecordFile, &GameMemory,
                                              sizeof(GameMemory),
                                              &BytesRead, &Overlapped);
                            Assert(Result && (BytesRead == sizeof(GameMemory)));
                            Result = ReadFile(RecordFile, NewInput,
                                              sizeof(*NewInput),
                                              &BytesRead, 0);
                        }
                        Assert(Result && (BytesRead == sizeof(*NewInput)));
                    } break;
                }
                
                WIN32_FILE_ATTRIBUTE_DATA FileInfo;
                if(GetFileAttributesEx(DLLPath, GetFileExInfoStandard, &FileInfo))
                {
                    u64 LastWriteTime = (((u64)FileInfo.ftLastWriteTime.dwHighDateTime << 32) |
                                         (FileInfo.ftLastWriteTime.dwLowDateTime << 0));
                    if((LastWriteTime > CodeWriteTime) && !FileExists(PDBLockPath))
                    {
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
                        }
                        else
                        {
                            GameUpdateAndRender = 0;
                            GameGetSoundSamples = 0;
                        }
                    }
                }

                if(GameUpdateAndRender)
                {
                    GameUpdateAndRender(&GameMemory, NewInput, &GameBackBuffer);
                }
                if(GameGetSoundSamples && SecondarySoundBuffer)
                {
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
                    Assert(SUCCEEDED(SecondarySoundBuffer->Lock(ByteToLock, BytesToWrite,
                                                                &GameSoundBuffer.Region1, (LPDWORD)&GameSoundBuffer.Region1Size,
                                                                &GameSoundBuffer.Region2, (LPDWORD)&GameSoundBuffer.Region2Size,
                                                                0)));
                    GameGetSoundSamples(&GameMemory, GameInput, &GameSoundBuffer);
                    Assert(SUCCEEDED(SecondarySoundBuffer->Unlock(GameSoundBuffer.Region1, GameSoundBuffer.Region1Size,
                                                                  GameSoundBuffer.Region2, GameSoundBuffer.Region2Size)));
                    ByteToLock += BytesToWrite;
                    if(ByteToLock >= GameSoundBuffer.Size)
                    {
                        ByteToLock -= GameSoundBuffer.Size;
                    }
                }

                u32 RecordIconMargin = 10;
                u32 RecordIconRadius = 10;
                switch(RecordingState)
                {
                    case RecordingState_Recording:
                    {
                        u32 RecordIconRadius = 10;
                        u8 *PixelRow = ((u8 *)GameBackBuffer.Memory +
                                        GameBackBuffer.Pitch*(GameBackBuffer.Height -
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
                            PixelRow -= GameBackBuffer.Pitch;
                        }
                    } break;

                    case RecordingState_PlayingRecord:
                    {
                        u8 *PixelRow = ((u8 *)GameBackBuffer.Memory +
                                        GameBackBuffer.Pitch*(GameBackBuffer.Height -
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
                            PixelRow -= GameBackBuffer.Pitch;
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
                            PixelRow -= GameBackBuffer.Pitch;
                        }
                    } break;
                }

                QueryPerformanceCounter(&Counter);
                r32 FrameSeconds = (Counter.QuadPart - LastCounter.QuadPart)*ClocksToSeconds;
#if 0
                char Text[256];
                r32 ElapsedSeconds = FrameSeconds;
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
#if TROIDS_PROFILE
                    OutputDebugStringA("Missed framerate!\n");
#endif
                }
#endif
                
                HDC DeviceContext = GetDC(Window);
                CopyBackBufferToWindow(DeviceContext, &GlobalBackBuffer);
                ReleaseDC(Window, DeviceContext);

                // TODO(chris): Move debug event processing to game code
#if TROIDS_PROFILE
                debug_event *CodeBeginStack = PushArray(&GlobalDebugState->Arena,
                                                        MAX_DEBUG_EVENTS,
                                                        debug_event);
                u32 CodeBeginStackCount = 0;
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
                
                            _snprintf_s(Text, sizeof(Text), "%s %s %u %lu\n",
                                        BeginEvent->Name, BeginEvent->File,
                                        BeginEvent->Line, CyclesPassed);
                            OutputDebugStringA(Text);
                        } break;
                    }
                }
                GlobalDebugState->EventCount = 0;
                
                _snprintf_s(Text, sizeof(Text), "Frame time: %fms\n",
                            FrameSeconds*1000);
                OutputDebugStringA(Text);
#endif
                
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
