/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#define TEXT_Z 200000.0f
#define HUD_Z 100000.0f
#define DEBUG_BITMAPS 0

inline r32
GetTextAdvance(loaded_font *Font, char A, char B)
{
    r32 Result = Font->KerningTable[A][B];
    return(Result);
}

enum draw_text_flags
{
    DrawTextFlags_NoLineAdvance = 1 << 0,
    DrawTextFlags_Measure = 1 << 1,
};

// TODO(chris): Clean this up. Make fonts and font layout more systematic.
internal text_measurement
DrawText(render_buffer *RenderBuffer, text_layout *Layout, u32 TextLength, char *Text, u32 Flags = 0)
{
    text_measurement Result;
    v2 PInit = Layout->P;
    char Prev = 0;
    char *At = Text;
    r32 Ascent = Layout->Scale*Layout->Font->Ascent;
    r32 Height = Layout->Scale*Layout->Font->Height;
    r32 Descent = Height - Ascent;
    r32 LineAdvance = Layout->Scale*Layout->Font->LineAdvance;
    Result.TextMinY = REAL32_MAX;
    Result.TextMaxY = -REAL32_MAX;

    b32 Measure = IsSet(Flags, DrawTextFlags_Measure);
    for(u32 AtIndex = 0;
        AtIndex < TextLength;
        ++AtIndex)
    {
        loaded_bitmap *Glyph = Layout->Font->Glyphs + *At;
        if(Glyph->Height && Glyph->Width)
        {
            v2 XAxis = V2(1, 0);
            v2 YAxis = Perp(XAxis);
#if 0
            if(!Measure)
            {
                PushRectangle(&TranState->RenderBuffer, CenterDim(P, Scale*(XAxis + YAxis)),
                              V4(1.0f, 0.0f, 1.0f, 1.0f));
            }
#endif
            v2 Dim = Layout->Scale*V2((r32)Glyph->Width, (r32)Glyph->Height);
            if(Layout->DropShadow)
            {
                v3 P = V3(Layout->P + Height*V2(0.05f, -0.05f), TEXT_Z-1);
                if(!Measure)
                {
                    PushBitmap(RenderBuffer, Glyph, P,
                               XAxis, YAxis, Dim, V4(0, 0, 0, Layout->Color.a));
                }
                Result.TextMinY = Minimum(Result.TextMinY, P.y - Glyph->Align.y*Dim.y);
                Result.TextMaxY = Maximum(Result.TextMaxY, P.y + (1.0f-Glyph->Align.y)*Dim.y);
            }
            v3 P = V3(Layout->P, TEXT_Z);
            if(!Measure)
            {
                PushBitmap(RenderBuffer, Glyph, P, XAxis, YAxis, Dim, Layout->Color);
            }
            Result.TextMinY = Minimum(Result.TextMinY, P.y - Glyph->Align.y*Dim.y);
            Result.TextMaxY = Maximum(Result.TextMaxY, P.y + (1.0f-Glyph->Align.y)*Dim.y);
        }
        Prev = *At++;
        Layout->P.x += Layout->Scale*GetTextAdvance(Layout->Font, Prev, *At);
    }

    Result.BaseLine = PInit.y;
    Result.MinX = PInit.x;
    Result.LineMinY = Result.BaseLine - Descent;
    Result.MaxX = Layout->P.x;
    Result.LineMaxY = Result.BaseLine + Ascent;
        
    if(!IsSet(Flags, DrawTextFlags_NoLineAdvance))
    {
        Layout->P.x = PInit.x;
        Layout->P.y -= LineAdvance;
    }
    if(IsSet(Flags, DrawTextFlags_Measure))
    {
        Layout->P = PInit;
    }
    
    return(Result);
}

#if TROIDS_INTERNAL
internal rectangle2
DEBUGDrawButton(render_buffer *RenderBuffer, text_layout *Layout, u32 TextLength, char *Text,
           v4 Color = INVERTED_COLOR, r32 Border = 0.0f)
{
    rectangle2 Result;
    v2 PInit = Layout->P;
    Layout->P.x += Border;
    Layout->P.y -= Border;
    Result = AddRadius(GetLineRect(DrawText(RenderBuffer, Layout, TextLength, Text)), Border);
    PushRectangle(RenderBuffer, V3(Result.Min, HUD_Z), GetDim(Result), Color);
    
    return(Result);
}

internal void
DEBUGDrawFillBar(render_buffer *RenderBuffer, text_layout *Layout, r32 Used, r32 Max, u32 Precision = 0)
{
    char Text[256];
    u32 TextLength = _snprintf_s(Text, sizeof(Text), "%.*f / %.*f", Precision, Used, Precision, Max);
    rectangle2 TextRect = GetLineRect(DrawText(RenderBuffer, Layout, TextLength, Text));
    r32 Percentage = Used / Max;

    v2 SizeDim;
    v2 UsedDim;
    v2 P = TextRect.Min;
    SizeDim.y = UsedDim.y = GetDim(TextRect).y;
    SizeDim.x = RenderBuffer->Width - P.x - 10.0f;
    UsedDim.x = SizeDim.x*Percentage;
    PushRectangle(RenderBuffer, V3(P, HUD_Z + 1000), SizeDim, INVERTED_COLOR);
    PushRectangle(RenderBuffer, V3(P, HUD_Z + 2000), UsedDim, V4(0, 1, 0, 1));
}

internal void
DEBUGDrawFillBar(render_buffer *RenderBuffer, text_layout *Layout, u32 Used, u32 Max)
{
    DEBUGDrawFillBar(RenderBuffer, Layout, (r32)Used, (r32)Max);
}

internal void
DEBUGDrawFillBar(render_buffer *RenderBuffer, text_layout *Layout, u64 Used, u64 Max)
{
    DEBUGDrawFillBar(RenderBuffer, Layout, (r32)Used, (r32)Max);
}
#endif

#if 1
#pragma optimize("gts", on)
#endif
// TODO(chris): Further optimization
internal void
RenderBitmap(loaded_bitmap *BackBuffer, loaded_bitmap *Bitmap, v2 Origin, v2 XAxis, v2 YAxis,
             v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f), s32 OffsetX = 0, s32 OffsetY = 0)
{
    // TODO(chris): This should probably be called ColorModulation?
    // NOTE(chris): This color is just a linear multiply, so it's not sRGB
    // NOTE(chris): pre-multiply alpha
    v4 Tint = Color;
    Tint.rgb *= Tint.a;
    
    s32 XMin = Clamp(OffsetX, Floor(Minimum(Minimum(Origin.x, Origin.x + XAxis.x),
                                      Minimum(Origin.x + YAxis.x, Origin.x + XAxis.x + YAxis.x))),
                     OffsetX + BackBuffer->Width);
    s32 YMin = Clamp(OffsetY, Floor(Minimum(Minimum(Origin.y, Origin.y + XAxis.y),
                                      Minimum(Origin.y + YAxis.y, Origin.y + XAxis.y + YAxis.y))),
                     OffsetY + BackBuffer->Height);

    s32 XMax = Clamp(OffsetX, Ceiling(Maximum(Maximum(Origin.x, Origin.x + XAxis.x),
                                        Maximum(Origin.x + YAxis.x, Origin.x + XAxis.x + YAxis.x))),
                     OffsetX + BackBuffer->Width);
    s32 YMax = Clamp(OffsetY, Ceiling(Maximum(Maximum(Origin.y, Origin.y + XAxis.y),
                                        Maximum(Origin.y + YAxis.y, Origin.y + XAxis.y + YAxis.y))),
                     OffsetY + BackBuffer->Height);
    // TODO(chris): Crashed here once when drawing in the top right corner. Check that out.

    s32 AdjustedXMin = XMin;
    s32 AdjustedXMax = XMax;
    s32 Width = XMax-XMin;
    s32 Height = YMax-YMin;
    s32 Adjustment = 4-Width%4;
    if(Adjustment < 4)
    {
        AdjustedXMin = Maximum(OffsetX, XMin-Adjustment);
        Adjustment -= XMin-AdjustedXMin;
        AdjustedXMax = Minimum(OffsetX + BackBuffer->Width, XMax+Adjustment);
        Width = AdjustedXMax-AdjustedXMin;
    }

    __m128 One255 = _mm_set_ps1(255.0f);
    __m128 Inv255 = _mm_set_ps1(1.0f / 255.0f);
    __m128 InvXAxisLengthSq = _mm_set_ps1(1.0f / LengthSq(XAxis));
    __m128 InvYAxisLengthSq = _mm_set_ps1(1.0f / LengthSq(YAxis));
    __m128 BitmapInternalWidth = _mm_set_ps1((r32)(Bitmap->Width - 2));
    __m128 BitmapInternalHeight = _mm_set_ps1((r32)(Bitmap->Height - 2));
    __m128 BitmapWidth = _mm_set_ps1((r32)Bitmap->Width);
    __m128i BitmapHeight = _mm_set1_epi32(Bitmap->Height);
    __m128 OriginX = _mm_set_ps1(Origin.x);
    __m128 OriginY = _mm_set_ps1(Origin.y);
    __m128 XAxisX = _mm_set_ps1(XAxis.x);
    __m128 XAxisY = _mm_set_ps1(XAxis.y);
    __m128 YAxisX = _mm_set_ps1(YAxis.x);
    __m128 YAxisY = _mm_set_ps1(YAxis.y);
    __m128i One = _mm_set1_epi32(1);
    __m128 Four = _mm_set_ps1(4.0f);
    __m128 RealZero = _mm_set_ps1(0.0f);
    __m128 RealOne = _mm_set_ps1(1.0f);
    __m128 Half = _mm_set_ps1(0.5f);
    __m128i ColorMask = _mm_set1_epi32(0xFF);
    __m128 TintR = _mm_set_ps1(Tint.r);
    __m128 TintG = _mm_set_ps1(Tint.g);
    __m128 TintB = _mm_set_ps1(Tint.b);
    __m128 TintA = _mm_set_ps1(Tint.a);
    __m128 Mask;
    u8 *PixelRow = (u8 *)BackBuffer->Memory + (BackBuffer->Pitch*YMin);
    IGNORED_TIMED_LOOP_FUNCTION(Width*Height);
#if DEBUG_BITMAPS
    __m128i Pink = _mm_set1_epi32(0xFFFF00FF);
    __m128i Blue = _mm_set1_epi32(0xFF0000FF);
#endif
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Pixel = (u32 *)PixelRow + AdjustedXMin;
        __m128 X4 = _mm_set_ps((r32)AdjustedXMin + 3, (r32)AdjustedXMin + 2,
                                   (r32)AdjustedXMin + 1, (r32)AdjustedXMin + 0);
        __m128 Y4 = _mm_set_ps1((r32)Y);
        for(s32 X = AdjustedXMin;
            X < AdjustedXMax;
            X += 4)
        {
#if DEBUG_BITMAPS
            // TODO(chris): Ship bounding rectangle seems too big when rotating?
            _mm_storeu_si128((__m128i *)Pixel, Pink);
#endif
            __m128 TestPointX = _mm_sub_ps(X4, OriginX);
            __m128 TestPointY = _mm_sub_ps(Y4, OriginY);

            __m128 U = _mm_mul_ps(_mm_add_ps((_mm_mul_ps(TestPointX, XAxisX)),
                                             _mm_mul_ps(TestPointY, XAxisY)),
                                  InvXAxisLengthSq);
            __m128 V = _mm_mul_ps(_mm_add_ps((_mm_mul_ps(TestPointX, YAxisX)),
                                             _mm_mul_ps(TestPointY, YAxisY)),
                                  InvYAxisLengthSq);

            Mask = _mm_and_ps(_mm_and_ps(_mm_cmpge_ps(U, RealZero), _mm_cmpge_ps(V, RealZero)),
                              _mm_and_ps(_mm_cmple_ps(U, RealOne), _mm_cmple_ps(V, RealOne)));

            __m128 tX = _mm_add_ps(_mm_mul_ps(U, BitmapInternalWidth), Half);
            __m128 tY = _mm_add_ps(_mm_mul_ps(V, BitmapInternalHeight), Half);

            __m128 ttX = _mm_cvtepi32_ps(_mm_cvttps_epi32(_mm_and_ps(tX, Mask)));
            __m128 ttY = _mm_cvtepi32_ps(_mm_cvttps_epi32(_mm_and_ps(tY, Mask)));

            __m128 OffsetA = _mm_add_ps(_mm_mul_ps(ttY, BitmapWidth), ttX);
            __m128 OffsetB = _mm_add_ps(OffsetA, RealOne);
            __m128 OffsetC = _mm_add_ps(OffsetA, BitmapWidth);
            __m128 OffsetD = _mm_add_ps(OffsetC, RealOne);

            __m128i TexelA = _mm_set_epi32(*((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetA).m128i_u32[3]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetA).m128i_u32[2]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetA).m128i_u32[1]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetA).m128i_u32[0]));
            __m128i TexelB = _mm_set_epi32(*((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetB).m128i_u32[3]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetB).m128i_u32[2]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetB).m128i_u32[1]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetB).m128i_u32[0]));
            __m128i TexelC = _mm_set_epi32(*((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetC).m128i_u32[3]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetC).m128i_u32[2]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetC).m128i_u32[1]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetC).m128i_u32[0]));
            __m128i TexelD = _mm_set_epi32(*((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetD).m128i_u32[3]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetD).m128i_u32[2]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetD).m128i_u32[1]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetD).m128i_u32[0]));


            __m128 TexelAR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelA, 16), ColorMask));
            __m128 TexelBR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelB, 16), ColorMask));
            __m128 TexelCR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelC, 16), ColorMask));
            __m128 TexelDR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelD, 16), ColorMask));

            __m128 TexelAG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelA, 8), ColorMask));
            __m128 TexelBG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelB, 8), ColorMask));
            __m128 TexelCG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelC, 8), ColorMask));
            __m128 TexelDG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelD, 8), ColorMask));

            __m128 TexelAB = _mm_cvtepi32_ps(_mm_and_si128(TexelA, ColorMask));
            __m128 TexelBB = _mm_cvtepi32_ps(_mm_and_si128(TexelB, ColorMask));
            __m128 TexelCB = _mm_cvtepi32_ps(_mm_and_si128(TexelC, ColorMask));
            __m128 TexelDB = _mm_cvtepi32_ps(_mm_and_si128(TexelD, ColorMask));

            __m128 TexelAA = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelA, 24), ColorMask));
            __m128 TexelBA = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelB, 24), ColorMask));
            __m128 TexelCA = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelC, 24), ColorMask));
            __m128 TexelDA = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelD, 24), ColorMask));

