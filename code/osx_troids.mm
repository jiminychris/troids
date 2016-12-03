/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

// NOTE(chris): Used https://github.com/vbo/handmadehero_osx_platform_layer by Vadim Borodin to get started.
// Any comments that are not mine I altered to give the original author credit.

#include "osx_troids.h"


#include <mach/mach_time.h>

// NOTE(vadim): I am using mach_absolute_time() to implement a high-resolution timer.
// This function returns time in some CPU-dependent units, so we
// need to obtain and cache a coefficient info to convert it to something usable.
// TODO(vadim): maybe use seconds only to talk with game and use usec for calculations?

typedef uint64_t hrtime_t;

static hrtime_t hrtimeStartAbs;
static mach_timebase_info_data_t hrtimeInfo;
static const uint64_t NANOS_PER_USEC = 1000ULL;
static const uint64_t NANOS_PER_MILLISEC = 1000ULL * NANOS_PER_USEC;
static const uint64_t NANOS_PER_SEC = 1000ULL * NANOS_PER_MILLISEC;

// NOTE(vadim): Monotonic time in CPU-dependent units.
static hrtime_t 
OSXHRTime() 
{
   return mach_absolute_time() - hrtimeStartAbs; 
}

// NOTE(vadim): Delta in seconds between two time values in CPU-dependent units.
static inline double 
OSXHRTimeDeltaSeconds(hrtime_t past, hrtime_t future) 
{
    double delta = (double)(future - past)
                 * (double)hrtimeInfo.numer
                 / (double)NANOS_PER_SEC
                 / (double)hrtimeInfo.denom;
    return delta;
}

// NOTE(vadim): High-resolution sleep until moment specified by value
// in CPU-dependent units and offset in seconds.
// According to Apple docs this should be in at least 500usec precision.
static inline void osxHRSleepUntilAbsPlusSeconds(hrtime_t baseTime, double seconds) {
    uint64_t timeToWaitAbs = seconds
                           / (double)hrtimeInfo.numer
                           * (double)NANOS_PER_SEC
                           * (double)hrtimeInfo.denom;
    uint64_t nowAbs = mach_absolute_time();
    mach_wait_until(nowAbs + timeToWaitAbs);
}

// NOTE(vadim): Combines HRSleep with busy waiting for even more precise sleep.
static inline void osxHRWaitUntilAbsPlusSeconds(hrtime_t baseTime, double seconds) {
    while (1) {
        hrtime_t timeNow = OSXHRTime();
        double timeSinceBase = OSXHRTimeDeltaSeconds(baseTime, timeNow);
        double waitRemainder = seconds - timeSinceBase;
        // We can't sleep precise enough. According to Apple sleep
        // may vary by +500usec sometimes. So:
        // 1) We sleep at all only if we need to wait for more than 2ms
        // 2) We sleep for 1ms less then we need and busy-wait the rest
        if (waitRemainder > 0.002) {
            double timeToSleep = waitRemainder - 0.001;
            osxHRSleepUntilAbsPlusSeconds(timeNow, timeToSleep);
        }
        if (waitRemainder < 0.0001) {
            break;
        }
    }
}

#include <sys/stat.h>
#if 0

static void osxExeRelToAbsoluteFilename(
    char * fileName,
    uint32_t destSize, char *dest
) {
    int pathLength = globalApplicationState.exeFilePathEndPointer
                   - globalApplicationState.exeFileName;
    stringConcat(pathLength, globalApplicationState.exeFileName,
        stringLength(fileName), fileName, dest);
}

// NOTE(vadim): stat() function is actually part of POSIX API, nothing OS X specific here.
// TODO(vadim): maybe use NSFileManager instead? Which is better?
static int osxGetFileLastWriteTime(char *fileName) {
    int t = 0;
    struct stat fileStat;
    if (stat(fileName, &fileStat) == 0) {
        t = fileStat.st_mtimespec.tv_sec;
    } else {
        // TODO(vadim): Diagnostic
    }
    return t;
}
#endif

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

// NOTE(vadim): For some reason the game doesn't pass debug_file_read_result to
// DEBUG_PLATFORM_FREE_FILE_MEMORY implementation so we have no options other
// than malloc()/free() there. Both vm_deallocate and munmap take memory size
// as a parameter and looks like we can't just pass zero there like on win32.

// TODO(vadim): Add some error checking.
internal
PLATFORM_READ_FILE(OSXReadFile)
{   
    read_file_result Result = {};

    int File = open(FileName, O_RDONLY);

    if(File != -1)
    {
        struct stat FileStat;
        fstat(File, &FileStat);

        Result.ContentsSize = FileStat.st_size - Offset;
        Result.Contents = malloc(Result.ContentsSize);
        lseek(File, SEEK_SET, Offset);
        read(File, Result.Contents, Result.ContentsSize);
        close(File);
    }
    else
    {
        // TODO(chris): Diagnostics
        Assert(!"Failed to open file");
    }

    return(Result);
}

#include <dlfcn.h>
#include <mach-o/dyld.h>

// Basically API is the same as on Windows:
//  ----------------------------------------------
// | OS X                   | Windows             |
// |----------------------------------------------|
// | dlopen()               | LoadLibrary()       |
// | dlclose()              | FreeLibrary()       |
// | dlsym()                | GetProcAddress()    |
// | _NSGetExecutablePath() | GetModuleFileName() |
//  ----------------------------------------------

// Memory
// ======

#include <sys/mman.h>
#include <string.h>

inline static void * 
OSXMemoryAllocate(size_t Address, size_t Size) 
{
    // NOTE(vadim): Specify address 0 if yot don't care where
    // this memory is allocated. If you do care
    // about memory address use something like
    // 2 TB for address parameter.
    char *StartAddress = (char*)Address;
    int Prot = PROT_READ | PROT_WRITE;
    int Flags = MAP_PRIVATE | MAP_ANON;
    if (StartAddress != 0) 
    {
        Flags |= MAP_FIXED;
    }
    void *Result = (uint8_t *)mmap(StartAddress, Size, Prot, Flags, -1, 0);
    return(Result);
}

inline static void 
OSXMemoryCopy(void *dst, void *src, size_t size) 
{
    memcpy(dst, src, size);
}

/* NOTE(vadim): I am trying to use a minimal set of Cocoa features here
   to do things in the spirit of original Handmade Hero.
   E.g. I am rolling my own main event processing loop
   instead of one defined by NSApplication::run()
   to have more control over timing and such.
    
   What I am using to achive this is a sort of hack
   I learned from GLWF library source code:
    
   1) Create NSApplication
   2) Configure it to use our custom NSApplicationDelegate
   3) Issue [NSApp run]
   4) From our NSApplicationDelegate::applicationDidFinishLaunching() issue [NSApp stop]
   
   After this is done we get an ability to implement our own
   main loop effectively ignoring the whole Cocoa event dispatch system.
   The point of calling NSApp::run() at all is to allow Cocoa to perform
   any initialization it wants using it's existing code. Some stuff
   does not work well if we skip this step.
 */

