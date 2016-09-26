#if !defined(WIN32_TROIDS_FONT_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

internal void
Win32LoadFont(loaded_font *Font, HDC DeviceContext, char *FontName, DWORD FontHeight, DWORD FontWeight)
{
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
        SelectObject(FontDC, Win32Font);
        SetBkColor(FontDC, RGB(0, 0, 0));
        SetTextColor(FontDC, RGB(255, 255, 255));

        TEXTMETRIC Metrics;
        GetTextMetrics(FontDC, &Metrics);
        Font->Height = (r32)Metrics.tmHeight;
        Font->Ascent = (r32)Metrics.tmAscent;
        Font->LineAdvance = (Font->Height +
                                              (r32)Metrics.tmExternalLeading);

        char FirstChar = ' ';
        char LastChar = '~';
                
        ABCFLOAT ABCWidths[128];
        b32 GetWidthsResult = GetCharABCWidthsFloat(FontDC, FirstChar, LastChar, ABCWidths);
        Assert(GetWidthsResult);

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
                            
                Font->KerningTable[GlyphIndex][SecondGlyphIndex] =
                    FirstGlyphAdvance + SecondABCWidth->abcfA;
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

            loaded_bitmap *Glyph = Font->Glyphs + GlyphIndex;
            Glyph->Height = MaxY - MinY + 3;
            Glyph->Width = MaxX - MinX + 3;
            r32 Baseline = (r32)FontBuffer.Height-(r32)Metrics.tmAscent;
            Glyph->Align = {0.0f, (Baseline-(r32)MinY)/(r32)Glyph->Height};
            Glyph->Pitch = Glyph->Width*sizeof(u32);
            // TODO(chris): Don't do this. This should allocate from a custom asset
            // virtual memory system.
            Glyph->Memory = VirtualAlloc(0,
                                         sizeof(u32)*Glyph->Height*Glyph->Width,
                                         MEM_COMMIT|MEM_RESERVE,
                                         PAGE_READWRITE);

            u8 *SourceRow = (u8 *)FontBuffer.Memory + FontBuffer.Pitch*MinY;
            u8 *DestRow = (u8 *)Glyph->Memory + Glyph->Pitch;
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
                DestRow += Glyph->Pitch;
            }                    

            // NOTE(chris): Enable this to assert that the border around the glyph is pure alpha
#if 0
            u8 *Row = (u8 *)Glyph->Memory;
            for(s32 Y = 0;
                Y < Glyph->Height;
                ++Y)
            {
                u32 *Pixel = (u32 *)Row;
                for(s32 X = 0;
                    X < Glyph->Width;
                    ++X)
                {
                    if((Y == 0) ||
                       (Y == (Glyph->Height - 1)) ||
                       (X == 0) ||
                       (X == (Glyph->Width - 1)))
                    {
                        Assert(!(*Pixel));
                    }
                }
                Row += Glyph->Pitch;
            }
#endif
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

#define WIN32_TROIDS_FONT_H
#endif