#if GAMMA_CORRECT
            // NOTE(chris): sRGB to linear
            TexelAR = _mm_mul_ps(_mm_mul_ps(TexelAR, TexelAR), Inv255);
            TexelBR = _mm_mul_ps(_mm_mul_ps(TexelBR, TexelBR), Inv255);
            TexelCR = _mm_mul_ps(_mm_mul_ps(TexelCR, TexelCR), Inv255);
            TexelDR = _mm_mul_ps(_mm_mul_ps(TexelDR, TexelDR), Inv255);

            TexelAG = _mm_mul_ps(_mm_mul_ps(TexelAG, TexelAG), Inv255);
            TexelBG = _mm_mul_ps(_mm_mul_ps(TexelBG, TexelBG), Inv255);
            TexelCG = _mm_mul_ps(_mm_mul_ps(TexelCG, TexelCG), Inv255);
            TexelDG = _mm_mul_ps(_mm_mul_ps(TexelDG, TexelDG), Inv255);

            TexelAB = _mm_mul_ps(_mm_mul_ps(TexelAB, TexelAB), Inv255);
            TexelBB = _mm_mul_ps(_mm_mul_ps(TexelBB, TexelBB), Inv255);
            TexelCB = _mm_mul_ps(_mm_mul_ps(TexelCB, TexelCB), Inv255);
            TexelDB = _mm_mul_ps(_mm_mul_ps(TexelDB, TexelDB), Inv255);
#endif

            __m128 tU = _mm_sub_ps(tX, ttX);
            __m128 tV = _mm_sub_ps(tY, ttY);
            __m128 tUInv = _mm_sub_ps(RealOne, tU);
            __m128 tVInv = _mm_sub_ps(RealOne, tV);

            __m128 LerpR = _mm_and_ps(_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(TexelAR, tUInv), _mm_mul_ps(TexelBR, tU)), tVInv),
                                                 _mm_mul_ps(_mm_add_ps(_mm_mul_ps(TexelCR, tUInv), _mm_mul_ps(TexelDR, tU)), tV)), Mask);
            __m128 LerpG = _mm_and_ps(_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(TexelAG, tUInv), _mm_mul_ps(TexelBG, tU)), tVInv),
                                                 _mm_mul_ps(_mm_add_ps(_mm_mul_ps(TexelCG, tUInv), _mm_mul_ps(TexelDG, tU)), tV)), Mask);
            __m128 LerpB = _mm_and_ps(_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(TexelAB, tUInv), _mm_mul_ps(TexelBB, tU)), tVInv),
                                                 _mm_mul_ps(_mm_add_ps(_mm_mul_ps(TexelCB, tUInv), _mm_mul_ps(TexelDB, tU)), tV)), Mask);
            __m128 LerpA = _mm_and_ps(_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(TexelAA, tUInv), _mm_mul_ps(TexelBA, tU)), tVInv),
                                                 _mm_mul_ps(_mm_add_ps(_mm_mul_ps(TexelCA, tUInv), _mm_mul_ps(TexelDA, tU)), tV)), Mask);

            __m128i Pixels = _mm_loadu_si128((__m128i *)Pixel);

            __m128 DR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Pixels, 16), ColorMask));
            __m128 DG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Pixels, 8), ColorMask));
            __m128 DB = _mm_cvtepi32_ps(_mm_and_si128(Pixels, ColorMask));

#if GAMMA_CORRECT
            // NOTE(chris): sRGB to linear
            DR = _mm_mul_ps(_mm_mul_ps(DR, DR), Inv255);
            DG = _mm_mul_ps(_mm_mul_ps(DG, DG), Inv255);
            DB = _mm_mul_ps(_mm_mul_ps(DB, DB), Inv255);
#endif
            __m128 DA = _mm_sub_ps(RealOne, _mm_mul_ps(Inv255, _mm_mul_ps(LerpA, TintA)));
#if DEBUG_BITMAPS
            __m128i IntMask = _mm_set_epi32(Mask.m128_u32[3], Mask.m128_u32[2],
                                            Mask.m128_u32[1], Mask.m128_u32[0]);
            Pixels = _mm_or_si128(_mm_andnot_si128(IntMask, Pixels), _mm_and_si128(Blue, IntMask));
#endif

            __m128 ResultR = _mm_add_ps(_mm_mul_ps(TintR, LerpR), _mm_mul_ps(DA, DR));
            __m128 ResultG = _mm_add_ps(_mm_mul_ps(TintG, LerpG), _mm_mul_ps(DA, DG));
            __m128 ResultB = _mm_add_ps(_mm_mul_ps(TintB, LerpB), _mm_mul_ps(DA, DB));

#if GAMMA_CORRECT
            // NOTE(chris): linear to sRGB
            __m128 ResultR255 = _mm_mul_ps(One255, ResultR);
            __m128 ResultG255 = _mm_mul_ps(One255, ResultG);
            __m128 ResultB255 = _mm_mul_ps(One255, ResultB);

            ResultR = _mm_mul_ps(ResultR255, _mm_rsqrt_ps(ResultR255));
            ResultG = _mm_mul_ps(ResultG255, _mm_rsqrt_ps(ResultG255));
            ResultB = _mm_mul_ps(ResultB255, _mm_rsqrt_ps(ResultB255));
#endif

            __m128i Result = _mm_or_si128(
                _mm_or_si128(
                    _mm_slli_epi32(
                        _mm_cvtps_epi32(ResultR),
                        16),
                    _mm_slli_epi32(
                        _mm_cvtps_epi32(ResultG),
                        8)),
                _mm_cvtps_epi32(ResultB));

            _mm_storeu_si128((__m128i *)Pixel, Result);

            Pixel += 4;
            X4 = _mm_add_ps(X4, Four);
        }
        PixelRow += BackBuffer->Pitch;
    }
}
// TODO(chris): Further optimization
internal void
RenderBitmap(render_chunk *RenderChunk, loaded_bitmap *Bitmap, v2 Origin, v2 XAxis, v2 YAxis,
             v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f), s32 OffsetX = 0, s32 OffsetY = 0)
{
    // TODO(chris): This should probably be called ColorModulation?
    // NOTE(chris): This color is just a linear multiply, so it's not sRGB
    // NOTE(chris): pre-multiply alpha
    v4 Tint = Color;
    Tint.rgb *= Tint.a;
    
    loaded_bitmap *SampleBuffer = &RenderChunk->SampleBuffer;
    loaded_bitmap *CoverageBuffer = &RenderChunk->CoverageBuffer;
    s32 XMin = Clamp(OffsetX, Floor(Minimum(Minimum(Origin.x, Origin.x + XAxis.x),
                                      Minimum(Origin.x + YAxis.x, Origin.x + XAxis.x + YAxis.x))),
                     OffsetX + CoverageBuffer->Width);
    s32 YMin = Clamp(OffsetY, Floor(Minimum(Minimum(Origin.y, Origin.y + XAxis.y),
                                      Minimum(Origin.y + YAxis.y, Origin.y + XAxis.y + YAxis.y))),
                     OffsetY + CoverageBuffer->Height);

    s32 XMax = Clamp(OffsetX, Ceiling(Maximum(Maximum(Origin.x, Origin.x + XAxis.x),
                                        Maximum(Origin.x + YAxis.x, Origin.x + XAxis.x + YAxis.x))),
                     OffsetX + CoverageBuffer->Width);
    s32 YMax = Clamp(OffsetY, Ceiling(Maximum(Maximum(Origin.y, Origin.y + XAxis.y),
                                        Maximum(Origin.y + YAxis.y, Origin.y + XAxis.y + YAxis.y))),
                     OffsetY + CoverageBuffer->Height);
    // TODO(chris): Crashed here once when drawing in the top right corner. Check that out.

    s32 AdjustedXMin = (XMin/4)*4;
    s32 AdjustedXMax = ((XMax+3)/4)*4;

    __m128 One255 = _mm_set_ps1(255.0f);
    __m128 Inv255 = _mm_set_ps1(1.0f / 255.0f);
    __m128 InvXAxisLengthSq = _mm_set_ps1(1.0f / LengthSq(XAxis));
    __m128 InvYAxisLengthSq = _mm_set_ps1(1.0f / LengthSq(YAxis));
    __m128 BitmapInternalWidth = _mm_set_ps1((r32)(Bitmap->Width - 2));
    __m128 BitmapInternalHeight = _mm_set_ps1((r32)(Bitmap->Height - 2));
    __m128 BitmapWidth = _mm_set_ps1((r32)Bitmap->Width);
    __m128i BitmapHeight = _mm_set1_epi32(Bitmap->Height);
    __m128 OriginX = _mm_set_ps1(Origin.x);
    __m128 OriginY = _mm_set_ps1(Origin.y);
    __m128 XAxisX = _mm_set_ps1(XAxis.x);
    __m128 XAxisY = _mm_set_ps1(XAxis.y);
    __m128 YAxisX = _mm_set_ps1(YAxis.x);
    __m128 YAxisY = _mm_set_ps1(YAxis.y);
    __m128i One = _mm_set1_epi32(1);
    __m128 Four = _mm_set_ps1(4.0f);
    __m128 RealZero = _mm_set_ps1(0.0f);
    __m128 RealOne = _mm_set_ps1(1.0f);
    __m128 Half = _mm_set_ps1(0.5f);
    __m128i ColorMask = _mm_set1_epi32(0xFF);
    __m128 TintR = _mm_set_ps1(Tint.r);
    __m128 TintG = _mm_set_ps1(Tint.g);
    __m128 TintB = _mm_set_ps1(Tint.b);
    __m128 TintA = _mm_set_ps1(Tint.a);
    __m128 Mask;
    __m128i Used = _mm_set1_epi32(0);
    u32 SampleAdvance = 4*SAMPLE_COUNT;
    u8 *CoverageRow = (u8 *)CoverageBuffer->Memory + (CoverageBuffer->Pitch*YMin);
    u8 *SampleRow = (u8 *)SampleBuffer->Memory + (SampleBuffer->Pitch*YMin);
    IGNORED_TIMED_LOOP_FUNCTION(Width*Height);
#if DEBUG_BITMAPS
    __m128i Pink = _mm_set1_epi32(0xFFFF00FF);
    __m128i Blue = _mm_set1_epi32(0xFF0000FF);
#endif
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Coverage = (u32 *)CoverageRow + AdjustedXMin;
        u32 *Sample = (u32 *)SampleRow + SAMPLE_COUNT*AdjustedXMin;
        __m128 X4 = _mm_set_ps((r32)AdjustedXMin + 3, (r32)AdjustedXMin + 2,
                                   (r32)AdjustedXMin + 1, (r32)AdjustedXMin + 0);
        __m128 Y4 = _mm_set_ps1((r32)Y);
        for(s32 X = AdjustedXMin;
            X < AdjustedXMax;
            X += 4)
        {
            __m128 TestPointX = _mm_sub_ps(X4, OriginX);
            __m128 TestPointY = _mm_sub_ps(Y4, OriginY);

            __m128 U = _mm_mul_ps(_mm_add_ps((_mm_mul_ps(TestPointX, XAxisX)),
                                             _mm_mul_ps(TestPointY, XAxisY)),
                                  InvXAxisLengthSq);
            __m128 V = _mm_mul_ps(_mm_add_ps((_mm_mul_ps(TestPointX, YAxisX)),
                                             _mm_mul_ps(TestPointY, YAxisY)),
                                  InvYAxisLengthSq);

            Mask = _mm_and_ps(_mm_and_ps(_mm_cmpge_ps(U, RealZero), _mm_cmpge_ps(V, RealZero)),
                              _mm_and_ps(_mm_cmple_ps(U, RealOne), _mm_cmple_ps(V, RealOne)));

            _mm_storeu_si128((__m128i *)Coverage,
                             _mm_or_si128(_mm_loadu_si128((__m128i *)Coverage),
                                          _mm_castps_si128(Mask)));
            Used = _mm_or_si128(Used, _mm_castps_si128(Mask));

            __m128 tX = _mm_add_ps(_mm_mul_ps(U, BitmapInternalWidth), Half);
            __m128 tY = _mm_add_ps(_mm_mul_ps(V, BitmapInternalHeight), Half);

            __m128 ttX = _mm_cvtepi32_ps(_mm_cvttps_epi32(_mm_and_ps(tX, Mask)));
            __m128 ttY = _mm_cvtepi32_ps(_mm_cvttps_epi32(_mm_and_ps(tY, Mask)));

            __m128 OffsetA = _mm_add_ps(_mm_mul_ps(ttY, BitmapWidth), ttX);
            __m128 OffsetB = _mm_add_ps(OffsetA, RealOne);
            __m128 OffsetC = _mm_add_ps(OffsetA, BitmapWidth);
            __m128 OffsetD = _mm_add_ps(OffsetC, RealOne);

            __m128i TexelA = _mm_set_epi32(*((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetA).m128i_u32[3]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetA).m128i_u32[2]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetA).m128i_u32[1]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetA).m128i_u32[0]));
            __m128i TexelB = _mm_set_epi32(*((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetB).m128i_u32[3]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetB).m128i_u32[2]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetB).m128i_u32[1]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetB).m128i_u32[0]));
            __m128i TexelC = _mm_set_epi32(*((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetC).m128i_u32[3]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetC).m128i_u32[2]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetC).m128i_u32[1]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetC).m128i_u32[0]));
            __m128i TexelD = _mm_set_epi32(*((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetD).m128i_u32[3]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetD).m128i_u32[2]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetD).m128i_u32[1]),
                                           *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetD).m128i_u32[0]));


            __m128 TexelAR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelA, 16), ColorMask));
            __m128 TexelBR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelB, 16), ColorMask));
            __m128 TexelCR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelC, 16), ColorMask));
            __m128 TexelDR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelD, 16), ColorMask));

            __m128 TexelAG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelA, 8), ColorMask));
            __m128 TexelBG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelB, 8), ColorMask));
            __m128 TexelCG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelC, 8), ColorMask));
            __m128 TexelDG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelD, 8), ColorMask));

            __m128 TexelAB = _mm_cvtepi32_ps(_mm_and_si128(TexelA, ColorMask));
            __m128 TexelBB = _mm_cvtepi32_ps(_mm_and_si128(TexelB, ColorMask));
            __m128 TexelCB = _mm_cvtepi32_ps(_mm_and_si128(TexelC, ColorMask));
            __m128 TexelDB = _mm_cvtepi32_ps(_mm_and_si128(TexelD, ColorMask));

            __m128 TexelAA = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelA, 24), ColorMask));
            __m128 TexelBA = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelB, 24), ColorMask));
            __m128 TexelCA = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelC, 24), ColorMask));
            __m128 TexelDA = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelD, 24), ColorMask));