@interface ApplicationDelegate
    : NSObject <NSApplicationDelegate, NSWindowDelegate> @end


@implementation ApplicationDelegate : NSObject
    - (void)applicationDidFinishLaunching: (NSNotification *)notification {
        // NOTE(vadim): Stop the event loop after app initialization:
        // I'll make my own loop later.
        [NSApp stop: nil];
        // NOTE(vadim): Post empty event: without it we can't put application to front
        // for some reason (I get this technique from GLFW source).
        NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
        NSEvent* event =
            [NSEvent otherEventWithType: NSApplicationDefined
                     location: NSMakePoint(0, 0)
                     modifierFlags: 0
                     timestamp: 0
                     windowNumber: 0
                     context: nil
                     subtype: 0
                     data1: 0
                     data2: 0];
        [NSApp postEvent: event atStart: YES];
        [pool drain];
    }

    - (NSApplicationTerminateReply)
      applicationShouldTerminate: (NSApplication *)sender {
        GlobalRunning = false;
        return NO;
    }

    - (void)dealloc {
        [super dealloc];
    }
@end

// NOTE(vadim): We can handle most events right from the main loop but some events
// like window focus (aka windowDidBecomeKey) could only be handled
// by custom NSWindowDelegate. Or maybe I am missing something?

@interface WindowDelegate:
    NSObject <NSWindowDelegate> @end

@implementation WindowDelegate : NSObject
    - (BOOL)windowShouldClose: (id)sender {
        GlobalRunning = false;
        return NO;
    }

    - (void)windowDidBecomeKey: (NSNotification *)notification {
#if DEBUG_TURN_OVERLAY_ON_UNFOCUS
        NSWindow *window = [notification object];
        [window setLevel: NSNormalWindowLevel];
        [window setAlphaValue: 1];
        [window setIgnoresMouseEvents: NO];
#endif
    }

    - (void)windowDidResignKey: (NSNotification *)notification {
#if DEBUG_TURN_OVERLAY_ON_UNFOCUS
        NSWindow *window = [notification object];
        [window setLevel: NSMainMenuWindowLevel];
        [window setAlphaValue: DEBUG_OVERLAY_OPACITY];
        [window setIgnoresMouseEvents: YES];
#endif
    }
@end

union osx_keyboard_state
{
    
    game_button Buttons[8];
    struct
    {
        game_button W;
        game_button A;
        game_button S;
        game_button D;
        game_button Up;
        game_button Left;
        game_button Down;
        game_button Right;
    };
};

global_variable game_controller *GlobalNewKeyboard;
global_variable osx_keyboard_state *GlobalNewOSXKeyboard;
global_variable b32 GlobalFocus = true;

static inline void 
CocoaProcessKeyUpDown(game_button *Button, int IsDown) 
{
    if(Button->EndedDown != IsDown) {
        Button->EndedDown = IsDown;
        ++Button->HalfTransitionCount;
    }
}

inline void
ProcessButtonInput(game_button *Button, b32 WentDown)
{
    Assert(WentDown == 0 || WentDown == 1);
    Assert(Button->EndedDown == 0 || Button->EndedDown == 1);
    if(WentDown != Button->EndedDown)
    {
        Button->EndedDown = !Button->EndedDown;
        ++Button->HalfTransitionCount;
    }
}

static inline void 
CocoaToggleFullscreen(NSWindow *Window, NSRect MonitorRect)
{
    if([Window styleMask] == NSBorderlessWindowMask) 
    {
        [Window setLevel: NSNormalWindowLevel];
        [Window setStyleMask: NSClosableWindowMask
                            | NSMiniaturizableWindowMask
                            | NSTitledWindowMask
                            | NSResizableWindowMask];
        [Window setHidesOnDeactivate: NO];
        [Window setFrame: PreviousWindowRect
                display: NO];
    } 
    else 
    {
        PreviousWindowRect = Window.frame;
        [Window setStyleMask: NSBorderlessWindowMask];
        [Window setLevel: NSMainMenuWindowLevel + 1];
        [Window setHidesOnDeactivate: YES];
        [Window setContentSize: MonitorRect.size];
        [Window setFrameOrigin: MonitorRect.origin];
    }
}

// Handling Gamepad Input
// ======================

#if 0
#include <IOKit/hid/IOHIDLib.h>
#include <Kernel/IOKit/hidsystem/IOHIDUsageTables.h>

// I am using IOKit IOHID API to handle gamepad input.
// The main idea is to use IOHIDManager API to subscribe for
// device plug-in events. When some compatible device plug-ins
// we choose an empty controller slot for it and subscribe for
// actual USB HID value change notifications (button presses
// and releases, stick movement etc). When device gets unplugged
// we mark the corresponding controller slot as disconnected
// and unsubscribe from all related events.

// Check out Apple's HID Class Device Interface Guide
// for official recommendations.

// TODO: I've tested this only with PS3 controller!
// Obviously there should be some controller specific nuances here
// and we need to decide how to handle this properly.
// TODO: Recheck deadzone handling.

