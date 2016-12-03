/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include <windows.h>
#include <stdio.h>

#include "troids_platform.h"
#include "troids_intrinsics.h"
#include "troids_assets.h"
#include "troids_math.h"

struct asset_blob
{
    u32 Size;
    void *Memory;
};

struct win32_backbuffer
{
    s32 Width;
    s32 Height;
    s32 MonitorWidth;
    s32 MonitorHeight;
    s32 Pitch;
    BITMAPINFO Info;
    void *Memory;
};

global_variable u32 FontCount;
global_variable loaded_font Fonts[256];
global_variable u32 BitmapCount;
global_variable tra_bitmap Bitmaps[256];
global_variable asset_blob BitmapBlobs[256];
global_variable u32 AssetSize;

internal void
LoadBitmap(u32 BitmapID, void *Data)
{
    asset_blob *Blob = BitmapBlobs + BitmapID;
    tra_bitmap *Bitmap = Bitmaps + BitmapID;
    *Bitmap = {};
    bitmap_header *BMPHeader = (bitmap_header *)Data;

    Assert(BMPHeader->FileType == (('B' << 0) | ('M' << 8)));
    Assert(BMPHeader->Reserved1 == 0);
    Assert(BMPHeader->Reserved2 == 0);
    Assert(BMPHeader->Planes == 1);
    Assert(BMPHeader->BitsPerPixel == 32);
    Assert(BMPHeader->Compression == 3);
    Assert(BMPHeader->ColorsUsed == 0);
    Assert(BMPHeader->ColorsImportant == 0);

    Bitmap->Height = BMPHeader->Height;
    Bitmap->Width = BMPHeader->Width;
    Bitmap->Align = V2(0.5f, 0.5f);
    Bitmap->Pitch = Bitmap->Width*sizeof(u32);
    Blob->Size = Bitmap->Pitch*Bitmap->Height;
    Blob->Memory = (u8 *)BMPHeader + BMPHeader->BitmapOffset;

    u32 AlphaShift = BitScanForward(BMPHeader->AlphaMask).Index;
    u32 RedShift = BitScanForward(BMPHeader->RedMask).Index;
    u32 GreenShift = BitScanForward(BMPHeader->GreenMask).Index;
    u32 BlueShift = BitScanForward(BMPHeader->BlueMask).Index;

    u8 *PixelRow = (u8 *)Blob->Memory;
    r32 Inv255 = 1.0f / 255.0f;
    for(s32 Y = 0;
        Y < Bitmap->Height;
        ++Y)
    {
        u32 *Pixel = ((u32 *)PixelRow);
        for(s32 X = 0;
            X < Bitmap->Width;
            ++X)
        {
            v4 Color = V4i((*Pixel >> RedShift) & 0xFF,
                           (*Pixel >> GreenShift) & 0xFF,
                           (*Pixel >> BlueShift) & 0xFF,
                           (*Pixel >> AlphaShift) & 0xFF);

#if GAMMA_CORRECT
            Color = sRGB255ToLinear1(Color);
            // NOTE(chris): Pre-multiply alpha
            Color.rgb *= Color.a;
            Color = Linear1TosRGB255(Color);
#else
            Color.rgb *= Color.a*Inv255;
#endif
            u32 R = (u32)(Color.r + 0.5f);
            u32 G = (u32)(Color.g + 0.5f);
            u32 B = (u32)(Color.b + 0.5f);
            u32 A = (u32)(Color.a + 0.5f);

            *Pixel++ = ((A << 24) |
                        (R << 16) |
                        (G << 8) |
                        (B << 0));
        }
        PixelRow += Bitmap->Pitch;
    }
}

internal u32
StartBitmap()
{
    u32 Result = BitmapCount;
    Assert(Result < ArrayCount(Bitmaps));
    return(Result);
}

internal void
EndBitmap(u32 BitmapID)
{
    tra_bitmap *Bitmap = Bitmaps + BitmapID;
    Bitmap->MemoryOffset = AssetSize;
    AssetSize += Bitmap->Pitch*Bitmap->Height;

    ++BitmapCount;
}

internal void
PackBitmap(char *FileName)
{
    FILE *BitmapFile;
    
    errno_t FileOpenResult = fopen_s(&BitmapFile, "laser.bmp", "rb");
    Assert(FileOpenResult == 0);
    fseek(BitmapFile, 0, SEEK_END);
    u32 FileSize = ftell(BitmapFile);
    fseek(BitmapFile, 0, SEEK_SET);
    void *BitmapFileData = malloc(FileSize);
    fread(BitmapFileData, 1, FileSize, BitmapFile);

    u32 BitmapID = StartBitmap();
    LoadBitmap(BitmapID, BitmapFileData);
    EndBitmap(BitmapID);
}