#if GAMMA_CORRECT
            // NOTE(chris): sRGB to linear
            TexelAR = _mm_mul_ps(_mm_mul_ps(TexelAR, TexelAR), Inv255);
            TexelBR = _mm_mul_ps(_mm_mul_ps(TexelBR, TexelBR), Inv255);
            TexelCR = _mm_mul_ps(_mm_mul_ps(TexelCR, TexelCR), Inv255);
            TexelDR = _mm_mul_ps(_mm_mul_ps(TexelDR, TexelDR), Inv255);

            TexelAG = _mm_mul_ps(_mm_mul_ps(TexelAG, TexelAG), Inv255);
            TexelBG = _mm_mul_ps(_mm_mul_ps(TexelBG, TexelBG), Inv255);
            TexelCG = _mm_mul_ps(_mm_mul_ps(TexelCG, TexelCG), Inv255);
            TexelDG = _mm_mul_ps(_mm_mul_ps(TexelDG, TexelDG), Inv255);

            TexelAB = _mm_mul_ps(_mm_mul_ps(TexelAB, TexelAB), Inv255);
            TexelBB = _mm_mul_ps(_mm_mul_ps(TexelBB, TexelBB), Inv255);
            TexelCB = _mm_mul_ps(_mm_mul_ps(TexelCB, TexelCB), Inv255);
            TexelDB = _mm_mul_ps(_mm_mul_ps(TexelDB, TexelDB), Inv255);
#endif

            __m128 tU = _mm_sub_ps(tX, ttX);
            __m128 tV = _mm_sub_ps(tY, ttY);
            __m128 tUInv = _mm_sub_ps(RealOne, tU);
            __m128 tVInv = _mm_sub_ps(RealOne, tV);

            __m128 LerpR = _mm_and_ps(_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(TexelAR, tUInv), _mm_mul_ps(TexelBR, tU)), tVInv),
                                                 _mm_mul_ps(_mm_add_ps(_mm_mul_ps(TexelCR, tUInv), _mm_mul_ps(TexelDR, tU)), tV)), Mask);
            __m128 LerpG = _mm_and_ps(_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(TexelAG, tUInv), _mm_mul_ps(TexelBG, tU)), tVInv),
                                                 _mm_mul_ps(_mm_add_ps(_mm_mul_ps(TexelCG, tUInv), _mm_mul_ps(TexelDG, tU)), tV)), Mask);
            __m128 LerpB = _mm_and_ps(_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(TexelAB, tUInv), _mm_mul_ps(TexelBB, tU)), tVInv),
                                                 _mm_mul_ps(_mm_add_ps(_mm_mul_ps(TexelCB, tUInv), _mm_mul_ps(TexelDB, tU)), tV)), Mask);
            __m128 LerpA = _mm_and_ps(_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(TexelAA, tUInv), _mm_mul_ps(TexelBA, tU)), tVInv),
                                                 _mm_mul_ps(_mm_add_ps(_mm_mul_ps(TexelCA, tUInv), _mm_mul_ps(TexelDA, tU)), tV)), Mask);

            __m128i Sample00 = _mm_loadu_si128((__m128i *)Sample);
            __m128i Sample10 = _mm_loadu_si128((__m128i *)(Sample+4));
            __m128i Sample01 = _mm_loadu_si128((__m128i *)(Sample+8));
            __m128i Sample11 = _mm_loadu_si128((__m128i *)(Sample+12));

            __m128 Sample00R = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample00, 16), ColorMask));
            __m128 Sample00G = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample00, 8), ColorMask));
            __m128 Sample00B = _mm_cvtepi32_ps(_mm_and_si128(Sample00, ColorMask));
            __m128 Sample10R = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample10, 16), ColorMask));
            __m128 Sample10G = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample10, 8), ColorMask));
            __m128 Sample10B = _mm_cvtepi32_ps(_mm_and_si128(Sample10, ColorMask));
            __m128 Sample01R = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample01, 16), ColorMask));
            __m128 Sample01G = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample01, 8), ColorMask));
            __m128 Sample01B = _mm_cvtepi32_ps(_mm_and_si128(Sample01, ColorMask));
            __m128 Sample11R = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample11, 16), ColorMask));
            __m128 Sample11G = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample11, 8), ColorMask));
            __m128 Sample11B = _mm_cvtepi32_ps(_mm_and_si128(Sample11, ColorMask));

#if GAMMA_CORRECT
            // NOTE(chris): sRGB to linear
            Sample00R = _mm_mul_ps(_mm_mul_ps(Sample00R, Sample00R), Inv255);
            Sample00B = _mm_mul_ps(_mm_mul_ps(Sample00B, Sample00B), Inv255);
            Sample00G = _mm_mul_ps(_mm_mul_ps(Sample00G, Sample00G), Inv255);
            Sample10R = _mm_mul_ps(_mm_mul_ps(Sample10R, Sample10R), Inv255);
            Sample10B = _mm_mul_ps(_mm_mul_ps(Sample10B, Sample10B), Inv255);
            Sample10G = _mm_mul_ps(_mm_mul_ps(Sample10G, Sample10G), Inv255);
            Sample01R = _mm_mul_ps(_mm_mul_ps(Sample01R, Sample01R), Inv255);
            Sample01B = _mm_mul_ps(_mm_mul_ps(Sample01B, Sample01B), Inv255);
            Sample01G = _mm_mul_ps(_mm_mul_ps(Sample01G, Sample01G), Inv255);
            Sample11R = _mm_mul_ps(_mm_mul_ps(Sample11R, Sample11R), Inv255);
            Sample11B = _mm_mul_ps(_mm_mul_ps(Sample11B, Sample11B), Inv255);
            Sample11G = _mm_mul_ps(_mm_mul_ps(Sample11G, Sample11G), Inv255);
#endif

            __m128 DA = _mm_sub_ps(RealOne, _mm_mul_ps(Inv255, _mm_mul_ps(LerpA, TintA)));

            __m128 Result00R = _mm_add_ps(_mm_mul_ps(TintR, LerpR), _mm_mul_ps(DA, Sample00R));
            __m128 Result00G = _mm_add_ps(_mm_mul_ps(TintG, LerpG), _mm_mul_ps(DA, Sample00G));
            __m128 Result00B = _mm_add_ps(_mm_mul_ps(TintB, LerpB), _mm_mul_ps(DA, Sample00B));
            __m128 Result10R = _mm_add_ps(_mm_mul_ps(TintR, LerpR), _mm_mul_ps(DA, Sample10R));
            __m128 Result10G = _mm_add_ps(_mm_mul_ps(TintG, LerpG), _mm_mul_ps(DA, Sample10G));
            __m128 Result10B = _mm_add_ps(_mm_mul_ps(TintB, LerpB), _mm_mul_ps(DA, Sample10B));
            __m128 Result01R = _mm_add_ps(_mm_mul_ps(TintR, LerpR), _mm_mul_ps(DA, Sample01R));
            __m128 Result01G = _mm_add_ps(_mm_mul_ps(TintG, LerpG), _mm_mul_ps(DA, Sample01G));
            __m128 Result01B = _mm_add_ps(_mm_mul_ps(TintB, LerpB), _mm_mul_ps(DA, Sample01B));
            __m128 Result11R = _mm_add_ps(_mm_mul_ps(TintR, LerpR), _mm_mul_ps(DA, Sample11R));
            __m128 Result11G = _mm_add_ps(_mm_mul_ps(TintG, LerpG), _mm_mul_ps(DA, Sample11G));
            __m128 Result11B = _mm_add_ps(_mm_mul_ps(TintB, LerpB), _mm_mul_ps(DA, Sample11B));

#if GAMMA_CORRECT
            // NOTE(chris): linear to sRGB
            __m128 Result00R255 = _mm_mul_ps(One255, Result00R);
            __m128 Result00G255 = _mm_mul_ps(One255, Result00G);
            __m128 Result00B255 = _mm_mul_ps(One255, Result00B);
            Result00R = _mm_mul_ps(Result00R255, _mm_rsqrt_ps(Result00R255));
            Result00G = _mm_mul_ps(Result00G255, _mm_rsqrt_ps(Result00G255));
            Result00B = _mm_mul_ps(Result00B255, _mm_rsqrt_ps(Result00B255));
            __m128 Result10R255 = _mm_mul_ps(One255, Result10R);
            __m128 Result10G255 = _mm_mul_ps(One255, Result10G);
            __m128 Result10B255 = _mm_mul_ps(One255, Result10B);
            Result10R = _mm_mul_ps(Result10R255, _mm_rsqrt_ps(Result10R255));
            Result10G = _mm_mul_ps(Result10G255, _mm_rsqrt_ps(Result10G255));
            Result10B = _mm_mul_ps(Result10B255, _mm_rsqrt_ps(Result10B255));
            __m128 Result01R255 = _mm_mul_ps(One255, Result01R);
            __m128 Result01G255 = _mm_mul_ps(One255, Result01G);
            __m128 Result01B255 = _mm_mul_ps(One255, Result01B);
            Result01R = _mm_mul_ps(Result01R255, _mm_rsqrt_ps(Result01R255));
            Result01G = _mm_mul_ps(Result01G255, _mm_rsqrt_ps(Result01G255));
            Result01B = _mm_mul_ps(Result01B255, _mm_rsqrt_ps(Result01B255));
            __m128 Result11R255 = _mm_mul_ps(One255, Result11R);
            __m128 Result11G255 = _mm_mul_ps(One255, Result11G);
            __m128 Result11B255 = _mm_mul_ps(One255, Result11B);
            Result11R = _mm_mul_ps(Result11R255, _mm_rsqrt_ps(Result11R255));
            Result11G = _mm_mul_ps(Result11G255, _mm_rsqrt_ps(Result11G255));
            Result11B = _mm_mul_ps(Result11B255, _mm_rsqrt_ps(Result11B255));