static void iokitControllerValueChangeCallbackImpl(
    void *context, IOReturn result,
    void *sender, IOHIDValueRef valueRef
) {
    game_controller *controller = (game_controller *)context;
    if (IOHIDValueGetLength(valueRef) > 2) {
        return;
    }
    IOHIDElementRef element = IOHIDValueGetElement(valueRef);
    if (CFGetTypeID(element) != IOHIDElementGetTypeID()) {
        return;
    }
    int usagePage = IOHIDElementGetUsagePage(element);
    int usage = IOHIDElementGetUsage(element);
    CFIndex value = IOHIDValueGetIntegerValue(valueRef);
    // Parsing usagePage/usage/value according to USB HID Usage Tables spec.
    switch (usagePage) {
        case kHIDPage_GenericDesktop: { // Sticks handling.
            float valueNormalized;
            int inDeadZone = false;
            int deadZoneMin = 120;
            int deadZoneMax = 133;
            int center = 128;
            if (value > deadZoneMin && value < deadZoneMax) {
                valueNormalized = 0;
                inDeadZone = true;
            } else {
                if (value > center) {
                    valueNormalized = (float)(value - center + 1) / (float)center;
                } else {
                    valueNormalized = (float)(value - center) / (float)center;
                }
            }
            switch (usage) {
                case kHIDUsage_GD_X: {
                    if (!inDeadZone) {
                        //controller->IsAnalog = true;
                    }
                    controller->LeftStick.x = valueNormalized;
                } break;
                case kHIDUsage_GD_Y: {
                    if (!inDeadZone) {
                        //controller->IsAnalog = true;
                    }
                    controller->LeftStick.y = valueNormalized;
                } break;
            }
        } break;
        case kHIDPage_Button: {
            int isDown = (value != 0);
            switch (usage) {
                case 1: { // Select
                    cocoaProcessKeyUpDown(&controller->Select, isDown);
                } break;
                case 4: { // Start
                    cocoaProcessKeyUpDown(&controller->Start, isDown);
                } break;
                case 5: { // D-pad Up
                    //cocoaProcessKeyUpDown(&controller->MoveUp, isDown);
                    //controller->IsAnalog = false;
                } break;
                case 8: { // D-pad Left
                    //cocoaProcessKeyUpDown(&controller->MoveLeft, isDown);
                    //controller->IsAnalog = false;
                } break;
                case 7: { // D-pad Down
                    //cocoaProcessKeyUpDown(&controller->MoveDown, isDown);
                    //controller->IsAnalog = false;
                } break;
                case 6: { // D-pad Right
                    //cocoaProcessKeyUpDown(&controller->MoveRight, isDown);
                    //controller->IsAnalog = false;
                } break;
                case 13: { // Triangle
                    cocoaProcessKeyUpDown(&controller->ActionUp, isDown);
                } break;
                case 16: { // Square
                    cocoaProcessKeyUpDown(&controller->ActionLeft, isDown);
                } break;
                case 15: { // Cross
                    cocoaProcessKeyUpDown(&controller->ActionDown, isDown);
                } break;
                case 14: { // Circle
                    cocoaProcessKeyUpDown(&controller->ActionRight, isDown);
                } break;
                default: {
                    // Uncomment this to learn you gamepad buttons:
                    //NSLog(@"Unhandled button %d", usage);
                } break;
            }
        } break;
    }
    CFRelease(valueRef);
}

static void iokitControllerUnplugCallbackImpl(void* context, IOReturn result, void* sender) {
    game_controller *controller = (game_controller *)context;
    IOHIDDeviceRef device = (IOHIDDeviceRef)sender;
    controller->IsConnected = false;
    IOHIDDeviceRegisterInputValueCallback(device, 0, 0);
    IOHIDDeviceRegisterRemovalCallback(device, 0, 0);
    IOHIDDeviceUnscheduleFromRunLoop(
        device, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDDeviceClose(device, kIOHIDOptionsTypeNone);
}

static void iokitControllerPluginCallbackImpl(
    void* context, IOReturn result,
    void* sender, IOHIDDeviceRef device
) {
    // Find first free controller slot.
    game_input *input = (game_input *)context;
    game_controller * controller = 0;
    game_controller *controllers = input->Controllers;
    for (size_t i = 0; i < 5; ++i) {
        if (!controllers[i].IsConnected) {
            controller = &controllers[i];
            break;
        }
    }
    // All controller slots are occupied!
    if (!controller) {
        return;
    }
    controller->IsConnected = true;
    IOHIDDeviceOpen(device, kIOHIDOptionsTypeNone);
    IOHIDDeviceScheduleWithRunLoop(
        device, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    // Subscribe for this device events.
    // I am passing actual controller pointer as a callback
    // so we can distinguish between events from different
    // controllers.
    IOHIDDeviceRegisterInputValueCallback(
        device, iokitControllerValueChangeCallbackImpl, (void *)controller);
    IOHIDDeviceRegisterRemovalCallback(
        device, iokitControllerUnplugCallbackImpl, (void *)controller);
}

// Utility function to prepare input for IOHIDManager.
// Creates a CoreFoundation-style dictionary like this:
//   kIOHIDDeviceUsagePageKey => usagePage
//   kIOHIDDeviceUsageKey => usageValue
static CFMutableDictionaryRef iokitCreateDeviceMatchingDict(
    uint32_t usagePage, uint32_t usageValue
) {
    CFMutableDictionaryRef result = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);
    CFNumberRef pageNumber = CFNumberCreate(
        kCFAllocatorDefault, kCFNumberIntType, &usagePage);
    CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsagePageKey), pageNumber);
    CFRelease(pageNumber);
    CFNumberRef usageNumber = CFNumberCreate(
        kCFAllocatorDefault, kCFNumberIntType, &usageValue);
    CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsageKey), usageNumber);
    CFRelease(usageNumber);
    return result;
}