internal void
PackFont(HDC DeviceContext, char *FontName, DWORD FontHeight, DWORD FontWeight,
         b32 Monospace)
{
    loaded_font *Font = Fonts + FontCount;
    ++FontCount;
    
    for(u32 GlyphIndex = 0;
        GlyphIndex < ArrayCount(Font->Glyphs);
        ++GlyphIndex)
    {
        Font->Glyphs[GlyphIndex] = {};
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

        int MapModePrevious = SetMapMode(DeviceContext, MM_TEXT);
        HFONT Win32Font = CreateFont(FontHeight, 0, // NOTE(chris): Height, Width
                                     0, // NOTE(chris): Escapement
                                     0, // NOTE(chris): Orientation
                                     FontWeight, // NOTE(chris): Weight
                                     0, // NOTE(chris): Italic
                                     0, // NOTE(chris): Underline
                                     0, // NOTE(chris): Strikeout
                                     ANSI_CHARSET,
                                     OUT_DEFAULT_PRECIS,
                                     CLIP_DEFAULT_PRECIS,
                                     ANTIALIASED_QUALITY,
                                     DEFAULT_PITCH|FF_DONTCARE,
                                     FontName);
        SetMapMode(DeviceContext, MapModePrevious);
        SelectObject(FontDC, Win32Font);
        SetBkColor(FontDC, RGB(0, 0, 0));
        SetTextColor(FontDC, RGB(255, 255, 255));

        TEXTMETRIC Metrics;
        GetTextMetrics(FontDC, &Metrics);
        Font->Height = (r32)Metrics.tmHeight;
        Font->Ascent = (r32)Metrics.tmAscent;
        Font->LineAdvance = (Font->Height + (r32)Metrics.tmExternalLeading);

        char FirstChar = ' ';
        char LastChar = '~';
                
        ABCFLOAT ABCWidths[128];
        b32 GetWidthsResult = GetCharABCWidthsFloat(FontDC, FirstChar, LastChar, ABCWidths);
        Assert(GetWidthsResult);

        ABCFLOAT *MABCWidth = ABCWidths + 'm' - FirstChar;
        r32 MaxCharWidth = MABCWidth->abcfB;

        r32 Inv255 = 1.0f / 255.0f;
        for(char GlyphIndex = FirstChar;
            GlyphIndex <= LastChar;
            ++GlyphIndex)
        {
            ABCFLOAT *FirstABCWidth = ABCWidths + GlyphIndex - FirstChar;
            r32 FirstGlyphAdvance = FirstABCWidth->abcfB + FirstABCWidth->abcfC;
            Font->KerningTable[GlyphIndex][0] = FirstGlyphAdvance;
            for(char SecondGlyphIndex = FirstChar;
                SecondGlyphIndex <= LastChar;
                ++SecondGlyphIndex)
            {
                ABCFLOAT *SecondABCWidth = ABCWidths + SecondGlyphIndex - FirstChar;

                if(Monospace)
                {
                    r32 FirstDiff = Maximum(0.0f, MaxCharWidth - FirstABCWidth->abcfB);
                    r32 SecondDiff = Maximum(0.0f, MaxCharWidth - SecondABCWidth->abcfB);
                    Font->KerningTable[GlyphIndex][SecondGlyphIndex] =
                        FirstDiff/2 + FirstABCWidth->abcfB + SecondDiff/2;
                }
                else
                {
                    Font->KerningTable[GlyphIndex][SecondGlyphIndex] =
                        FirstGlyphAdvance + SecondABCWidth->abcfA;
                }
            }
            if(GlyphIndex == ' ') continue;

            SIZE GlyphSize;
            b32 GetTextExtentResult = GetTextExtentPoint32(FontDC, &GlyphIndex, 1, &GlyphSize);
            Assert(GetTextExtentResult);
            b32 TextOutResult = TextOutA(FontDC, 128, 0, &GlyphIndex, 1);
            Assert(TextOutResult);

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

            u32 BitmapID = StartBitmap();
            
            Font->Glyphs[GlyphIndex] = {BitmapID};
            tra_bitmap *GlyphBitmap = Bitmaps + BitmapID;
            asset_blob *Blob = BitmapBlobs + BitmapID;
            
            GlyphBitmap->Height = MaxY - MinY + 3;
            GlyphBitmap->Width = MaxX - MinX + 3;
            r32 Baseline = (r32)FontBuffer.Height-(r32)Metrics.tmAscent;
            GlyphBitmap->Align = {0.0f, (Baseline-(r32)MinY)/(r32)GlyphBitmap->Height};
            GlyphBitmap->Pitch = GlyphBitmap->Width*sizeof(u32);

            Blob->Size = GlyphBitmap->Pitch*GlyphBitmap->Height;
            Blob->Memory = VirtualAlloc(0,
                                        Blob->Size,
                                        MEM_COMMIT|MEM_RESERVE,
                                        PAGE_READWRITE);

            u8 *SourceRow = (u8 *)FontBuffer.Memory + FontBuffer.Pitch*MinY;
            u8 *DestRow = (u8 *)Blob->Memory + GlyphBitmap->Pitch;
            for(s32 Y = MinY;
                Y <= MaxY;
                ++Y)
            {
                u32 *Source = (u32 *)SourceRow + MinX;
                u32 *Dest = (u32 *)DestRow + 1;
                for(s32 X = MinX;
                    X <= MaxX;
                    ++X)
                {
                    u32 Gray = (*Source & 0xFF);
                    // NOTE(chris): Pre-multiply alpha
                    v4 Color = V4i(Gray, Gray, Gray, Gray);
#if GAMMA_CORRECT
                    // NOTE(chris): If I assume that Gray is an alpha value, colors look wrong.
                    // So I'm going to assume that it's an sRGB color.
                    v4 LinearColor = sRGB255ToLinear1(Color);
                    LinearColor.a = LinearColor.r;
                    Color = Linear1TosRGB255(LinearColor);
#endif

                    u32 R = (u32)(Color.r + 0.5f);
                    u32 G = (u32)(Color.g + 0.5f);
                    u32 B = (u32)(Color.b + 0.5f);
                    u32 A = (u32)(Color.a + 0.5f);
                    
                    *Dest++ = ((A << 24) |
                               (R << 16) |
                               (G << 8)  |
                               (B << 0));
                    *Source++ = 0;
                }
                SourceRow += FontBuffer.Pitch;
                DestRow += GlyphBitmap->Pitch;
            }

            // NOTE(chris): Enable this to assert that the border around the glyph is pure alpha
#if 1
            u8 *Row = (u8 *)Blob->Memory;
            for(s32 Y = 0;
                Y < GlyphBitmap->Height;
                ++Y)
            {
                u32 *Pixel = (u32 *)Row;
                for(s32 X = 0;
                    X < GlyphBitmap->Width;
                    ++X)
                {
                    if((Y == 0) ||
                       (Y == (GlyphBitmap->Height - 1)) ||
                       (X == 0) ||
                       (X == (GlyphBitmap->Width - 1)))
                    {
                        Assert(!(*Pixel));
                    }
                }
                Row += GlyphBitmap->Pitch;
            }
#endif
            EndBitmap(BitmapID);
        }

        KERNINGPAIR KerningPairs[128];
        u32 KerningPairCount = GetKerningPairs(FontDC, ArrayCount(KerningPairs), 0);
        Assert(KerningPairCount <= ArrayCount(KerningPairs));
        if(KerningPairCount)
        {
            b32 GetKerningResult = GetKerningPairs(FontDC, KerningPairCount, KerningPairs);
            Assert(GetKerningResult);
            for(u32 KerningPairIndex = 0;
                KerningPairIndex < KerningPairCount;
                ++KerningPairIndex)
            {
                KERNINGPAIR *KerningPair = KerningPairs + KerningPairIndex;
                if((KerningPair->wFirst < ArrayCount(Font->KerningTable)) &&
                   (KerningPair->wSecond < ArrayCount(Font->KerningTable[0])))
                {
                    Font->KerningTable[KerningPair->wFirst][KerningPair->wSecond] +=
                        (r32)KerningPair->iKernAmount;
                }
            }
        }

        DeleteObject(FontBitmap);
        DeleteDC(FontDC);
    }
}