#endif

            __m128i Result00 = _mm_or_si128(
                _mm_or_si128(_mm_slli_epi32(_mm_cvtps_epi32(Result00R), 16),
                             _mm_slli_epi32(_mm_cvtps_epi32(Result00G), 8)),
                _mm_cvtps_epi32(Result00B));
            __m128i Result10 = _mm_or_si128(
                _mm_or_si128(_mm_slli_epi32(_mm_cvtps_epi32(Result10R), 16),
                             _mm_slli_epi32(_mm_cvtps_epi32(Result10G), 8)),
                _mm_cvtps_epi32(Result10B));
            __m128i Result01 = _mm_or_si128(
                _mm_or_si128(_mm_slli_epi32(_mm_cvtps_epi32(Result01R), 16),
                             _mm_slli_epi32(_mm_cvtps_epi32(Result01G), 8)),
                _mm_cvtps_epi32(Result01B));
            __m128i Result11 = _mm_or_si128(
                _mm_or_si128(_mm_slli_epi32(_mm_cvtps_epi32(Result11R), 16),
                             _mm_slli_epi32(_mm_cvtps_epi32(Result11G), 8)),
                _mm_cvtps_epi32(Result11B));

            _mm_storeu_si128((__m128i *)Sample, _mm_or_si128(_mm_and_si128(_mm_castps_si128(Mask), Result00),
                                                             _mm_andnot_si128(_mm_castps_si128(Mask), Sample00)));
            _mm_storeu_si128((__m128i *)(Sample+4), _mm_or_si128(_mm_and_si128(_mm_castps_si128(Mask), Result10),
                                                                 _mm_andnot_si128(_mm_castps_si128(Mask), Sample10)));
            _mm_storeu_si128((__m128i *)(Sample+8), _mm_or_si128(_mm_and_si128(_mm_castps_si128(Mask), Result01),
                                                                 _mm_andnot_si128(_mm_castps_si128(Mask), Sample01)));
            _mm_storeu_si128((__m128i *)(Sample+12), _mm_or_si128(_mm_and_si128(_mm_castps_si128(Mask), Result11),
                                                                  _mm_andnot_si128(_mm_castps_si128(Mask), Sample11)));

            Coverage += 4;
            Sample += SampleAdvance;
            X4 = _mm_add_ps(X4, Four);
        }
        CoverageRow += CoverageBuffer->Pitch;
        SampleRow += SampleBuffer->Pitch;
    }
    b32 UsedArray[4];
    _mm_storeu_si128((__m128i *)UsedArray, Used);
    RenderChunk->Used |= UsedArray[0] || UsedArray[1] || UsedArray[2] || UsedArray[3];
}

// TODO(chris): SIMD this!
internal void
RenderAlignedSolidRectangle(loaded_bitmap *BackBuffer, rectangle2 Rect, v3 Color,
                          s32 OffsetX = 0, s32 OffsetY = 0)
{
    s32 XMin = Clamp(OffsetX, RoundS32(Rect.Min.x), OffsetX + BackBuffer->Width);
    s32 YMin = Clamp(OffsetY, RoundS32(Rect.Min.y), OffsetY + BackBuffer->Height);
    s32 XMax = Clamp(OffsetX, RoundS32(Rect.Max.x), OffsetX + BackBuffer->Width);
    s32 YMax = Clamp(OffsetY, RoundS32(Rect.Max.y), OffsetY + BackBuffer->Height);

    Color *= 255.0f;
    u32 DestColor = ((RoundU32(Color.r) << 16) |
                     (RoundU32(Color.g) << 8) |
                     (RoundU32(Color.b) << 0));

    s32 Height = YMax-YMin;
    s32 Width = XMax-XMin;
    IGNORED_TIMED_LOOP_FUNCTION(Height*Width);
    u8 *PixelRow = (u8 *)BackBuffer->Memory + BackBuffer->Pitch*YMin;
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Dest = (u32 *)PixelRow + XMin;
        for(s32 X = XMin;
            X < XMax;
            ++X)
        {
            *Dest++ = DestColor;
        }
        PixelRow += BackBuffer->Pitch;
    }
}

// TODO(chris): Further optimization
internal void
RenderAlignedTranslucentRectangle(loaded_bitmap *BackBuffer, rectangle2 Rect, v4 Color,
                           s32 OffsetX = 0, s32 OffsetY = 0)
{
    s32 XMin = Clamp(OffsetX, RoundS32(Rect.Min.x), OffsetX + BackBuffer->Width);
    s32 YMin = Clamp(OffsetY, RoundS32(Rect.Min.y), OffsetY + BackBuffer->Height);
    s32 XMax = Clamp(OffsetX, RoundS32(Rect.Max.x), OffsetX + BackBuffer->Width);
    s32 YMax = Clamp(OffsetY, RoundS32(Rect.Max.y), OffsetY + BackBuffer->Height);

    Color.rgb *= 255.0f*Color.a;
    __m128 SR = _mm_set_ps1(Color.r);
    __m128 SG = _mm_set_ps1(Color.g);
    __m128 SB = _mm_set_ps1(Color.b);
    __m128 DA = _mm_set_ps1(1.0f - Color.a);
    __m128i Four = _mm_set1_epi32(4);
    __m128i XMin4 = _mm_set1_epi32(XMin);
    __m128i XMax4 = _mm_set1_epi32(XMax);
    __m128i ColorMask = _mm_set1_epi32(0x000000FF);

    s32 AdjustedXMin = XMin;
    s32 AdjustedXMax = XMax;
    s32 Width = XMax-XMin;
    s32 Height = YMax-YMin;
    s32 Adjustment = 4-Width%4;
    if(Adjustment < 4)
    {
        AdjustedXMin = Maximum(OffsetX, XMin-Adjustment);
        Adjustment -= XMin-AdjustedXMin;
        AdjustedXMax = Minimum(OffsetX + BackBuffer->Width, XMax+Adjustment);
        Width = AdjustedXMax-AdjustedXMin;
    }

    IGNORED_TIMED_LOOP_FUNCTION(Height*Width);
    u8 *PixelRow = (u8 *)BackBuffer->Memory + BackBuffer->Pitch*YMin;
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        __m128i X4 = _mm_set_epi32(AdjustedXMin+3, AdjustedXMin+2, AdjustedXMin+1, AdjustedXMin+0);
        u32 *Dest = (u32 *)PixelRow + AdjustedXMin;
        for(s32 X = AdjustedXMin;
            X < AdjustedXMax;
            X += 4)
        {
            __m128i Mask = _mm_andnot_si128(_mm_cmplt_epi32(X4, XMin4), _mm_cmplt_epi32(X4, XMax4));

            __m128i RawDest = _mm_loadu_si128((__m128i *)Dest);

            __m128 DR = _mm_add_ps(_mm_mul_ps(DA,
                                              _mm_cvtepi32_ps(
                                                  _mm_and_si128(_mm_srli_epi32(RawDest, 16),
                                                                ColorMask))),
                                   SR);
            __m128 DG = _mm_add_ps(_mm_mul_ps(DA,
                                              _mm_cvtepi32_ps(
                                                  _mm_and_si128(_mm_srli_epi32(RawDest, 8),
                                                                ColorMask))),
                                   SG);
            __m128 DB = _mm_add_ps(_mm_mul_ps(DA,
                                              _mm_cvtepi32_ps(
                                                  _mm_and_si128(RawDest,
                                                                ColorMask))),
                                   SB);

            __m128i Result = _mm_and_si128(_mm_or_si128(_mm_or_si128(_mm_slli_epi32(_mm_cvtps_epi32(DR), 16),
                                                                     _mm_slli_epi32(_mm_cvtps_epi32(DG), 8)),
                                                        _mm_cvtps_epi32(DB)), Mask);

            _mm_storeu_si128((__m128i *)Dest, Result);

            Dest += 4;
            X4 = _mm_add_epi32(X4, Four);
        }
        PixelRow += BackBuffer->Pitch;
    }
}

// TODO(chris): SIMD this!
internal void
RenderAlignedInvertedRectangle(loaded_bitmap *BackBuffer, rectangle2 Rect,
                               s32 OffsetX = 0, s32 OffsetY = 0)
{
    s32 XMin = Clamp(OffsetX, RoundS32(Rect.Min.x), OffsetX + BackBuffer->Width);
    s32 YMin = Clamp(OffsetY, RoundS32(Rect.Min.y), OffsetY + BackBuffer->Height);
    s32 XMax = Clamp(OffsetX, RoundS32(Rect.Max.x), OffsetX + BackBuffer->Width);
    s32 YMax = Clamp(OffsetY, RoundS32(Rect.Max.y), OffsetY + BackBuffer->Height);

    s32 Height = YMax-YMin;
    s32 Width = XMax-XMin;
    IGNORED_TIMED_LOOP_FUNCTION(Height*Width);
    u8 *PixelRow = (u8 *)BackBuffer->Memory + BackBuffer->Pitch*YMin;
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Dest = (u32 *)PixelRow + XMin;
        for(s32 X = XMin;
            X < XMax;
            ++X)
        {
            *Dest++ = InvertColor(*Dest);
        }
        PixelRow += BackBuffer->Pitch;
    }
}