// Our job here is to just subscribe for device plug-in events
// for all device types we want to support.
static void iokitInit(game_input *input) {
    IOHIDManagerRef hidManager = IOHIDManagerCreate(
        kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    uint32_t matches[] = {
        kHIDUsage_GD_Joystick,
        kHIDUsage_GD_GamePad,
        kHIDUsage_GD_MultiAxisController
    };
    CFMutableArrayRef deviceMatching = CFArrayCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
    for (int i = 0; i < ARRAY_COUNT(matches); ++i) {
        CFDictionaryRef matching = iokitCreateDeviceMatchingDict(
            kHIDPage_GenericDesktop, matches[i]);
        CFArrayAppendValue(deviceMatching, matching);
        CFRelease(matching);
    }
    IOHIDManagerSetDeviceMatchingMultiple(hidManager, deviceMatching);
    CFRelease(deviceMatching);
    IOHIDManagerRegisterDeviceMatchingCallback(
        hidManager, iokitControllerPluginCallbackImpl, input);
    IOHIDManagerScheduleWithRunLoop(
        hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
}
#endif

#import <OpenGL/gl.h>

@interface OpenGLView: NSOpenGLView @end
@implementation OpenGLView : NSOpenGLView
    - (void)reshape {
        glViewport(0, 0, GlobalBackBuffer.MonitorWidth, GlobalBackBuffer.MonitorHeight);
    }
@end

internal NSOpenGLContext *
InitializeOpenGL(NSWindow *Window) 
{
    NSOpenGLContext *OpenGLContext = 0;
    NSOpenGLPixelFormatAttribute Attributes[] = 
    {
        // NOTE(vadim): Only accelerated renders.
        NSOpenGLPFAAccelerated,
        // NOTE(vadim): Indicates that the pixel format choosing policy is altered
        // for the color buffer such that the buffer closest to the requested size
        // is preferred, regardless of the actual color buffer depth
        // of the supported graphics device.
        NSOpenGLPFAClosestPolicy,
        // NOTE(vadim): Request double-buffered format.
        NSOpenGLPFADoubleBuffer,
        // NOTE(vadim): Request no multisampling.
        NSOpenGLPFASampleBuffers,
        0,
        // NOTE(vadim): This is just for end of array.
        0,
    };

    NSOpenGLPixelFormat *PixelFormat =
        [[NSOpenGLPixelFormat alloc] initWithAttributes: Attributes];
    OpenGLContext =
        [[NSOpenGLContext alloc] initWithFormat: PixelFormat
                                 shareContext: 0];
    if(OpenGLContext) 
    {
        // NOTE(vadim): We have only one context so just make it current here
        // and don't think about it to much.
        [OpenGLContext makeCurrentContext];
        // NOTE(vadim): Explicitly set back buffer resolution.
        // Without it back buffer will be automatically resized with window
        // so in fullscreen we'll be actually drawing to 1280x800 or something
        // regardless of our target resolution.
        // NOTE(vadim): what to do with game vs screen aspect ratio?
        // If we actually change video mode this would be handled automagically
        // but looks like it's anti-pattern in OS X world.
        GLint dims[] = {GlobalBackBuffer.Width, GlobalBackBuffer.Height};
        CGLSetParameter(OpenGLContext.CGLContextObj, kCGLCPSurfaceBackingSize, dims);
        CGLEnable(OpenGLContext.CGLContextObj, kCGLCESurfaceBackingSize);
        // NOTE(vadim): Substitute window's default contentView with OpenGL view
        // and configure it to use our newly created OpenGL context.
        OpenGLView *View = [[OpenGLView alloc] init];
        [Window setContentView: View];
        [View setOpenGLContext: OpenGLContext];
        [View setPixelFormat: PixelFormat];
        [OpenGLContext setView: View];

#if 1
        // NOTE(vadim): Prepare texture to draw our bitmap to.
        glEnable(GL_TEXTURE_2D);
        glClearColor(1, 0, 1, 1);
        glDisable(GL_DEPTH_TEST);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 1);
        // NOTE(vadim): Specify texture parameters.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        // NOTE(vadim): Allocate texture storage and specify how to read values from it.
        // Last NULL means that we don't have data right now and just want OpenGL
        // to allocate memory for the texture of a particular size.
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA,
            GlobalBackBuffer.Pitch/sizeof(u32), GlobalBackBuffer.Height,
            0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
#endif
    }
    else
    {
        // TODO(chris): diagnostics
        Assert(!"Failed to initialize Open GL context");
    }
    return OpenGLContext;
}

inline void
CopyBackBufferToWindow(NSOpenGLContext *OpenGLContext, osx_backbuffer *BackBuffer)
{
#if 1
    glTexSubImage2D(
        GL_TEXTURE_2D, 0, 0, 0,
        BackBuffer->Pitch/sizeof(u32), BackBuffer->Height,
        GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, BackBuffer->Memory);
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

    [OpenGLContext flushBuffer];
#else
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
        [OpenGLContext flushBuffer];
    }
#endif
}

void *
ThreadProc(void *Parameter)
{
    thread_context *Context = (thread_context *)Parameter;

    for(;;)
    {
        Context->Active = false;
        sem_wait(GlobalThreadQueueSemaphore);
        Context->Active = true;
        u32 WorkIndex = (AtomicIncrement(&GlobalThreadQueueStartIndex) &
                         (ArrayCount(GlobalThreadQueue)-1));
        thread_work Work = GlobalThreadQueue[WorkIndex];
        GlobalThreadQueue[WorkIndex].Free = true;

        Work.Callback(Work.Params);
        Work.Progress->Finished = true;
    }
}

internal 
PLATFORM_PUSH_THREAD_WORK(OSXPushThreadWork)
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
    sem_post(GlobalThreadQueueSemaphore);
}

internal 
PLATFORM_WAIT_FOR_ALL_THREAD_WORK(OSXWaitForAllThreadWork)
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

// Audio output
// ============
#if 0
#import <AudioUnit/AudioUnit.h>

// I am using CoreAudio AudioUnit API here. The main idea
// is to create an AudioUnit configured to output
// PCM data to the standard output device specifying
// so called render callback to be called when
// AudioUnit needs more audio data from the application.

// This callback will be called from the separated thread
// several times per frame so we need to buffer data
// we read from the game and read from this buffer
// from the callback.

// I am using a sort of DirectSound-like API with ring-buffer
// data structure and two pointers to it: playCursor and writeCursor.

typedef struct
{
    uint32_t isValid;
    uint32_t samplesPerSecond;
    uint32_t playCursor;
    uint32_t writeCursor;
    uint64_t runningSampleIndex;
    int16_t *buffer;
    uint32_t bufferSizeInSamples;
} CoreAudioOutputState;

// Actual callback implementation is a bit janky due to thread-safety
// issues and ring-buffer complexities.

#include "libkern/OSAtomic.h"

static OSStatus audioRenderCallbackImpl(
    void* inRefCon, AudioUnitRenderActionFlags* ioActionFlags,
    const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber,
    UInt32 inNumberFrames, AudioBufferList* ioData
) {
    CoreAudioOutputState *state = (CoreAudioOutputState *)inRefCon;
    int addWriteCursor = inNumberFrames;
    uint32_t newWriteCursor = state->writeCursor + inNumberFrames;
    if (newWriteCursor >= state->bufferSizeInSamples) {
        addWriteCursor = addWriteCursor - state->bufferSizeInSamples;
    }
    OSAtomicAdd32(addWriteCursor, (int32_t *)&state->writeCursor);
    AudioUnitSampleType *leftSamples = ioData->mBuffers[0].mData;
    AudioUnitSampleType *rightSamples = ioData->mBuffers[1].mData;
    for (int i = 0; i < inNumberFrames; ++i) {
        leftSamples[i] = (float)state->buffer[state->playCursor*2] / 32768.0;
        rightSamples[i] = (float)state->buffer[state->playCursor*2 + 1] / 32768.0;
        int addPlayCursor = 1;
        uint32_t newPlayCursor = state->playCursor + addPlayCursor;
        if (newPlayCursor == state->bufferSizeInSamples) {
            addPlayCursor = addPlayCursor - state->bufferSizeInSamples;
        }
        OSAtomicAdd32(addPlayCursor, (int32_t *)&state->playCursor);
    }
    return noErr;
}