int main(int ArgCount, char **Args)
{
    char *DataPath = DATA_PATH;
    b32 ChangeDirectorySucceeded = SetCurrentDirectory(DataPath);
    Assert(ChangeDirectorySucceeded);

    FontCount = 1;
    BitmapCount = 1;
    AssetSize = 0;
    Fonts[0] = {};
    Bitmaps[0] = {};

    PackBitmap("laser.bmp");

    HDC DeviceContext = GetDC(0);
    PackFont(DeviceContext, "Arial",
             128, FW_NORMAL, false);
    // TODO(chris): Only pack debug font in internal builds?
//#if TROIDS_INTERNAL
    PackFont(DeviceContext, "Courier New",
             42, FW_BOLD, true);
//#endif

    FILE *OutFile;
    errno_t OutFileOpenResult = fopen_s(&OutFile, "assets.tra", "wb");
    Assert(OutFileOpenResult == 0);

    fwrite(&FontCount, sizeof(u32), 1, OutFile);
    fwrite(Fonts, sizeof(loaded_font), FontCount, OutFile);
    fwrite(&BitmapCount, sizeof(u32), 1, OutFile);
    fwrite(Bitmaps, sizeof(tra_bitmap), BitmapCount, OutFile);
    for(u32 BitmapIndex = 0;
        BitmapIndex < BitmapCount;
        ++BitmapIndex)
    {
        asset_blob *Blob = BitmapBlobs + BitmapIndex;
        fwrite(Blob->Memory, Blob->Size, 1, OutFile);
    }
    
    return 0;
}