// TODO(chris): Further optimization
internal void
RenderTriangle(render_chunk *RenderChunk, 
               v2 A, v2 B, v2 C, v4 Color, s32 OffsetX = 0, s32 OffsetY = 0)
{
    loaded_bitmap *SampleBuffer = &RenderChunk->SampleBuffer;
    loaded_bitmap *CoverageBuffer = &RenderChunk->CoverageBuffer;
    s32 XMin = Clamp(OffsetX, Floor(Minimum(Minimum(A.x, B.x), C.x)),
                     OffsetX + CoverageBuffer->Width);
    s32 YMin = Clamp(OffsetY, Floor(Minimum(Minimum(A.y, B.y), C.y)),
                     OffsetY + CoverageBuffer->Height);

    s32 XMax = Clamp(OffsetX, Ceiling(Maximum(Maximum(A.x, B.x), C.x)),
                     OffsetX + CoverageBuffer->Width);
    s32 YMax = Clamp(OffsetY, Ceiling(Maximum(Maximum(A.y, B.y), C.y)),
                     OffsetY + CoverageBuffer->Height);

    s32 AdjustedXMin = (XMin/4)*4;
    s32 AdjustedXMax = ((XMax+3)/4)*4;

#if GAMMA_CORRECT
    // NOTE(chris): sRGB to linear
    Color = sRGB1ToLinear1(Color);
#endif
    Color *= Color.a;

    v2 PerpAB = Perp(B - A);
    v2 PerpBC = Perp(C - B);
    v2 PerpCA = Perp(A - C);
    __m128 SR = _mm_set_ps1(255.0f*Color.r);
    __m128 SG = _mm_set_ps1(255.0f*Color.g);
    __m128 SB = _mm_set_ps1(255.0f*Color.b);
    __m128 DA = _mm_set_ps1(1.0f - Color.a);
    __m128 PerpABX = _mm_set_ps1(PerpAB.x);
    __m128 PerpBCX = _mm_set_ps1(PerpBC.x);
    __m128 PerpCAX = _mm_set_ps1(PerpCA.x);
    __m128 PerpABY = _mm_set_ps1(PerpAB.y);
    __m128 PerpBCY = _mm_set_ps1(PerpBC.y);
    __m128 PerpCAY = _mm_set_ps1(PerpCA.y);
    __m128 ABIsTopLeft = _mm_set_ps1(RealMask(((A.y == B.y) && (B.x < A.x)) || (B.y < A.y)));
    __m128 BCIsTopLeft = _mm_set_ps1(RealMask(((B.y == C.y) && (C.x < B.x)) || (C.y < B.y)));
    __m128 CAIsTopLeft = _mm_set_ps1(RealMask(((C.y == A.y) && (A.x < C.x)) || (A.y < C.y)));
    __m128 AX = _mm_set_ps1(A.x);
    __m128 BX = _mm_set_ps1(B.x);
    __m128 CX = _mm_set_ps1(C.x);
    __m128 AY = _mm_set_ps1(A.y);
    __m128 BY = _mm_set_ps1(B.y);
    __m128 CY = _mm_set_ps1(C.y);
    __m128 Four = _mm_set_ps1(4.0f);
    __m128 RealZero = _mm_set_ps1(0.0f);
    __m128 RealOne = _mm_set_ps1(1.0f);
    __m128 Half = _mm_set_ps1(0.5f);
    __m128 Quarter = _mm_set_ps1(0.25f);
    __m128 Sixteenth = _mm_set_ps1(0.0625f);
    __m128 Inv255 = _mm_set_ps1(1.0f/255.0f);
    __m128 One255 = _mm_set_ps1(255.0f);
    __m128i ColorMask = _mm_set1_epi32(0x000000FF);
    u32 SampleAdvance = 4*SAMPLE_COUNT;
    u8 *SampleRow = (u8 *)SampleBuffer->Memory + SampleBuffer->Pitch*YMin;
    u8 *CoverageRow = (u8 *)CoverageBuffer->Memory + CoverageBuffer->Pitch*YMin;
    __m128 SampleY0 = _mm_set_ps1((r32)YMin + 0.25f);
    __m128 SampleY1 = _mm_set_ps1((r32)YMin + 0.75f);
    __m128 SampleY0RelA = _mm_sub_ps(SampleY0, AY);
    __m128 SampleY0RelB = _mm_sub_ps(SampleY0, BY);
    __m128 SampleY0RelC = _mm_sub_ps(SampleY0, CY);
    __m128 SampleY1RelA = _mm_sub_ps(SampleY1, AY);
    __m128 SampleY1RelB = _mm_sub_ps(SampleY1, BY);
    __m128 SampleY1RelC = _mm_sub_ps(SampleY1, CY);
    __m128 SampleX0 = _mm_set_ps((r32)AdjustedXMin + 3.25f, (r32)AdjustedXMin + 2.25f,
                                 (r32)AdjustedXMin + 1.25f, (r32)AdjustedXMin + 0.25f);
    __m128 SampleX1 = _mm_set_ps((r32)AdjustedXMin + 3.75f, (r32)AdjustedXMin + 2.75f,
                                 (r32)AdjustedXMin + 1.75f, (r32)AdjustedXMin + 0.75f);
    __m128i Used = _mm_set1_epi32(0);
    IGNORED_TIMED_LOOP_FUNCTION(Width*Height);
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Sample = (u32 *)SampleRow + SAMPLE_COUNT*AdjustedXMin;
        b32 *Coverage = (b32 *)CoverageRow + AdjustedXMin;

        __m128 SampleX0RelA = _mm_sub_ps(SampleX0, AX);
        __m128 SampleX0RelB = _mm_sub_ps(SampleX0, BX);
        __m128 SampleX0RelC = _mm_sub_ps(SampleX0, CX);
        __m128 SampleX1RelA = _mm_sub_ps(SampleX1, AX);
        __m128 SampleX1RelB = _mm_sub_ps(SampleX1, BX);
        __m128 SampleX1RelC = _mm_sub_ps(SampleX1, CX);
        for(s32 X = AdjustedXMin;
            X < AdjustedXMax;
            X += 4)
        {
            // TODO(chris): IMPORTANT "top-left" rule
            __m128 Sample00InnerAB = _mm_add_ps(_mm_mul_ps(SampleX0RelA, PerpABX),
                                                   _mm_mul_ps(SampleY0RelA, PerpABY));
            __m128 Sample00InnerBC = _mm_add_ps(_mm_mul_ps(SampleX0RelB, PerpBCX),
                                                   _mm_mul_ps(SampleY0RelB, PerpBCY));
            __m128 Sample00InnerCA = _mm_add_ps(_mm_mul_ps(SampleX0RelC, PerpCAX),
                                                   _mm_mul_ps(SampleY0RelC, PerpCAY));
            __m128 Sample10InnerAB = _mm_add_ps(_mm_mul_ps(SampleX1RelA, PerpABX),
                                                   _mm_mul_ps(SampleY0RelA, PerpABY));
            __m128 Sample10InnerBC = _mm_add_ps(_mm_mul_ps(SampleX1RelB, PerpBCX),
                                                   _mm_mul_ps(SampleY0RelB, PerpBCY));
            __m128 Sample10InnerCA = _mm_add_ps(_mm_mul_ps(SampleX1RelC, PerpCAX),
                                                   _mm_mul_ps(SampleY0RelC, PerpCAY));
            __m128 Sample01InnerAB = _mm_add_ps(_mm_mul_ps(SampleX0RelA, PerpABX),
                                                   _mm_mul_ps(SampleY1RelA, PerpABY));
            __m128 Sample01InnerBC = _mm_add_ps(_mm_mul_ps(SampleX0RelB, PerpBCX),
                                                   _mm_mul_ps(SampleY1RelB, PerpBCY));
            __m128 Sample01InnerCA = _mm_add_ps(_mm_mul_ps(SampleX0RelC, PerpCAX),
                                                   _mm_mul_ps(SampleY1RelC, PerpCAY));
            __m128 Sample11InnerAB = _mm_add_ps(_mm_mul_ps(SampleX1RelA, PerpABX),
                                                   _mm_mul_ps(SampleY1RelA, PerpABY));
            __m128 Sample11InnerBC = _mm_add_ps(_mm_mul_ps(SampleX1RelB, PerpBCX),
                                                   _mm_mul_ps(SampleY1RelB, PerpBCY));
            __m128 Sample11InnerCA = _mm_add_ps(_mm_mul_ps(SampleX1RelC, PerpCAX),
                                                   _mm_mul_ps(SampleY1RelC, PerpCAY));
            __m128 Sample00Mask = _mm_and_ps(
                _mm_and_ps(_mm_or_ps(_mm_cmpgt_ps(Sample00InnerAB, RealZero),
                                     _mm_and_ps(ABIsTopLeft, _mm_cmpeq_ps(Sample00InnerAB, RealZero))),
                           _mm_or_ps(_mm_cmpgt_ps(Sample00InnerBC, RealZero),
                                     _mm_and_ps(BCIsTopLeft, _mm_cmpeq_ps(Sample00InnerBC, RealZero)))),
                _mm_or_ps(_mm_cmpgt_ps(Sample00InnerCA, RealZero),
                          _mm_and_ps(CAIsTopLeft, _mm_cmpeq_ps(Sample00InnerCA, RealZero))));
            __m128 Sample10Mask = _mm_and_ps(
                _mm_and_ps(_mm_or_ps(_mm_cmpgt_ps(Sample10InnerAB, RealZero),
                                     _mm_and_ps(ABIsTopLeft, _mm_cmpeq_ps(Sample10InnerAB, RealZero))),
                           _mm_or_ps(_mm_cmpgt_ps(Sample10InnerBC, RealZero),
                                     _mm_and_ps(BCIsTopLeft, _mm_cmpeq_ps(Sample10InnerBC, RealZero)))),
                _mm_or_ps(_mm_cmpgt_ps(Sample10InnerCA, RealZero),
                          _mm_and_ps(CAIsTopLeft, _mm_cmpeq_ps(Sample10InnerCA, RealZero))));
            __m128 Sample01Mask = _mm_and_ps(
                _mm_and_ps(_mm_or_ps(_mm_cmpgt_ps(Sample01InnerAB, RealZero),
                                     _mm_and_ps(ABIsTopLeft, _mm_cmpeq_ps(Sample01InnerAB, RealZero))),
                           _mm_or_ps(_mm_cmpgt_ps(Sample01InnerBC, RealZero),
                                     _mm_and_ps(BCIsTopLeft, _mm_cmpeq_ps(Sample01InnerBC, RealZero)))),
                _mm_or_ps(_mm_cmpgt_ps(Sample01InnerCA, RealZero),
                          _mm_and_ps(CAIsTopLeft, _mm_cmpeq_ps(Sample01InnerCA, RealZero))));
            __m128 Sample11Mask = _mm_and_ps(
                _mm_and_ps(_mm_or_ps(_mm_cmpgt_ps(Sample11InnerAB, RealZero),
                                     _mm_and_ps(ABIsTopLeft, _mm_cmpeq_ps(Sample11InnerAB, RealZero))),
                           _mm_or_ps(_mm_cmpgt_ps(Sample11InnerBC, RealZero),
                                     _mm_and_ps(BCIsTopLeft, _mm_cmpeq_ps(Sample11InnerBC, RealZero)))),
                _mm_or_ps(_mm_cmpgt_ps(Sample11InnerCA, RealZero),
                          _mm_and_ps(CAIsTopLeft, _mm_cmpeq_ps(Sample11InnerCA, RealZero))));

            __m128i Cover = _mm_castps_si128(_mm_or_ps(_mm_or_ps(Sample00Mask, Sample10Mask),
                                                       _mm_or_ps(Sample01Mask, Sample11Mask)));
            _mm_storeu_si128((__m128i *)Coverage,
                             _mm_or_si128(_mm_loadu_si128((__m128i *)Coverage), Cover));

            Used = _mm_or_si128(Used, Cover);

            __m128i Sample00 = _mm_loadu_si128((__m128i *)Sample);
            __m128i Sample10 = _mm_loadu_si128((__m128i *)(Sample + 4));
            __m128i Sample01 = _mm_loadu_si128((__m128i *)(Sample + 8));
            __m128i Sample11 = _mm_loadu_si128((__m128i *)(Sample + 12));

            __m128 Sample00R = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample00, 16), ColorMask));
            __m128 Sample00G = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample00, 8), ColorMask));
            __m128 Sample00B = _mm_cvtepi32_ps(_mm_and_si128(Sample00, ColorMask));

            __m128 Sample10R = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample10, 16), ColorMask));
            __m128 Sample10G = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample10, 8), ColorMask));
            __m128 Sample10B = _mm_cvtepi32_ps(_mm_and_si128(Sample10, ColorMask));

            __m128 Sample01R = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample01, 16), ColorMask));
            __m128 Sample01G = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample01, 8), ColorMask));
            __m128 Sample01B = _mm_cvtepi32_ps(_mm_and_si128(Sample01, ColorMask));

            __m128 Sample11R = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample11, 16), ColorMask));
            __m128 Sample11G = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample11, 8), ColorMask));
            __m128 Sample11B = _mm_cvtepi32_ps(_mm_and_si128(Sample11, ColorMask));

#if GAMMA_CORRECT
            // NOTE(chris): sRGB to linear
            Sample00R = _mm_mul_ps(_mm_mul_ps(Sample00R, Sample00R), Inv255);
            Sample10R = _mm_mul_ps(_mm_mul_ps(Sample10R, Sample10R), Inv255);
            Sample01R = _mm_mul_ps(_mm_mul_ps(Sample01R, Sample01R), Inv255);
            Sample11R = _mm_mul_ps(_mm_mul_ps(Sample11R, Sample11R), Inv255);

            Sample00G = _mm_mul_ps(_mm_mul_ps(Sample00G, Sample00G), Inv255);
            Sample10G = _mm_mul_ps(_mm_mul_ps(Sample10G, Sample10G), Inv255);
            Sample01G = _mm_mul_ps(_mm_mul_ps(Sample01G, Sample01G), Inv255);
            Sample11G = _mm_mul_ps(_mm_mul_ps(Sample11G, Sample11G), Inv255);

            Sample00B = _mm_mul_ps(_mm_mul_ps(Sample00B, Sample00B), Inv255);
            Sample10B = _mm_mul_ps(_mm_mul_ps(Sample10B, Sample10B), Inv255);
            Sample01B = _mm_mul_ps(_mm_mul_ps(Sample01B, Sample01B), Inv255);
            Sample11B = _mm_mul_ps(_mm_mul_ps(Sample11B, Sample11B), Inv255);
#endif            
            __m128 Result00R = _mm_add_ps(SR, _mm_mul_ps(Sample00R, DA));
            __m128 Result00G = _mm_add_ps(SG, _mm_mul_ps(Sample00G, DA));
            __m128 Result00B = _mm_add_ps(SB, _mm_mul_ps(Sample00B, DA));
            __m128 Result10R = _mm_add_ps(SR, _mm_mul_ps(Sample10R, DA));
            __m128 Result10G = _mm_add_ps(SG, _mm_mul_ps(Sample10G, DA));
            __m128 Result10B = _mm_add_ps(SB, _mm_mul_ps(Sample10B, DA));
            __m128 Result01R = _mm_add_ps(SR, _mm_mul_ps(Sample01R, DA));
            __m128 Result01G = _mm_add_ps(SG, _mm_mul_ps(Sample01G, DA));
            __m128 Result01B = _mm_add_ps(SB, _mm_mul_ps(Sample01B, DA));
            __m128 Result11R = _mm_add_ps(SR, _mm_mul_ps(Sample11R, DA));
            __m128 Result11G = _mm_add_ps(SG, _mm_mul_ps(Sample11G, DA));
            __m128 Result11B = _mm_add_ps(SB, _mm_mul_ps(Sample11B, DA));

#if GAMMA_CORRECT
            // NOTE(chris): linear to sRGB
            __m128 Result00R255 = _mm_mul_ps(One255, Result00R);
            __m128 Result00G255 = _mm_mul_ps(One255, Result00G);
            __m128 Result00B255 = _mm_mul_ps(One255, Result00B);
            Result00R = _mm_mul_ps(Result00R255, _mm_rsqrt_ps(Result00R255));
            Result00G = _mm_mul_ps(Result00G255, _mm_rsqrt_ps(Result00G255));
            Result00B = _mm_mul_ps(Result00B255, _mm_rsqrt_ps(Result00B255));
            __m128 Result10R255 = _mm_mul_ps(One255, Result10R);
            __m128 Result10G255 = _mm_mul_ps(One255, Result10G);
            __m128 Result10B255 = _mm_mul_ps(One255, Result10B);
            Result10R = _mm_mul_ps(Result10R255, _mm_rsqrt_ps(Result10R255));
            Result10G = _mm_mul_ps(Result10G255, _mm_rsqrt_ps(Result10G255));
            Result10B = _mm_mul_ps(Result10B255, _mm_rsqrt_ps(Result10B255));
            __m128 Result01R255 = _mm_mul_ps(One255, Result01R);
            __m128 Result01G255 = _mm_mul_ps(One255, Result01G);
            __m128 Result01B255 = _mm_mul_ps(One255, Result01B);
            Result01R = _mm_mul_ps(Result01R255, _mm_rsqrt_ps(Result01R255));
            Result01G = _mm_mul_ps(Result01G255, _mm_rsqrt_ps(Result01G255));
            Result01B = _mm_mul_ps(Result01B255, _mm_rsqrt_ps(Result01B255));
            __m128 Result11R255 = _mm_mul_ps(One255, Result11R);
            __m128 Result11G255 = _mm_mul_ps(One255, Result11G);
            __m128 Result11B255 = _mm_mul_ps(One255, Result11B);
            Result11R = _mm_mul_ps(Result11R255, _mm_rsqrt_ps(Result11R255));
            Result11G = _mm_mul_ps(Result11G255, _mm_rsqrt_ps(Result11G255));
            Result11B = _mm_mul_ps(Result11B255, _mm_rsqrt_ps(Result11B255));