static AudioUnit coreAudioCreateOutputUnit(CoreAudioOutputState *state) {
    AudioUnit outputUnit;
    // Find default AudioComponent.
    AudioComponent outputComponent; {
        AudioComponentDescription description = {
            kAudioUnitType_Output,
            kAudioUnitSubType_DefaultOutput,
            kAudioUnitManufacturer_Apple
        };
        outputComponent = AudioComponentFindNext(NULL, &description);
    }
    // Initialize output AudioUnit.
    AudioComponentInstanceNew(outputComponent, &outputUnit);
    AudioUnitInitialize(outputUnit);
    // Prepare stream description.
    AudioStreamBasicDescription audioFormat; {
        audioFormat.mSampleRate = (float)state->samplesPerSecond;
        audioFormat.mFormatID = kAudioFormatLinearPCM;
        // TODO: maybe switch to 16-bit PCM here?
        audioFormat.mFormatFlags = kAudioFormatFlagsAudioUnitCanonical;
        audioFormat.mFramesPerPacket = 1;
        audioFormat.mBytesPerFrame = sizeof(AudioUnitSampleType);
        audioFormat.mBytesPerPacket =
            audioFormat.mFramesPerPacket * audioFormat.mBytesPerFrame;
        audioFormat.mChannelsPerFrame = 2; // 2 for stereo
        audioFormat.mBitsPerChannel = 8 * sizeof(AudioUnitSampleType);
        audioFormat.mReserved = 0;
    }
    // Configure AudioUnit.
    AudioUnitSetProperty(
        outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input,
        0, &audioFormat, sizeof(audioFormat));
    // Specify RenderCallback.
    AURenderCallbackStruct renderCallback = {audioRenderCallbackImpl, state};
    AudioUnitSetProperty(
        outputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Global,
        0, &renderCallback, sizeof(renderCallback));
    return outputUnit;
}

static void coreAudioCopySamplesToRingBuffer(
    CoreAudioOutputState *state,
    game_sound_output_buffer *source,
    int samplesCount
) {
    int realWriteCursor = state->runningSampleIndex % state->bufferSizeInSamples;
    int sampleIndex = 0;
    for (int i = 0; i < samplesCount; ++i) {
        state->buffer[realWriteCursor*2] = source->Samples[sampleIndex++];
        state->buffer[realWriteCursor*2 + 1] = source->Samples[sampleIndex++];
        realWriteCursor++;
        realWriteCursor = realWriteCursor % state->bufferSizeInSamples;
        state->runningSampleIndex++;
    }
}
#endif

global_variable u32 GlobalSwapInterval;
global_variable r32 GlobalDtForFrame;
global_variable r32 GlobalMonitorRefreshPeriod;

#define internal static

internal b32
ChangeSwapInterval(NSOpenGLContext *OpenGLContext, u32 Interval)
{
    TIMED_FUNCTION();
    b32 Result = false;
    GlobalSwapInterval = Interval;
    GlobalDtForFrame = GlobalSwapInterval * GlobalMonitorRefreshPeriod;

    [OpenGLContext setValues: (s32 *)&Interval
                   forParameter: NSOpenGLCPSwapInterval];

    // TODO(chris): Can we assume success? Or should we getValues here to check?

    Result = true;

    return(Result);
}

#include <errno.h>