#endif

            __m128i Result00 = _mm_or_si128(
                _mm_or_si128(_mm_slli_epi32(_mm_cvtps_epi32(Result00R), 16),
                             _mm_slli_epi32(_mm_cvtps_epi32(Result00G), 8)),
                _mm_cvtps_epi32(Result00B));
            __m128i Result10 = _mm_or_si128(
                _mm_or_si128(_mm_slli_epi32(_mm_cvtps_epi32(Result10R), 16),
                             _mm_slli_epi32(_mm_cvtps_epi32(Result10G), 8)),
                _mm_cvtps_epi32(Result10B));
            __m128i Result01 = _mm_or_si128(
                _mm_or_si128(_mm_slli_epi32(_mm_cvtps_epi32(Result01R), 16),
                             _mm_slli_epi32(_mm_cvtps_epi32(Result01G), 8)),
                _mm_cvtps_epi32(Result01B));
            __m128i Result11 = _mm_or_si128(
                _mm_or_si128(_mm_slli_epi32(_mm_cvtps_epi32(Result11R), 16),
                             _mm_slli_epi32(_mm_cvtps_epi32(Result11G), 8)),
                _mm_cvtps_epi32(Result11B));

            _mm_storeu_si128((__m128i *)Sample, _mm_or_si128(_mm_and_si128(_mm_castps_si128(Sample00Mask), Result00),
                                                             _mm_andnot_si128(_mm_castps_si128(Sample00Mask), Sample00)));
            _mm_storeu_si128((__m128i *)(Sample+4), _mm_or_si128(_mm_and_si128(_mm_castps_si128(Sample10Mask), Result10),
                                                               _mm_andnot_si128(_mm_castps_si128(Sample10Mask), Sample10)));
            _mm_storeu_si128((__m128i *)(Sample+8), _mm_or_si128(_mm_and_si128(_mm_castps_si128(Sample01Mask), Result01),
                                                               _mm_andnot_si128(_mm_castps_si128(Sample01Mask), Sample01)));
            _mm_storeu_si128((__m128i *)(Sample+12), _mm_or_si128(_mm_and_si128(_mm_castps_si128(Sample11Mask), Result11),
                                                                _mm_andnot_si128(_mm_castps_si128(Sample11Mask), Sample11)));

            Coverage += 4;
            Sample += SampleAdvance;
            SampleX0RelA = _mm_add_ps(SampleX0RelA, Four);
            SampleX0RelB = _mm_add_ps(SampleX0RelB, Four);
            SampleX0RelC = _mm_add_ps(SampleX0RelC, Four);
            SampleX1RelA = _mm_add_ps(SampleX1RelA, Four);
            SampleX1RelB = _mm_add_ps(SampleX1RelB, Four);
            SampleX1RelC = _mm_add_ps(SampleX1RelC, Four);
        }
        SampleY0RelA = _mm_add_ps(SampleY0RelA, RealOne);
        SampleY0RelB = _mm_add_ps(SampleY0RelB, RealOne);
        SampleY0RelC = _mm_add_ps(SampleY0RelC, RealOne);
        SampleY1RelA = _mm_add_ps(SampleY1RelA, RealOne);
        SampleY1RelB = _mm_add_ps(SampleY1RelB, RealOne);
        SampleY1RelC = _mm_add_ps(SampleY1RelC, RealOne);
        SampleRow += SampleBuffer->Pitch;
        CoverageRow += CoverageBuffer->Pitch;
    }
    b32 UsedArray[4];
    _mm_storeu_si128((__m128i *)UsedArray, Used);
    RenderChunk->Used |= UsedArray[0] || UsedArray[1] || UsedArray[2] || UsedArray[3];
}

internal void
ClearBuffers(render_chunk *RenderChunk)
{
    if(RenderChunk->Used)
    {
        TIMED_FUNCTION();
        loaded_bitmap *CoverageBuffer = &RenderChunk->CoverageBuffer;
        loaded_bitmap *SampleBuffer = &RenderChunk->SampleBuffer;
        Assert((CoverageBuffer->Width&3) == 0);
        s32 XMin = RenderChunk->OffsetX;
        s32 YMin = RenderChunk->OffsetY;
        s32 XMax = RenderChunk->OffsetX + CoverageBuffer->Width;
        s32 YMax = RenderChunk->OffsetY + CoverageBuffer->Height;
        u32 SampleAdvance = 4*SAMPLE_COUNT;
        u8 *SampleRow = (u8 *)SampleBuffer->Memory + SampleBuffer->Pitch*YMin;
        u8 *CoverageRow = (u8 *)CoverageBuffer->Memory + CoverageBuffer->Pitch*YMin;
#if GAMMA_CORRECT
        v3 ClearColor = sRGB1ToLinear255(RenderChunk->ClearColor);
#else
        v3 ClearColor = 255.0f*RenderChunk->ClearColor;
#endif
        __m128i Color = _mm_set1_epi32((RoundU32(ClearColor.r) << 16) |
                                            (RoundU32(ClearColor.g) << 8) |
                                            (RoundU32(ClearColor.b)));
        __m128i Zero = _mm_set1_epi32(0);
        for(s32 Y = YMin;
            Y < YMax;
            ++Y)
        {
            u32 *Sample = (u32 *)SampleRow + SAMPLE_COUNT*XMin;
            b32 *Coverage = (b32 *)CoverageRow + XMin;
            for(s32 X = XMin;
                X < XMax;
                X += 4)
            {
                _mm_storeu_si128((__m128i *)Coverage, Zero);
                _mm_storeu_si128((__m128i *)(Sample), Color);
                _mm_storeu_si128((__m128i *)(Sample + 4), Color);
                _mm_storeu_si128((__m128i *)(Sample + 8), Color);
                _mm_storeu_si128((__m128i *)(Sample + 12), Color);
            
                Sample += SampleAdvance;
                Coverage += 4;
            }
            SampleRow += SampleBuffer->Pitch;
            CoverageRow += CoverageBuffer->Pitch;
        }
    }
    RenderChunk->Used = false;
}

THREAD_CALLBACK(ClearRenderBuffersCallback)
{
    ClearBuffers((render_chunk *)Params);
}

inline void
ClearAllBuffers(renderer_state *RendererState, b32 Force = false)
{
    TIMED_FUNCTION();
    thread_progress ThreadProgress[RENDER_CHUNK_COUNT];
    for(s32 RenderChunkIndex = RENDER_CHUNK_COUNT-1;
        RenderChunkIndex >= 0;
        --RenderChunkIndex)
    {
        thread_progress *Progress = ThreadProgress + RenderChunkIndex;
        render_chunk *RenderChunk = RendererState->RenderChunks + RenderChunkIndex;
        RenderChunk->Used |= Force;
        RenderChunk->ClearColor = RendererState->ClearColor;
        if(RenderChunkIndex == 0)
        {
            ClearRenderBuffersCallback(RenderChunk);
            Progress->Finished = true;
        }
        else
        {
            PlatformPushThreadWork(ClearRenderBuffersCallback, RenderChunk, Progress);
        }
    }

    b32 Finished;
    do
    {
        Finished = true;
        for(s32 ProgressIndex = 0;
            ProgressIndex < RENDER_CHUNK_COUNT;
            ++ProgressIndex)
        {
            thread_progress *Progress = ThreadProgress + ProgressIndex;
            Finished &= Progress->Finished;
        }
    } while(!Finished);
}

inline void
PushClear(renderer_state *RendererState, render_buffer *RenderBuffer, v3 Color)
{
    if(Color != RendererState->ClearColor)
    {
        RendererState->ClearColor = Color;
        ClearAllBuffers(RendererState, true);
    }
    PushRenderHeader(Data, &RenderBuffer->Arena, clear);
    Data->Color = Color;
}

internal void
RenderSamples(render_chunk *RenderChunk)
{
    TIMED_FUNCTION();
    loaded_bitmap *BackBuffer = &RenderChunk->BackBuffer;
    loaded_bitmap *SampleBuffer = &RenderChunk->SampleBuffer;
    loaded_bitmap *CoverageBuffer = &RenderChunk->CoverageBuffer;
    s32 OffsetX = RenderChunk->OffsetX;
    s32 OffsetY = RenderChunk->OffsetY;

    s32 XMin = OffsetX;
    s32 YMin = OffsetY;
    s32 XMax = OffsetX + BackBuffer->Width;
    s32 YMax = OffsetY + BackBuffer->Height;
    
    u8 *PixelRow = (u8 *)BackBuffer->Memory + BackBuffer->Pitch*OffsetY;
    u8 *SampleRow = (u8 *)SampleBuffer->Memory + YMin*SampleBuffer->Pitch;
    u8 *CoverageRow = (u8 *)CoverageBuffer->Memory + YMin*CoverageBuffer->Pitch;
    __m128 Four = _mm_set_ps1(4.0f);
    __m128 SampleFraction = _mm_set_ps1(1.0f / SAMPLE_COUNT);
    __m128 One255 = _mm_set_ps1(255.0f);
    __m128i Zero = _mm_set1_epi32(0);
    __m128i ColorMask = _mm_set1_epi32(0x000000FF);
    __m128i ClearColor = _mm_set1_epi32((RoundU32(255.0f*RenderChunk->ClearColor.r) << 16) |
                                        (RoundU32(255.0f*RenderChunk->ClearColor.g) << 8) |
                                        (RoundU32(255.0f*RenderChunk->ClearColor.b)));
    u32 SampleAdvance = 4*SAMPLE_COUNT;

    s32 AdjustedXMin = (XMin/4)*4;
    s32 AdjustedXMax = ((XMax+3)/4)*4;

    __m128 XMin4 = _mm_set_ps1((r32)XMin);
    __m128 XMax4 = _mm_set_ps1((r32)XMax);
    
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Pixel = (u32 *)PixelRow + AdjustedXMin;
        u32 *Sample = (u32 *)SampleRow + SAMPLE_COUNT*AdjustedXMin;
        b32 *Coverage = (b32 *)CoverageRow + AdjustedXMin;
        __m128 X4 = _mm_set_ps((r32)AdjustedXMin + 3, (r32)AdjustedXMin + 2,
                               (r32)AdjustedXMin + 1, (r32)AdjustedXMin);
        for(s32 X = AdjustedXMin;
            X < AdjustedXMax;
            X += 4)
        {
            __m128i XMask = _mm_castps_si128(_mm_and_ps(_mm_cmple_ps(XMin4, X4),
                                                        _mm_cmplt_ps(X4, XMax4)));
            __m128i Sample00 = _mm_loadu_si128((__m128i *)Sample);
            __m128i Sample10 = _mm_loadu_si128((__m128i *)(Sample + 4));
            __m128i Sample01 = _mm_loadu_si128((__m128i *)(Sample + 8));
            __m128i Sample11 = _mm_loadu_si128((__m128i *)(Sample + 12));

            __m128 Sample00R = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample00, 16), ColorMask));
            __m128 Sample00G = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample00, 8), ColorMask));
            __m128 Sample00B = _mm_cvtepi32_ps(_mm_and_si128(Sample00, ColorMask));

            __m128 Sample10R = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample10, 16), ColorMask));
            __m128 Sample10G = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample10, 8), ColorMask));
            __m128 Sample10B = _mm_cvtepi32_ps(_mm_and_si128(Sample10, ColorMask));

            __m128 Sample01R = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample01, 16), ColorMask));
            __m128 Sample01G = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample01, 8), ColorMask));
            __m128 Sample01B = _mm_cvtepi32_ps(_mm_and_si128(Sample01, ColorMask));

            __m128 Sample11R = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample11, 16), ColorMask));
            __m128 Sample11G = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Sample11, 8), ColorMask));
            __m128 Sample11B = _mm_cvtepi32_ps(_mm_and_si128(Sample11, ColorMask));

            __m128 R = _mm_mul_ps(SampleFraction,
                                  _mm_add_ps(_mm_add_ps(Sample00R, Sample10R),
                                             _mm_add_ps(Sample01R, Sample11R)));
            __m128 G = _mm_mul_ps(SampleFraction,
                                  _mm_add_ps(_mm_add_ps(Sample00G, Sample10G),
                                             _mm_add_ps(Sample01G, Sample11G)));
            __m128 B = _mm_mul_ps(SampleFraction,
                                  _mm_add_ps(_mm_add_ps(Sample00B, Sample10B),
                                             _mm_add_ps(Sample01B, Sample11B)));

            __m128i Result = _mm_or_si128(
                _mm_or_si128(
                    _mm_slli_epi32(_mm_cvtps_epi32(R), 16),
                    _mm_slli_epi32(_mm_cvtps_epi32(G), 8)),
                _mm_cvtps_epi32(B));

            __m128i Mask = _mm_and_si128(_mm_loadu_si128((__m128i *)Coverage), XMask);

            _mm_storeu_si128((__m128i *)Pixel, _mm_or_si128(_mm_and_si128(Mask, Result),
                                                            _mm_andnot_si128(Mask, ClearColor)));

#if 1
            _mm_storeu_si128((__m128i *)Coverage, Zero);
            _mm_storeu_si128((__m128i *)(Sample), ClearColor);
            _mm_storeu_si128((__m128i *)(Sample + 4), ClearColor);
            _mm_storeu_si128((__m128i *)(Sample + 8), ClearColor);
            _mm_storeu_si128((__m128i *)(Sample + 12), ClearColor);
#endif

            Pixel += 4;
            Sample += SampleAdvance;
            Coverage += 4;
            X4 = _mm_add_ps(X4, Four);
        }
        PixelRow += BackBuffer->Pitch;
        SampleRow += SampleBuffer->Pitch;
        CoverageRow += CoverageBuffer->Pitch;
    }
    RenderChunk->Used = false;
}

#if TROIDS_INTERNAL
// TODO(chris): Further optimization
internal void
DEBUGRenderSolidRectangle(loaded_bitmap *BackBuffer, v2 Origin, v2 XAxis, v2 YAxis,
                     v3 Color, s32 OffsetX = 0, s32 OffsetY = 0)
{
    s32 XMin = Clamp(OffsetX, Floor(Minimum(Minimum(Origin.x, Origin.x + XAxis.x),
                                      Minimum(Origin.x + YAxis.x, Origin.x + XAxis.x + YAxis.x))),
                     OffsetX + BackBuffer->Width);
    s32 YMin = Clamp(OffsetY, Floor(Minimum(Minimum(Origin.y, Origin.y + XAxis.y),
                                      Minimum(Origin.y + YAxis.y, Origin.y + XAxis.y + YAxis.y))),
                     OffsetY + BackBuffer->Height);

    s32 XMax = Clamp(OffsetX, Ceiling(Maximum(Maximum(Origin.x, Origin.x + XAxis.x),
                                        Maximum(Origin.x + YAxis.x, Origin.x + XAxis.x + YAxis.x))),
                     OffsetX + BackBuffer->Width);
    s32 YMax = Clamp(OffsetY, Ceiling(Maximum(Maximum(Origin.y, Origin.y + XAxis.y),
                                        Maximum(Origin.y + YAxis.y, Origin.y + XAxis.y + YAxis.y))),
                     OffsetY + BackBuffer->Height);

    s32 AdjustedXMin = XMin;
    s32 AdjustedXMax = XMax;
    s32 Width = XMax-XMin;
    s32 Height = YMax-YMin;
    s32 Adjustment = 4-Width%4;
    if(Adjustment < 4)
    {
        AdjustedXMin = Maximum(OffsetX, XMin-Adjustment);
        Adjustment -= XMin-AdjustedXMin;
        AdjustedXMax = Minimum(OffsetX + BackBuffer->Width, XMax+Adjustment);
        Width = AdjustedXMax-AdjustedXMin;
    }

    Color *= 255.0f;
    __m128i DestColor = _mm_set1_epi32(((RoundU32(Color.r) << 16) |
                                        (RoundU32(Color.g) << 8) |
                                        (RoundU32(Color.b) << 0)));

    __m128 InvXAxisLengthSq = _mm_set_ps1(1.0f / LengthSq(XAxis));
    __m128 InvYAxisLengthSq = _mm_set_ps1(1.0f / LengthSq(YAxis));
    __m128 OriginX = _mm_set_ps1(Origin.x);
    __m128 OriginY = _mm_set_ps1(Origin.y);
    __m128 XAxisX = _mm_set_ps1(XAxis.x);
    __m128 XAxisY = _mm_set_ps1(XAxis.y);
    __m128 YAxisX = _mm_set_ps1(YAxis.x);
    __m128 YAxisY = _mm_set_ps1(YAxis.y);
    __m128 Four = _mm_set_ps1(4.0f);
    __m128 RealZero = _mm_set_ps1(0.0f);
    __m128 RealOne = _mm_set_ps1(1.0f);
    __m128i Mask;
    u8 *PixelRow = (u8 *)BackBuffer->Memory + (BackBuffer->Pitch*YMin);
    IGNORED_TIMED_LOOP_FUNCTION(Width*Height);
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Pixel = (u32 *)PixelRow + AdjustedXMin;
        __m128 X4 = _mm_set_ps((r32)AdjustedXMin + 3, (r32)AdjustedXMin + 2,
                                   (r32)AdjustedXMin + 1, (r32)AdjustedXMin + 0);
        __m128 Y4 = _mm_set_ps1((r32)Y);
        for(s32 X = AdjustedXMin;
            X < AdjustedXMax;
            X += 4)
        {
            __m128i RawDest = _mm_loadu_si128((__m128i *)Pixel);
            __m128 TestPointX = _mm_sub_ps(X4, OriginX);
            __m128 TestPointY = _mm_sub_ps(Y4, OriginY);

            __m128 U = _mm_mul_ps(_mm_add_ps((_mm_mul_ps(TestPointX, XAxisX)),
                                             _mm_mul_ps(TestPointY, XAxisY)),
                                  InvXAxisLengthSq);
            __m128 V = _mm_mul_ps(_mm_add_ps((_mm_mul_ps(TestPointX, YAxisX)),
                                             _mm_mul_ps(TestPointY, YAxisY)),
                                  InvYAxisLengthSq);

            Mask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(U, RealZero), _mm_cmpge_ps(V, RealZero)),
                                               _mm_and_ps(_mm_cmple_ps(U, RealOne), _mm_cmple_ps(V, RealOne))));

            _mm_storeu_si128((__m128i *)Pixel, _mm_or_si128(_mm_andnot_si128(Mask, RawDest),
                                                            _mm_and_si128(DestColor, Mask)));

            Pixel += 4;
            X4 = _mm_add_ps(X4, Four);
        }
        PixelRow += BackBuffer->Pitch;
    }
}

// TODO(chris): Further optimization
internal void
DEBUGRenderSolidTriangle(loaded_bitmap *BackBuffer, v2 A, v2 B, v2 C, v3 Color,
                    s32 OffsetX = 0, s32 OffsetY = 0)
{
    s32 XMin = Clamp(OffsetX, RoundS32(Minimum(Minimum(A.x, B.x), C.x)),
                     OffsetX + BackBuffer->Width);
    s32 YMin = Clamp(OffsetY, RoundS32(Minimum(Minimum(A.y, B.y), C.y)),
                     OffsetY + BackBuffer->Height);

    s32 XMax = Clamp(OffsetX, RoundS32(Maximum(Maximum(A.x, B.x), C.x)),
                     OffsetX + BackBuffer->Width);
    s32 YMax = Clamp(OffsetY, RoundS32(Maximum(Maximum(A.y, B.y), C.y)),
                     OffsetY + BackBuffer->Height);

    s32 AdjustedXMin = XMin;
    s32 AdjustedXMax = XMax;
    s32 Width = XMax-XMin;
    s32 Height = YMax-YMin;
    s32 Adjustment = 4-Width%4;
    if(Adjustment < 4)
    {
        AdjustedXMin = Maximum(OffsetX, XMin-Adjustment);
        Adjustment -= XMin-AdjustedXMin;
        AdjustedXMax = Minimum(OffsetX + BackBuffer->Width, XMax+Adjustment);
        Width = AdjustedXMax-AdjustedXMin;
    }

    Color *= 255.0f;
    __m128i DestColor = _mm_set1_epi32(((RoundU32(Color.r) << 16) |
                                        (RoundU32(Color.g) << 8) |
                                        (RoundU32(Color.b) << 0)));

    v2 PerpAB = Perp(B - A);
    v2 PerpBC = Perp(C - B);
    v2 PerpCA = Perp(A - C);
    __m128 PerpABX = _mm_set_ps1(PerpAB.x);
    __m128 PerpBCX = _mm_set_ps1(PerpBC.x);
    __m128 PerpCAX = _mm_set_ps1(PerpCA.x);
    __m128 PerpABY = _mm_set_ps1(PerpAB.y);
    __m128 PerpBCY = _mm_set_ps1(PerpBC.y);
    __m128 PerpCAY = _mm_set_ps1(PerpCA.y);
    __m128 AX = _mm_set_ps1(A.x);
    __m128 BX = _mm_set_ps1(B.x);
    __m128 CX = _mm_set_ps1(C.x);
    __m128 AY = _mm_set_ps1(A.y);
    __m128 BY = _mm_set_ps1(B.y);
    __m128 CY = _mm_set_ps1(C.y);
    __m128 Four = _mm_set_ps1(4.0f);
    __m128 RealZero = _mm_set_ps1(0.0f);
    __m128 RealOne = _mm_set_ps1(1.0f);
    __m128i Mask;
    u8 *PixelRow = (u8 *)BackBuffer->Memory + (BackBuffer->Pitch*YMin);
    IGNORED_TIMED_LOOP_FUNCTION(Width*Height);
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Pixel = (u32 *)PixelRow + AdjustedXMin;
        __m128 X4 = _mm_set_ps((r32)AdjustedXMin + 3, (r32)AdjustedXMin + 2,
                                   (r32)AdjustedXMin + 1, (r32)AdjustedXMin + 0);
        __m128 Y4 = _mm_set_ps1((r32)Y);
        __m128 YRelA = _mm_sub_ps(Y4, AY);
        __m128 YRelB = _mm_sub_ps(Y4, BY);
        __m128 YRelC = _mm_sub_ps(Y4, CY);
        for(s32 X = AdjustedXMin;
            X < AdjustedXMax;
            X += 4)
        {
            __m128i RawDest = _mm_loadu_si128((__m128i *)Pixel);

            __m128 XRelA = _mm_sub_ps(X4, AX);
            __m128 XRelB = _mm_sub_ps(X4, BX);
            __m128 XRelC = _mm_sub_ps(X4, CX);

            __m128 ABInner = _mm_add_ps(_mm_mul_ps(XRelA, PerpABX), _mm_mul_ps(YRelA, PerpABY));
            __m128 BCInner = _mm_add_ps(_mm_mul_ps(XRelB, PerpBCX), _mm_mul_ps(YRelB, PerpBCY));
            __m128 CAInner = _mm_add_ps(_mm_mul_ps(XRelC, PerpCAX), _mm_mul_ps(YRelC, PerpCAY));
            
            Mask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(ABInner, RealZero),
                                                          _mm_cmpge_ps(BCInner, RealZero)),
                                               _mm_cmpge_ps(CAInner, RealZero)));

            _mm_storeu_si128((__m128i *)Pixel, _mm_or_si128(_mm_andnot_si128(Mask, RawDest),
                                                            _mm_and_si128(DestColor, Mask)));

            Pixel += 4;
            X4 = _mm_add_ps(X4, Four);
        }
        PixelRow += BackBuffer->Pitch;
    }
}

// TODO(chris): Further optimization
internal void
DEBUGRenderSolidCircle(loaded_bitmap *BackBuffer, v2 P, r32 Radius,
                  v3 Color, s32 OffsetX = 0, s32 OffsetY = 0)
{
    s32 XMin = Clamp(OffsetX, RoundS32(P.x - Radius), OffsetX + BackBuffer->Width);
    s32 YMin = Clamp(OffsetY, RoundS32(P.y - Radius), OffsetY + BackBuffer->Height);

    s32 XMax = Clamp(OffsetX, RoundS32(P.x + Radius), OffsetX + BackBuffer->Width);
    s32 YMax = Clamp(OffsetY, RoundS32(P.y  + Radius), OffsetY + BackBuffer->Height);

    s32 AdjustedXMin = XMin;
    s32 AdjustedXMax = XMax;
    s32 Width = XMax-XMin;
    s32 Height = YMax-YMin;
    s32 Adjustment = 4-Width%4;
    if(Adjustment < 4)
    {
        AdjustedXMin = Maximum(OffsetX, XMin-Adjustment);
        Adjustment -= XMin-AdjustedXMin;
        AdjustedXMax = Minimum(OffsetX + BackBuffer->Width, XMax+Adjustment);
        Width = AdjustedXMax-AdjustedXMin;
    }

    Color *= 255.0f;
    __m128i DestColor = _mm_set1_epi32(((RoundU32(Color.r) << 16) |
                                        (RoundU32(Color.g) << 8) |
                                        (RoundU32(Color.b) << 0)));

    __m128 RadiusSq = _mm_set_ps1(Radius*Radius);
    __m128 PX = _mm_set_ps1(P.x);
    __m128 PY = _mm_set_ps1(P.y);
    __m128 Four = _mm_set_ps1(4.0f);
    __m128 RealZero = _mm_set_ps1(0.0f);
    __m128 RealOne = _mm_set_ps1(1.0f);
    __m128i Mask;
    u8 *PixelRow = (u8 *)BackBuffer->Memory + (BackBuffer->Pitch*YMin);
    IGNORED_TIMED_LOOP_FUNCTION(Width*Height);
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Pixel = (u32 *)PixelRow + AdjustedXMin;
        __m128 X4 = _mm_set_ps((r32)AdjustedXMin + 3, (r32)AdjustedXMin + 2,
                                   (r32)AdjustedXMin + 1, (r32)AdjustedXMin + 0);
        __m128 Y4 = _mm_set_ps1((r32)Y);
        for(s32 X = AdjustedXMin;
            X < AdjustedXMax;
            X += 4)
        {
            __m128i RawDest = _mm_loadu_si128((__m128i *)Pixel);

            __m128 dX = _mm_sub_ps(X4, PX);
            __m128 dY = _mm_sub_ps(Y4, PY);
            __m128 DistanceSq = _mm_add_ps(_mm_mul_ps(dX, dX), _mm_mul_ps(dY, dY));
            Mask = _mm_castps_si128(_mm_cmple_ps(DistanceSq, RadiusSq));

            _mm_storeu_si128((__m128i *)Pixel, _mm_or_si128(_mm_andnot_si128(Mask, RawDest),
                                                            _mm_and_si128(DestColor, Mask)));

            Pixel += 4;
            X4 = _mm_add_ps(X4, Four);
        }
        PixelRow += BackBuffer->Pitch;
    }
}