int main(int ArgCount, char **Args) 
{ 
    GlobalRunning = true;

    GlobalThreadQueueSemaphore = sem_open("TROIDS Semaphore", O_CREAT, S_IRUSR|S_IWUSR, 0);
    if(GlobalThreadQueueSemaphore == SEM_FAILED)
    {
        // TODO(chris): Diagnostics
        int Error = errno;
        Assert(!"Thread queue semaphore creation failed");
    }

    for(u32 ThreadWorkIndex = 0;
        ThreadWorkIndex < ArrayCount(GlobalThreadQueue);
        ++ThreadWorkIndex)
    {
        thread_work *Work = GlobalThreadQueue + ThreadWorkIndex;
        Work->Free = true;
    }

    NSApplication *Application = [NSApplication sharedApplication];
    // NOTE(vadim): In Snow Leopard, programs without application bundles and
    // Info.plist files don't get a menubar and can't be brought
    // to the front unless the presentation option is changed.
    [Application setActivationPolicy: NSApplicationActivationPolicyRegular];
    // NOTE(vadim): Specify application delegate impl.
    ApplicationDelegate *AppDelegate = [[ApplicationDelegate alloc] init];
    [Application setDelegate: AppDelegate];
    // NOTE(vadim): Normally this function would block, so if we want
    // to make our own main loop we need to stop it just
    // after initialization (see ApplicationDelegate implementation).
    [Application run];

    CGDirectDisplayID Monitor = CGMainDisplayID();
    s32 MonitorWidth = CGDisplayPixelsWide(Monitor);
    s32 MonitorHeight = CGDisplayPixelsHigh(Monitor);

    s32 BackBufferWidth = MonitorWidth;
    s32 BackBufferHeight = MonitorHeight;
    while(BackBufferWidth*BackBufferHeight > 1920*1080)
    {
        BackBufferWidth /= 2;
        BackBufferHeight /= 2;
    }
    s32 WindowWidth = MonitorWidth / 2;
    s32 WindowHeight = MonitorHeight / 2;
    NSRect MonitorRect = NSMakeRect(0, 0, MonitorWidth, MonitorHeight);
    NSRect WindowRect = NSMakeRect(0, 0, WindowWidth, WindowHeight);

    // NOTE(vadim): Creates window and minimalistic menu.
    // I have not found a way to change application name programmatically
    // so if you run this without application bundle you see something
    // like osx_handmade.out.
    // TODO(vadim): Is there a way to specify application name from code?
    GlobalWindow = [[NSWindow alloc] initWithContentRect: WindowRect
                                     styleMask: NSClosableWindowMask
                                     backing: NSBackingStoreBuffered
                                     defer: YES];

    int WindowStyleMask = NSClosableWindowMask;
    [GlobalWindow setOpaque: YES];
    WindowDelegate *WinDelegate = [[WindowDelegate alloc] init];
    [GlobalWindow setDelegate: WinDelegate];

    NSString *TROIDS = @"TROIDS";

    NSMenu *MenuBar = [NSMenu alloc];
    [GlobalWindow setTitle: TROIDS];
    NSMenuItem *AppMenuItem = [NSMenuItem alloc];
    [MenuBar addItem: AppMenuItem];
    [Application setMainMenu: MenuBar];
    NSMenu *AppMenu = [NSMenu alloc];
    NSString *QuitTitle = [@"Quit " stringByAppendingString: TROIDS];
    // NOTE(vadim): Make menu respond to cmd+q
    NSMenuItem *QuitMenuItem =
        [[NSMenuItem alloc] initWithTitle: QuitTitle
                            action: @selector(terminate:)
                            keyEquivalent: @"q"];
    [AppMenu addItem: QuitMenuItem];
    [AppMenuItem setSubmenu: AppMenu];
    // NOTE(vadim): When running from console we need to manually steal focus
    // from the terminal window for some reason.
    [Application activateIgnoringOtherApps:YES];

    // A cryptic way to ask window to open.
    [GlobalWindow makeKeyAndOrderFront: Application];

    // TODO(chris): ShowCursor(false);

    char *DataPath = DATA_PATH;
    chdir(DataPath);

    GlobalBackBuffer.Width = BackBufferWidth;
    GlobalBackBuffer.Height = BackBufferHeight;
    GlobalBackBuffer.MonitorWidth = MonitorWidth;
    GlobalBackBuffer.MonitorHeight = MonitorHeight;
    GlobalBackBuffer.Pitch = GlobalBackBuffer.Width * sizeof(u32);
    u32 BackBufferAllocationWidth = ((GlobalBackBuffer.Width+3)/4)*4;
    u32 BackBufferSize = BackBufferAllocationWidth*GlobalBackBuffer.Height*sizeof(u32);
    GlobalBackBuffer.Memory = OSXMemoryAllocate(0, (size_t)BackBufferSize);
    u8 *Pixel = (u8 *)GlobalBackBuffer.Memory;
    u32 BackBufferIndex = BackBufferSize;
    while(BackBufferIndex--) 
    {
        *Pixel++ = 0;
    }

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
    GameMemory.PermanentMemory = OSXMemoryAllocate(0, TotalMemorySize);
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

    GlobalThreadCount = Minimum(MAX_THREAD_COUNT, [[NSProcessInfo processInfo] processorCount] - 1);
    for(u32 ProcessorIndex = 0;
        ProcessorIndex < GlobalThreadCount;
        ++ProcessorIndex)
    {
        thread_context *Context = GlobalThreadContext + ProcessorIndex;
        u32 ThreadCreateError = pthread_create(&Context->Thread, 0, ThreadProc, Context);
        Assert(!ThreadCreateError);
#if TROIDS_INTERNAL
        u64 ThreadID = 0;
        u32 ThreadIDError = 0;
        // NOTE(chris): Threads don't have IDs until they enter their callback or something. 
        // I figure just spinning at startup is simpler than introducing more synchronization.
        while(!ThreadID)
        {
            ThreadIDError = pthread_threadid_np(Context->Thread, &ThreadID);
        }
        Assert(!ThreadIDError);
        GetThreadStorage(ThreadID);
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

    // TODO(chris): Load fonts (optionally bundle the bitmaps with the game)

    GameMemory.PlatformReadFile = OSXReadFile;
    GameMemory.PlatformPushThreadWork = OSXPushThreadWork;
    GameMemory.PlatformWaitForAllThreadWork = OSXWaitForAllThreadWork;

    // TODO(chris): Initialize audio

    // TODO(chris): Initialize input and latch game pads

    // TODO(chris): Read the exe footer? But Contents bundle will probably be the better option

    // NOTE(chris): No code reloading or looping. Primary dev is on PC and no point maintaining those on
    // multiple platforms.

    // NOTE(vadim): This should be called once before using any timing functions!
    // Check out Apple's "Technical Q&A QA1398 Mach Absolute Time Units"
    // for official recommendations.
    hrtimeStartAbs = mach_absolute_time();
    mach_timebase_info(&hrtimeInfo);

    hrtime_t LastCounter, Counter;

    game_input GameInput[2] = {};
    game_input *OldInput = GameInput;
    game_input *NewInput = GameInput + 1;

    osx_keyboard_state OSXKeyboard[2] = {};
    osx_keyboard_state *OldOSXKeyboard = OSXKeyboard;
    osx_keyboard_state *NewOSXKeyboard = OSXKeyboard + 1;

    r64 MonitorRefreshRate = CGDisplayModeGetRefreshRate(CGDisplayCopyDisplayMode(Monitor));
    if(MonitorRefreshRate == 0.0)
    {
        CVDisplayLinkRef DisplayLink;
        CVReturn CreateDisplayLinkResult = CVDisplayLinkCreateWithCGDisplay(Monitor, &DisplayLink);
        if((CreateDisplayLinkResult == kCVReturnSuccess) && DisplayLink)
        {
            CVTime RefreshPeriod = CVDisplayLinkGetNominalOutputVideoRefreshPeriod(DisplayLink);
            MonitorRefreshRate = (r64)RefreshPeriod.timeScale / (r64)RefreshPeriod.timeValue;
        }
        else
        {
            MonitorRefreshRate = 60.0;
        }
    }
    GlobalMonitorRefreshPeriod = 1.0f / (r32)MonitorRefreshRate;


    NSOpenGLContext *OpenGLContext = InitializeOpenGL(GlobalWindow);
    b32 VSYNC = ChangeSwapInterval(OpenGLContext, 1);

    // TODO(chris): Set initial keyboard state

    // TODO(chris): Set most recently used controller

    CocoaToggleFullscreen(GlobalWindow, MonitorRect);

    LastCounter = OSXHRTime();
    while(GlobalRunning)
    {                
        BEGIN_TIMED_BLOCK(FrameIndex, "Platform");
        {
            TIMED_BLOCK("Input");

            game_input *TempInput = NewInput;
            NewInput = OldInput;
            OldInput = TempInput;
            *NewInput = ZERO(game_input);
            NewInput->dtForFrame = GlobalDtForFrame;
            NewInput->SeedValue = LastCounter;
            NewInput->MostRecentlyUsedController = OldInput->MostRecentlyUsedController;

            game_controller *NewKeyboard = &NewInput->Keyboard;
            GlobalNewKeyboard = NewKeyboard;
            
            game_controller *OldKeyboard = &OldInput->Keyboard;
            for(u32 ButtonIndex = 0;
                ButtonIndex < ArrayCount(NewKeyboard->Buttons);
                ++ButtonIndex)
            {
                game_button *NewButton = NewKeyboard->Buttons + ButtonIndex;
                game_button *OldButton = OldKeyboard->Buttons + ButtonIndex;
                NewButton->EndedDown = OldButton->EndedDown;
            }

            osx_keyboard_state *TempOSXKeyboard = NewOSXKeyboard;
            NewOSXKeyboard = OldOSXKeyboard;
            OldOSXKeyboard = TempOSXKeyboard;
            *NewOSXKeyboard = ZERO(osx_keyboard_state);
            GlobalNewOSXKeyboard = NewOSXKeyboard;
            
            for(u32 ButtonIndex = 0;
                ButtonIndex < ArrayCount(NewOSXKeyboard->Buttons);
                ++ButtonIndex)
            {
                game_button *NewButton = NewOSXKeyboard->Buttons + ButtonIndex;
                game_button *OldButton = OldOSXKeyboard->Buttons + ButtonIndex;
                NewButton->EndedDown = OldButton->EndedDown;
            }
            
            NewKeyboard->RightStick = OldKeyboard->RightStick;
            NewKeyboard->Type = ControllerType_Keyboard;
            GlobalLeftMouse.HalfTransitionCount = 0;

            b32 WasFocused = GlobalFocus;

            // NOTE(vadim): Flushes event queue, handling most of the events in place.
            // This should be called every frame or our window become unresponsive!
            // TODO(vadim): For some reason the whole thing hangs on window resize events: investigate.
            {
                NSAutoreleasePool *EventsAutoreleasePool = [[NSAutoreleasePool alloc] init];
                while (true) {
                    NSEvent* Event =
                        [Application nextEventMatchingMask: NSAnyEventMask
                                     untilDate: [NSDate distantPast]
                                     inMode: NSDefaultRunLoopMode
                                     dequeue: YES];
                    if (!Event) {
                        break;
                    }
                    switch ([Event type]) {
                        case NSKeyUp:
                        case NSKeyDown: 
                        {
                            int HotkeyMask = NSCommandKeyMask
                                           | NSAlternateKeyMask
                                           | NSControlKeyMask
                                           | NSAlphaShiftKeyMask;
                            if ([Event modifierFlags] & HotkeyMask) 
                            {
                                // NOTE(vadim): Handle events like cmd+q etc
                                [Application sendEvent:Event];
                                break;
                            }

                            int WentDown = ([Event type] == NSKeyDown);
                            switch ([Event keyCode]) 
                            {
                                case VK_SPACE:
                                {
                                    ProcessButtonInput(&NewKeyboard->ActionDown, WentDown);
                                } break;

                                case VK_RETURN:
                                {
                                    ProcessButtonInput(&NewKeyboard->Start, WentDown);
                                } break;

                                case VK_ESCAPE:
                                {
                                    ProcessButtonInput(&NewKeyboard->Select, WentDown);
                                } break;

                                case VK_W:
                                { 
                                    ProcessButtonInput(&NewOSXKeyboard->W, WentDown);
                                } break;

                                case VK_A:
                                {
                                    ProcessButtonInput(&NewOSXKeyboard->A, WentDown);
                                } break;

                                case VK_S:
                                {
                                    ProcessButtonInput(&NewOSXKeyboard->S, WentDown);
                                } break;

                                case VK_D:
                                {
                                    ProcessButtonInput(&NewOSXKeyboard->D, WentDown);
                                } break;

                                case VK_UP:
                                {
                                    ProcessButtonInput(&NewOSXKeyboard->Up, WentDown);
                                } break;

                                case VK_LEFT:
                                {
                                    ProcessButtonInput(&NewOSXKeyboard->Left, WentDown);
                                } break;

                                case VK_DOWN:
                                {
                                    ProcessButtonInput(&NewOSXKeyboard->Down, WentDown);
                                } break;
                            
                                case VK_RIGHT:
                                {
                                    ProcessButtonInput(&NewOSXKeyboard->Right, WentDown);
                                } break;

                                case VK_F1: 
                                {
                                    if(WentDown) 
                                    {
                                        CocoaToggleFullscreen(GlobalWindow, MonitorRect);
                                    }
                                } break;

                                default: 
                                {
                                    // NOTE(vadim): Uncomment to learn your keys:
                                    //NSLog(@"Unhandled key: %d", [event keyCode]);
                                } break;
                            }

                            // TODO(chris): Ctrl-Cmd-F for fullscreen? This is what Mac apps do.
                            // TODO(chris): Also, Cmd-Enter for fullscreen? Like DX.
                            // TODO(chris): Alt-F4 for close? Probably not, Cmd-Q does this.
                        } break;

                        case NSLeftMouseUp:
                        {
                            TIMED_BLOCK("Handle Mouse Up");
                            ProcessButtonInput(&GlobalLeftMouse, false);
                        } break;

                        case NSLeftMouseDown:
                        {
                            TIMED_BLOCK("Handle Mouse Down");
                            ProcessButtonInput(&GlobalLeftMouse, true);
                        } break;

                        default: 
                        {
                            // NOTE(vadim): Handle events like app focus/unfocus etc
                            [Application sendEvent:Event];
                        } break;
                    }
                }
                [EventsAutoreleasePool drain];
            }

            NSPoint MouseP = [NSEvent mouseLocation];
            GlobalMousePosition = V2i(MouseP.x, MouseP.y);

            if(NewOSXKeyboard->W.EndedDown)
            {
                NewKeyboard->LeftStick.y += 1.0f;
            }
            if(NewOSXKeyboard->A.EndedDown)
            {
                NewKeyboard->LeftStick.x -= 1.0f;
            }
            if(NewOSXKeyboard->S.EndedDown)
            {
                NewKeyboard->LeftStick.y -= 1.0f;
            }
            if(NewOSXKeyboard->D.EndedDown)
            {
                NewKeyboard->LeftStick.x += 1.0f;
            }
            if(NewOSXKeyboard->Up.EndedDown)
            {
                NewKeyboard->LeftStick.y += 1.0f;
            }
            if(NewOSXKeyboard->Left.EndedDown)
            {
                NewKeyboard->LeftStick.x -= 1.0f;
            }
            if(NewOSXKeyboard->Down.EndedDown)
            {
                NewKeyboard->LeftStick.y -= 1.0f;
            }
            if(NewOSXKeyboard->Right.EndedDown)
            {
                NewKeyboard->LeftStick.x += 1.0f;
            }
            NewKeyboard->LeftStick.x = Clamp(-1.0f, NewKeyboard->LeftStick.x, 1.0f);
            NewKeyboard->LeftStick.y = Clamp(-1.0f, NewKeyboard->LeftStick.y, 1.0f);

            // TODO(chris): Need to see how Mac handles controllers on lost focus
            #if 0
            if(GlobalFocus)
            {
                ProcessGamePadInput(OldInput, NewInput);
                if(!WasFocused)
                {
                    for(u32 GamePadIndex = 0;
                        GamePadIndex < ArrayCount(NewInput->GamePads);
                        ++GamePadIndex)
                    {
                        game_controller *GamePad = NewInput->GamePads + GamePadIndex;
                        for(u32 ButtonIndex = 0;
                            ButtonIndex < ArrayCount(GamePad->Buttons);
                            ++ButtonIndex)
                        {
                            game_button *Button = GamePad->Buttons + ButtonIndex;
                            Button->HalfTransitionCount = 0;
                        }
                    }
                }
            }
            #endif

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
            NewInput->MousePosition = V2i(BackBufferWidth*((r32)GlobalMousePosition.x / (r32)MonitorWidth),
                                       BackBufferHeight*((r32)GlobalMousePosition.y / (r32)MonitorHeight));
            NewInput->LeftMouse = GlobalLeftMouse;
        }

        END_TIMED_BLOCK(FrameIndex);

        GameUpdateAndRender(&GameMemory, NewInput, &RendererState);
        GlobalRunning &= !NewInput->Quit;

        DebugCollate(&GameMemory, NewInput, &RendererState);

        CopyBackBufferToWindow(OpenGLContext, &GlobalBackBuffer);

        hrtime_t Counter = OSXHRTime();
        r32 FrameSeconds = OSXHRTimeDeltaSeconds(LastCounter, Counter);
        FRAME_MARKER(FrameSeconds);
        LastCounter = Counter;
    }
#if 0

    game_controller_input *keyboardController = &input.Controllers[0];
    // Keyboard should be connected before IOKit init
    // or we may accidentally assign controller to it's slot.
    keyboardController->IsConnected = true;
    iokitInit(&input);
    // Load game code dynamically so we can reload it later
    // to enable live-edit.
    OSXGameCode game = osxLoadGameCode(gameDylibFullPath);
    // Allocate game memory.
    game_memory gameMemory = {0}; {
        gameMemory.IsInitialized = 0;
        gameMemory.PermanentStorageSize = MEGABYTES(64);
        gameMemory.TransientStorageSize = GIGABYTES(1);
        int64_t startAddress = TERABYTES(2);
        globalApplicationState.gameMemoryBlockSize =
            gameMemory.PermanentStorageSize + gameMemory.TransientStorageSize;
        globalApplicationState.gameMemoryBlock = osxMemoryAllocate(
            startAddress, globalApplicationState.gameMemoryBlockSize);
        gameMemory.PermanentStorage = globalApplicationState.gameMemoryBlock;
        gameMemory.TransientStorage = (
            (uint8_t *)gameMemory.PermanentStorage + gameMemory.PermanentStorageSize);
#if TROIDS_INTERNAL
        gameMemory.DEBUGPlatformFreeFileMemory = osxDEBUGFreeFileMemory;
        gameMemory.DEBUGPlatformReadEntireFile = osxDEBUGReadEntireFile;
        gameMemory.DEBUGPlatformWriteEntireFile = osxDEBUGWriteEntireFile;
#endif
    }
    // Allocate sound buffers.
    int soundSamplesPerSecond = 48000;
     // 16-bit for each stereo channel
    int soundBytesPerSample = 2 * sizeof(int16_t);
    int bufferSizeInSamples = soundSamplesPerSecond;
    int soundBufferSizeInBytes = bufferSizeInSamples * soundBytesPerSample;
    CoreAudioOutputState soundOutputState = {0}; {
        soundOutputState.isValid = false;
        soundOutputState.samplesPerSecond = soundSamplesPerSecond;
        soundOutputState.playCursor = 0;
        // Introduce some safe initial latency.
        soundOutputState.writeCursor = 20.0/1000.0 * soundSamplesPerSecond;
        soundOutputState.bufferSizeInSamples = bufferSizeInSamples;
        soundOutputState.buffer = osxMemoryAllocate(0, soundBufferSizeInBytes);
        for (int i = 0; i < bufferSizeInSamples * 2; ++i) {
            soundOutputState.buffer[i] = 0;
        }
    }
    game_sound_output_buffer gameSoundBuffer = {0}; {
        gameSoundBuffer.SamplesPerSecond = soundSamplesPerSecond;
        gameSoundBuffer.SampleCount = 0;
        gameSoundBuffer.Samples = osxMemoryAllocate(0, soundBufferSizeInBytes); 
    }
    thread_context threadContext = {0};

    // Initialize audio output.
    AudioUnit outputUnit = coreAudioCreateOutputUnit(&soundOutputState);
    AudioOutputUnitStart(outputUnit);
    // Initialize input playback
    OSXInputPlaybackState inputPlayback;
    osxInitInputPlayback(&inputPlayback);

    uint32_t fpsMode = FPS_MODE_INITIAL;
    uint32_t targetFPS = fpsModes[fpsMode];
    int sleepSync = !ENABLE_VSYNC || (fpsMode == FPS_MODE_LOW);
    double targetFrameTime = (double)1.0/(double)targetFPS;
    // Start main loop.
    hrtime_t timeNow;
    double timeDeltaSeconds = 0;
    hrtime_t timeLast = osxHRTime();
    // For some reason several first iterations
    // are a bit slow so let's just skip them.
    int loopsToSkip = 3;
    while(GlobalRunning) 
    {
#if HANDMADE_INTERNAL
        int gameLastWrite = osxGetFileLastWriteTime(gameDylibFullPath);
        if (gameLastWrite > game.lastWrite) 
        {
            osxUnloadGameCode(&game);
            game = osxLoadGameCode(gameDylibFullPath);
        }
#endif


        if (!globalApplicationState.onPause) {
            if (!loopsToSkip) {
                input.dtForFrame = timeDeltaSeconds;
                if (inputPlayback.recordIndex) {
                    osxRecordInput(&inputPlayback, &input);
                }
                if (inputPlayback.playIndex) {
                    osxPlayInput(&inputPlayback, &input);
                }
                game.updateAndRender(
                    &threadContext, &gameMemory, &input, &framebuffer);
                // TODO: improve audio synchronization
                uint32_t writeCursor = soundOutputState.writeCursor;
                if (!soundOutputState.isValid) {
                    soundOutputState.runningSampleIndex = writeCursor;
                    soundOutputState.isValid = true;
                }
                uint32_t soundSamplesToRequest = timeDeltaSeconds * soundSamplesPerSecond;
                if (soundSamplesToRequest > soundOutputState.bufferSizeInSamples) {
                    soundSamplesToRequest = soundOutputState.bufferSizeInSamples;
                }
                gameSoundBuffer.SampleCount = soundSamplesToRequest;
                game.getSoundSamples(&threadContext, &gameMemory, &gameSoundBuffer);
                coreAudioCopySamplesToRingBuffer(
                    &soundOutputState, &gameSoundBuffer, soundSamplesToRequest);
                openglUpdateFramebuffer(&openglState, framebuffer.Memory);
                if (sleepSync) {
                    // Wait until the target frame boundary minus some safety margin
                    // to not oversleep the beginning of next refresh.
                    // TODO: maybe for low-end machines we need greater safety margin?
                    osxHRWaitUntilAbsPlusSeconds(timeLast, targetFrameTime - 0.001);
                }
            } else {
                loopsToSkip--;
            }
            openglFlushBuffer(&openglState);
        }
        // For framerates lower than refresh rate we
        // need to manually stabilize our frame times
        // to make animation smoother.
        if (sleepSync) {
            osxHRWaitUntilAbsPlusSeconds(timeLast, targetFrameTime);
        }
        // Update timer
        timeNow = osxHRTime();
        timeDeltaSeconds = osxHRTimeDeltaSeconds(timeLast, timeNow);
        timeLast = timeNow;
    }
#endif
    return 0;
}