internal void
DEBUGRenderLine(loaded_bitmap *BackBuffer, v2 PointA, v2 PointB, v4 Color, s32 OffsetX = 0, s32 OffsetY = 0)
{
    u32 A = (u32)(Color.a*255.0f + 0.5f);
    u32 R = (u32)(Color.r*255.0f + 0.5f);
    u32 G = (u32)(Color.g*255.0f + 0.5f);
    u32 B = (u32)(Color.b*255.0f + 0.5f);

    u32 DestColor = ((A << 24) | // A
                     (R << 16) | // R
                     (G << 8)  | // G
                     (B << 0));  // B

    r32 XDistance = AbsoluteValue(PointB.x - PointA.x);
    r32 YDistance = AbsoluteValue(PointB.y - PointA.y);

    b32 MoreHorizontal = XDistance > YDistance;

    r32 OneOverDistance;
    v2 Start, End;
    s32 Min, Max;
    if(MoreHorizontal)
    {
        OneOverDistance = 1.0f / XDistance;
        if(PointA.x < PointB.x)
        {
            Start = PointA;
            End = PointB;
        }
        else
        {
            Start = PointB;
            End = PointA;
        }
        Min = Clamp(OffsetX, RoundS32(Start.x), OffsetX + BackBuffer->Width);
        Max = Clamp(OffsetX, RoundS32(End.x), OffsetX + BackBuffer->Width);
        for(s32 X = Min;
            X < Max;
            ++X)
        {
            s32 Y = RoundS32(Lerp(Start.y, OneOverDistance*(X - Start.x), End.y));
            if(Y >= OffsetY && Y < OffsetY + BackBuffer->Height)
            {
                u32 *Row = (u32 *)((u8 *)BackBuffer->Memory + Y*BackBuffer->Pitch);
                *(Row + X) = DestColor;
            }
        }
    }
    else
    {
        OneOverDistance = 1.0f / YDistance;
        if(PointA.y < PointB.y)
        {
            Start = PointA;
            End = PointB;
        }
        else
        {
            Start = PointB;
            End = PointA;
        }
        Min = Clamp(OffsetY, RoundS32(Start.y), OffsetY + BackBuffer->Height);
        Max = Clamp(OffsetY, RoundS32(End.y), OffsetY + BackBuffer->Height);
        for(s32 Y = Min;
            Y < Max;
            ++Y)
        {
            s32 X = RoundS32(Lerp(Start.x, OneOverDistance*(Y - Start.y), End.x));
            if(X >= OffsetX && X < OffsetX + BackBuffer->Width)
            {
                u32 *Row = (u32 *)((u8 *)BackBuffer->Memory + Y*BackBuffer->Pitch);
                *(Row + X) = DestColor;
            }
        }
    }
}
#endif
#pragma optimize("", on)

inline void
RenderAlignedRectangle(loaded_bitmap *BackBuffer, rectangle2 Rect, v4 Color,
                       s32 OffsetX = 0, s32 OffsetY = 0)
{
    if(Color.a == 1.0f)
    {
        RenderAlignedSolidRectangle(BackBuffer, Rect, Color.rgb, OffsetX, OffsetY);
    }
    else if(IsInvertedColor(Color))
    {
        RenderAlignedInvertedRectangle(BackBuffer, Rect, OffsetX, OffsetY);
    }
    else
    {
        RenderAlignedTranslucentRectangle(BackBuffer, Rect, Color, OffsetX, OffsetY);
    }
}

inline void
Clear(loaded_bitmap *BackBuffer, v3 Color, s32 OffsetX = 0, s32 OffsetY = 0)
{
    rectangle2 Rect;
    Rect.Min = V2(0, 0);
    Rect.Max = V2i(OffsetX + BackBuffer->Width, OffsetY + BackBuffer->Height);
    RenderAlignedSolidRectangle(BackBuffer, Rect, Color, OffsetX, OffsetY);
}

struct binary_node
{
    r32 SortKey;
    u32 Index;
    binary_node *Chain;
    binary_node *Prev;
    binary_node *Next;
};

inline void
Insert(memory_arena *Arena, binary_node **NodePtr, r32 SortKey, u32 Index)
{
    binary_node *Node;
    if(!(*NodePtr))
    {
        *NodePtr = PushStruct(Arena, binary_node);
        Node = *NodePtr;
        Node->SortKey = SortKey;
        Node->Index = Index;
        Node->Chain = 0;
        Node->Prev = 0;
        Node->Next = 0;
    }
    else
    {
        Node = *NodePtr;
        if(SortKey > Node->SortKey)
        {
            Insert(Arena, &Node->Next, SortKey, Index);
        }
        else if(SortKey < Node->SortKey)
        {
            Insert(Arena, &Node->Prev, SortKey, Index);
        }
        else
        {
            binary_node *Chain = 0;
            Insert(Arena, &Chain, SortKey, Index);
            Chain->Chain = Node->Chain;
            Node->Chain = Chain;
        }
    }
}

internal void
RenderTree(render_buffer *RenderBuffer, render_chunk *RenderChunk,
           binary_node *Node, s32 OffsetX, s32 OffsetY, u32 Flags)
{
    if(Node->Prev)
    {
        RenderTree(RenderBuffer, RenderChunk, Node->Prev, OffsetX, OffsetY, Flags);
    }
    for(binary_node *Chain = Node;
        Chain;
        Chain = Chain->Chain)
    {
        render_command_header *Header = (render_command_header *)((u8 *)RenderBuffer->Arena.Memory +
                                                                  Chain->Index);
        switch(Header->Command)
        {
            case RenderCommand_bitmap:
            {
                render_bitmap_data *Data = (render_bitmap_data *)(Header + 1);

                if(IsSet(Flags, RenderFlags_UsePipeline))
                {
                    RenderBitmap(RenderChunk, Data->Bitmap,
                                 Data->Origin,
                                 Data->XAxis, Data->YAxis,
                                 Data->Color, OffsetX, OffsetY);
                }
                else
                {
                    RenderBitmap(&RenderChunk->BackBuffer, Data->Bitmap,
                                 Data->Origin,
                                 Data->XAxis, Data->YAxis,
                                 Data->Color, OffsetX, OffsetY);
                }
            } break;

            case RenderCommand_aligned_rectangle:
            {
                render_aligned_rectangle_data *Data = (render_aligned_rectangle_data *)(Header + 1);
                RenderAlignedRectangle(&RenderChunk->BackBuffer, Data->Rect, Data->Color, OffsetX, OffsetY);
            } break;
            
            case RenderCommand_triangle:
            {
                render_triangle_data *Data = (render_triangle_data *)(Header + 1);
                if(IsSet(Flags, RenderFlags_UsePipeline))
                {
                    RenderTriangle(RenderChunk, Data->A, Data->B, Data->C,
                                   Data->Color, OffsetX, OffsetY);
                }
                else
                {
                }
            } break;
            
            case RenderCommand_clear:
            {
                render_clear_data *Data = (render_clear_data *)(Header + 1);
                Clear(&RenderChunk->BackBuffer, Data->Color, OffsetX, OffsetY);
            } break;

#if TROIDS_INTERNAL
            case RenderCommand_DEBUGrectangle:
            {
                render_DEBUGrectangle_data *Data = (render_DEBUGrectangle_data *)(Header + 1);

                DEBUGRenderSolidRectangle(&RenderChunk->BackBuffer, Data->Origin, Data->XAxis, Data->YAxis,
                                          Data->Color.rgb, OffsetX, OffsetY);
            } break;
            case RenderCommand_DEBUGtriangle:
            {
                render_DEBUGtriangle_data *Data = (render_DEBUGtriangle_data *)(Header + 1);

                DEBUGRenderSolidTriangle(&RenderChunk->BackBuffer, Data->A, Data->B, Data->C,
                                         Data->Color.rgb, OffsetX, OffsetY);
            } break;

            case RenderCommand_DEBUGcircle:
            {
                render_DEBUGcircle_data *Data = (render_DEBUGcircle_data *)(Header + 1);

                DEBUGRenderSolidCircle(&RenderChunk->BackBuffer, Data->P, Data->Radius, Data->Color.rgb, OffsetX, OffsetY);
            } break;
            
            case RenderCommand_DEBUGline:
            {
                render_DEBUGline_data *Data = (render_DEBUGline_data *)(Header + 1);

                DEBUGRenderLine(&RenderChunk->BackBuffer, Data->A, Data->B, Data->Color, OffsetX, OffsetY);
            } break;
#endif

            InvalidDefaultCase;
        }
    }
    if(Node->Next)
    {
        RenderTree(RenderBuffer, RenderChunk, Node->Next, OffsetX, OffsetY, Flags);
    }
}

struct render_tree_data
{
    u32 Flags;
    binary_node *Node;
    render_buffer *RenderBuffer;
    render_chunk *RenderChunk;
};

THREAD_CALLBACK(RenderTreeCallback)
{
    TIMED_FUNCTION();
    BEGIN_TIMED_BLOCK(Nothing, "Nothing");
    render_tree_data *Data = (render_tree_data *)Params;
    render_chunk *RenderChunk = Data->RenderChunk;

    {
        BEGIN_TIMED_BLOCK(GUID, "Render Tree");
        RenderTree(Data->RenderBuffer, RenderChunk, Data->Node,
                   RenderChunk->OffsetX, RenderChunk->OffsetY, Data->Flags);
        END_TIMED_BLOCK(GUID);
        if(IsSet(Data->Flags, RenderFlags_UsePipeline) && RenderChunk->Used)
        {
            RenderSamples(RenderChunk);
        }
    }
    END_TIMED_BLOCK(Nothing);
}

#define INSERT_RENDER_COMMAND(type, SortKey)                            \
    case RenderCommand_##type:                                          \
    {                                                                   \
        PackedBufferAdvance(Data, render_##type##_data, Cursor);        \
        Insert(&RenderBuffer->Arena, &SortTree, SortKey, Index);        \
    } break;


internal void
RenderBufferToBackBuffer(renderer_state *RendererState, render_buffer *RenderBuffer, u32 Flags = 0)
{
    binary_node *SortTree = 0;
    u8 *Cursor = (u8 *)RenderBuffer->Arena.Memory;
    u8 *End = Cursor + RenderBuffer->Arena.Used;
    while(Cursor != End)
    {
        render_command_header *Header = (render_command_header *)Cursor;
        u32 Index = (u32)((u8 *)Header - (u8 *)RenderBuffer->Arena.Memory);
        Cursor += sizeof(render_command_header);
        switch(Header->Command)
        {
            INSERT_RENDER_COMMAND(bitmap, Data->SortKey);
            INSERT_RENDER_COMMAND(aligned_rectangle, Data->SortKey);
            INSERT_RENDER_COMMAND(triangle, Data->SortKey);
            INSERT_RENDER_COMMAND(clear, -REAL32_MAX);
#if TROIDS_INTERNAL
            INSERT_RENDER_COMMAND(DEBUGrectangle, Data->SortKey);
            INSERT_RENDER_COMMAND(DEBUGtriangle, Data->SortKey);
            INSERT_RENDER_COMMAND(DEBUGcircle, Data->SortKey);
            INSERT_RENDER_COMMAND(DEBUGline, Data->SortKey);
#endif
            InvalidDefaultCase;
        }
    }
    if(SortTree)
    {
        render_tree_data RenderTreeData[RENDER_CHUNK_COUNT];
        thread_progress ThreadProgress[RENDER_CHUNK_COUNT];

        SplitWorkIntoSquares(RendererState->RenderChunks, RENDER_CHUNK_COUNT,
                             RendererState->BackBuffer.Width,
                             RendererState->BackBuffer.Height,
                             0, 0);
        
        for(s32 RenderChunkIndex = RENDER_CHUNK_COUNT-1;
            RenderChunkIndex >= 0;
            --RenderChunkIndex)
        {
            render_tree_data *Data = RenderTreeData + RenderChunkIndex;
            thread_progress *Progress = ThreadProgress + RenderChunkIndex;
            
            Data->Flags = Flags;
            Data->Node = SortTree;
            Data->RenderBuffer = RenderBuffer;
            Data->RenderChunk = RendererState->RenderChunks + RenderChunkIndex;
            if(RenderChunkIndex == 0)
            {
                RenderTreeCallback(Data);
                Progress->Finished = true;
            }
            else
            {
                PlatformPushThreadWork(RenderTreeCallback, Data, Progress);
            }
        }

        b32 Finished;
        do
        {
            Finished = true;
            for(s32 ProgressIndex = 0;
                ProgressIndex < RENDER_CHUNK_COUNT;
                ++ProgressIndex)
            {
                thread_progress *Progress = ThreadProgress + ProgressIndex;
                Finished &= Progress->Finished;
            }
        } while(!Finished);
    }
}
