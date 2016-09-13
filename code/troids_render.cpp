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

inline v2
WorldToScreenTransform(render_buffer *RenderBuffer, v3 WorldP)
{
    v2 Result = {};
    switch(RenderBuffer->Projection)
    {
        case Projection_Orthographic:
        {
            v3 NewP = WorldP - RenderBuffer->CameraP;
            r32 ScaleFactor = 1.0f / -(5.0f*NewP.z);
            r32 CosRot = Cos(RenderBuffer->CameraRot);
            r32 SinRot = Sin(RenderBuffer->CameraRot);
            {
                m33 RotMat =
                    {
                        CosRot, SinRot, 0,
                        -SinRot, CosRot, 0,
                        0, 0, 1,
                    };
                NewP = RotMat*NewP;
            }

            Result = (ScaleFactor*RenderBuffer->MetersToPixels*NewP.xy +
                                  0.5f*V2i(RenderBuffer->Width, RenderBuffer->Height));
        } break;

        case Projection_None:
        {
            Result = WorldP.xy;
        } break;

        InvalidDefaultCase;
    }
    return(Result);
}

inline void
PushRenderHeader(memory_arena *Arena, render_command Command)
{
    render_command_header *Header = PushStruct(Arena, render_command_header);
    Header->Command = Command;
}

inline void
PushBitmap(render_buffer *RenderBuffer, loaded_bitmap *Bitmap, v3 Origin, v2 XAxis, v2 YAxis,
           r32 Height, v4 Color)
{
    if(Bitmap->Height && Bitmap->Width)
    {
        PushRenderHeader(&RenderBuffer->Arena, RenderCommand_Bitmap);
        render_bitmap_data *Data = PushStruct(&RenderBuffer->Arena, render_bitmap_data);
        Data->Bitmap = Bitmap;
        Data->Origin = Origin;
        Data->XAxis = XAxis;
        Data->YAxis = YAxis;
        Data->Height = Height;
        Data->Color = Color;
        Data->SortKey = Origin.z;
    }
}

inline rectangle2
PushRectangle(render_buffer *RenderBuffer, v3 P, v2 Dim, v4 Color)
{
    PushRenderHeader(&RenderBuffer->Arena, RenderCommand_Rectangle);
    rectangle2 Rect = MinDim(P.xy, Dim);
    render_rectangle_data *Data = PushStruct(&RenderBuffer->Arena, render_rectangle_data);
    Data->Rect = Rect;
    Data->Color = Color;
    Data->SortKey = P.z;
    return(Rect);
}

// TODO(chris): How do I sort lines that move through z? This goes for bitmaps and rectangles also.
inline void
PushLine(render_buffer *RenderBuffer, v3 PointA, v3 PointB, v4 Color)
{
    PushRenderHeader(&RenderBuffer->Arena, RenderCommand_Line);
    render_line_data *Data = PushStruct(&RenderBuffer->Arena, render_line_data);
    Data->PointA = PointA;
    Data->PointB = PointB;
    Data->Color = Color;
    Data->SortKey = 0.5f*(PointA.z + PointB.z);
}


inline void
PushClear(render_buffer *RenderBuffer, v4 Color)
{
    PushRenderHeader(&RenderBuffer->Arena, RenderCommand_Clear);
    render_clear_data *Data = PushStruct(&RenderBuffer->Arena, render_clear_data);
    Data->Color = Color;
}

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
internal rectangle2
DrawText(render_buffer *RenderBuffer, text_layout *Layout, u32 TextLength, char *Text, u32 Flags)
{
    rectangle2 Result;
    v2 PInit = Layout->P;
    char Prev = 0;
    char *At = Text;
    r32 Ascent = Layout->Scale*Layout->Font->Ascent;
    r32 Height = Layout->Scale*Layout->Font->Height;
    r32 Descent = Height - Ascent;
    r32 LineAdvance = Layout->Scale*Layout->Font->LineAdvance;
    for(u32 AtIndex = 0;
        AtIndex < TextLength;
        ++AtIndex)
    {
        if(!IsSet(Flags, DrawTextFlags_Measure))
        {
            loaded_bitmap *Glyph = Layout->Font->Glyphs + *At;
            if(Glyph->Height && Glyph->Width)
            {
                v2 YAxis = V2(0, 1);
                v2 XAxis = -Perp(YAxis);
#if 0
                PushRectangle(&TranState->RenderBuffer, CenterDim(P, Scale*(XAxis + YAxis)),
                              V4(1.0f, 0.0f, 1.0f, 1.0f));
#endif
                PushBitmap(RenderBuffer, Glyph, V3(Layout->P + Height*V2(0.05f, -0.05f), TEXT_Z-1),
                           XAxis, YAxis, Layout->Scale*Glyph->Height, V4(0, 0, 0, 1));
                PushBitmap(RenderBuffer, Glyph, V3(Layout->P, TEXT_Z),
                           XAxis, YAxis, Layout->Scale*Glyph->Height);
            }
        }
        Prev = *At++;
        Layout->P.x += Layout->Scale*GetTextAdvance(Layout->Font, Prev, *At);
    }
    Result = MinMax(V2(PInit.x, Layout->P.y - Descent),
                    V2(Layout->P.x, Layout->P.y + Ascent));
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

internal rectangle2
DrawButton(render_buffer *RenderBuffer, text_layout *Layout, u32 TextLength, char *Text,
           v4 Color = INVERTED_COLOR, r32 Border = 0.0f)
{
    rectangle2 Result;
    v2 PInit = Layout->P;
    Layout->P.x += Border;
    Layout->P.y -= Border;
    Result = AddBorder(DrawText(RenderBuffer, Layout, TextLength, Text), Border);
    PushRectangle(RenderBuffer, V3(Result.Min, HUD_Z), GetDim(Result), Color);
    
    return(Result);
}

internal void
DrawFillBar(render_buffer *RenderBuffer, text_layout *Layout, r32 Used, r32 Max, u32 Precision = 0)
{
    char Text[256];
    u32 TextLength = _snprintf_s(Text, sizeof(Text), "%.*f / %.*f", Precision, Used, Precision, Max);
    rectangle2 TextRect = DrawText(RenderBuffer, Layout, TextLength, Text);
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
DrawFillBar(render_buffer *RenderBuffer, text_layout *Layout, u32 Used, u32 Max)
{
    DrawFillBar(RenderBuffer, Layout, (r32)Used, (r32)Max);
}

internal void
DrawFillBar(render_buffer *RenderBuffer, text_layout *Layout, u64 Used, u64 Max)
{
    DrawFillBar(RenderBuffer, Layout, (r32)Used, (r32)Max);
}

#if 1
#pragma optimize("gts", on)
#endif
// TODO(chris): Further optimization
internal void
RenderBitmap(loaded_bitmap *BackBuffer, loaded_bitmap *Bitmap, v2 Origin, v2 XAxis, v2 YAxis,
             v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f), s32 OffsetX = 0, s32 OffsetY = 0)
{
    Color.rgb *= Color.a;
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
    __m128 TintR = _mm_set_ps1(Color.r);
    __m128 TintG = _mm_set_ps1(Color.g);
    __m128 TintB = _mm_set_ps1(Color.b);
    __m128 TintA = _mm_set_ps1(Color.a);
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
            __m128 TexelAG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelA, 8), ColorMask));
            __m128 TexelAB = _mm_cvtepi32_ps(_mm_and_si128(TexelA, ColorMask));
            __m128 TexelAA = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelA, 24), ColorMask));

            __m128 TexelBR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelB, 16), ColorMask));
            __m128 TexelBG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelB, 8), ColorMask));
            __m128 TexelBB = _mm_cvtepi32_ps(_mm_and_si128(TexelB, ColorMask));
            __m128 TexelBA = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelB, 24), ColorMask));

            __m128 TexelCR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelC, 16), ColorMask));
            __m128 TexelCG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelC, 8), ColorMask));
            __m128 TexelCB = _mm_cvtepi32_ps(_mm_and_si128(TexelC, ColorMask));
            __m128 TexelCA = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelC, 24), ColorMask));

            __m128 TexelDR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelD, 16), ColorMask));
            __m128 TexelDG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelD, 8), ColorMask));
            __m128 TexelDB = _mm_cvtepi32_ps(_mm_and_si128(TexelD, ColorMask));
            __m128 TexelDA = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(TexelD, 24), ColorMask));

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

            __m128 DA = _mm_sub_ps(RealOne, _mm_mul_ps(Inv255, _mm_mul_ps(LerpA, TintA)));

            __m128i Pixels = _mm_loadu_si128((__m128i *)Pixel);
#if DEBUG_BITMAPS
            __m128i IntMask = _mm_set_epi32(Mask.m128_u32[3], Mask.m128_u32[2],
                                            Mask.m128_u32[1], Mask.m128_u32[0]);
            Pixels = _mm_or_si128(_mm_andnot_si128(IntMask, Pixels), _mm_and_si128(Blue, IntMask));
#endif

            __m128i Result = _mm_or_si128(
                _mm_or_si128(
                    _mm_slli_epi32(
                        _mm_cvtps_epi32(_mm_add_ps(
                                            _mm_mul_ps(TintR, LerpR),
                                            _mm_mul_ps(DA,
                                                       _mm_cvtepi32_ps(
                                                           _mm_and_si128(
                                                               _mm_srli_epi32(Pixels, 16),
                                                               ColorMask))))),
                        16),
                    _mm_slli_epi32(
                        _mm_cvtps_epi32(
                            _mm_add_ps(
                                _mm_mul_ps(TintG, LerpG),
                                _mm_mul_ps(DA,
                                           _mm_cvtepi32_ps(
                                               _mm_and_si128(
                                                   _mm_srli_epi32(Pixels, 8),
                                                   ColorMask))))),
                        8)),
                    _mm_cvtps_epi32(
                        _mm_add_ps(
                            _mm_mul_ps(TintB, LerpB),
                            _mm_mul_ps(DA, _mm_cvtepi32_ps(_mm_and_si128(Pixels, ColorMask))))));

            _mm_storeu_si128((__m128i *)Pixel, Result);

            Pixel += 4;
            X4 = _mm_add_ps(X4, Four);
        }
        PixelRow += BackBuffer->Pitch;
    }
}

// TODO(chris): Further optimization
internal void
RenderTranslucentRectangle(loaded_bitmap *BackBuffer, rectangle2 Rect, v4 Color,
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

            __m128i Result = _mm_or_si128(_mm_or_si128(_mm_slli_epi32(_mm_cvtps_epi32(DR), 16),
                                                       _mm_slli_epi32(_mm_cvtps_epi32(DG), 8)),
                                          _mm_cvtps_epi32(DB));

            _mm_storeu_si128((__m128i *)Dest, Result);

            Dest += 4;
            _mm_add_epi32(X4, Four);
        }
        PixelRow += BackBuffer->Pitch;
    }
}

// TODO(chris): SIMD this!
internal void
RenderInvertedRectangle(loaded_bitmap *BackBuffer, rectangle2 Rect, s32 OffsetX = 0, s32 OffsetY = 0)
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

// TODO(chris): SIMD this!
internal void
RenderSolidRectangle(loaded_bitmap *BackBuffer, rectangle2 Rect, v3 Color,
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

inline void
RenderRectangle(loaded_bitmap *BackBuffer, rectangle2 Rect, v4 Color, s32 OffsetX = 0, s32 OffsetY = 0)
{
    if(Color.a == 1.0f)
    {
        RenderSolidRectangle(BackBuffer, Rect, Color.rgb, OffsetX, OffsetY);
    }
    else if(IsInvertedColor(Color))
    {
        RenderInvertedRectangle(BackBuffer, Rect, OffsetX, OffsetY);
    }
    else
    {
        RenderTranslucentRectangle(BackBuffer, Rect, Color, OffsetX, OffsetY);
    }
}

internal void
RenderLine(loaded_bitmap *BackBuffer, v2 PointA, v2 PointB, v4 Color, s32 OffsetX = 0, s32 OffsetY = 0)
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
            if(Y >= 0 && Y < BackBuffer->Height)
            {
                *((u32 *)BackBuffer->Memory + Y*BackBuffer->Width + X) = DestColor;
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
            if(X >= 0 && X < BackBuffer->Width)
            {
                *((u32 *)BackBuffer->Memory + Y*BackBuffer->Width + X) = DestColor;
            }
        }
    }
}
#pragma optimize("", on)

inline void
Clear(loaded_bitmap *BackBuffer, v4 Color, s32 OffsetX = 0, s32 OffsetY = 0)
{
    rectangle2 Rect;
    Rect.Min = V2(0, 0);
    Rect.Max = V2i(OffsetX + BackBuffer->Width, OffsetY + BackBuffer->Height);
    RenderRectangle(BackBuffer, Rect, Color, OffsetX, OffsetY);
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
RenderTree(render_buffer *RenderBuffer, binary_node *Node, loaded_bitmap *BackBuffer,
           s32 OffsetX, s32 OffsetY)
{
    if(Node->Prev)
    {
        RenderTree(RenderBuffer, Node->Prev, BackBuffer, OffsetX, OffsetY);
    }
    for(binary_node *Chain = Node;
        Chain;
        Chain = Chain->Chain)
    {
        render_command_header *Header = (render_command_header *)((u8 *)RenderBuffer->Arena.Memory +
                                                                  Chain->Index);
        switch(Header->Command)
        {
            case RenderCommand_Bitmap:
            {
                render_bitmap_data *Data = (render_bitmap_data *)(Header + 1);
                v2 YAxis = Data->YAxis*Data->Height;
                v2 XAxis = Data->XAxis*Data->Height*((r32)Data->Bitmap->Width/(r32)Data->Bitmap->Height);
                v3 Origin = Data->Origin - V3(Hadamard(Data->Bitmap->Align, XAxis + YAxis), 0);
                
                XAxis = WorldToScreenTransform(RenderBuffer, Origin + V3(XAxis, 0));
                YAxis = WorldToScreenTransform(RenderBuffer, Origin + V3(YAxis, 0));
                v2 ScreenOrigin = WorldToScreenTransform(RenderBuffer, Origin);
                XAxis -= ScreenOrigin;
                YAxis -= ScreenOrigin;

                RenderBitmap(BackBuffer, Data->Bitmap,
                             ScreenOrigin,
                             XAxis, YAxis,
                             Data->Color, OffsetX, OffsetY);
            } break;

            case RenderCommand_Rectangle:
            {
                render_rectangle_data *Data = (render_rectangle_data *)(Header + 1);
                RenderRectangle(BackBuffer, Data->Rect, Data->Color, OffsetX, OffsetY);
            } break;
            
            case RenderCommand_Line:
            {
                render_line_data *Data = (render_line_data *)(Header + 1);
                RenderLine(BackBuffer, Data->PointA.xy, Data->PointB.xy, Data->Color, OffsetX, OffsetY);
            } break;
            
            case RenderCommand_Clear:
            {
                render_clear_data *Data = (render_clear_data *)(Header + 1);
                Clear(BackBuffer, Data->Color, OffsetX, OffsetY);
            } break;

            InvalidDefaultCase;
        }
    }
    if(Node->Next)
    {
        RenderTree(RenderBuffer, Node->Next, BackBuffer, OffsetX, OffsetY);
    }
}

struct render_tree_data
{
    render_buffer *RenderBuffer;
    binary_node *Node;
    loaded_bitmap BackBuffer;
    s32 OffsetX;
    s32 OffsetY;
};

THREAD_CALLBACK(RenderTreeCallback)
{
    TIMED_FUNCTION();
    render_tree_data *Data = (render_tree_data *)Params;
    RenderTree(Data->RenderBuffer, Data->Node, &Data->BackBuffer, Data->OffsetX, Data->OffsetY);
}

inline void
SplitWorkIntoSquares(render_buffer *RenderBuffer, binary_node *Node, void *Memory,
                     u32 CoreCount, s32 Width, s32 Height, s32 Pitch, s32 OffsetX, s32 OffsetY,
                     render_tree_data *Data)
{
    if(CoreCount == 1)
    {
        Data->RenderBuffer = RenderBuffer;
        Data->Node = Node;
        Data->OffsetX = OffsetX;
        Data->OffsetY = OffsetY;
        Data->BackBuffer.Height = Height;
        Data->BackBuffer.Width = Width;
        Data->BackBuffer.Pitch = Pitch;
        Data->BackBuffer.Memory = Memory;
    }
    else if(CoreCount & 1)
    {
        Assert(!"Odd core count not supported");
    }
    else
    {
        u32 HalfCores = CoreCount/2;
        if(Width >= Height)
        {
            s32 HalfWidth = Width/2;
            SplitWorkIntoSquares(RenderBuffer, Node, Memory,
                      HalfCores, HalfWidth, Height, Pitch, OffsetX, OffsetY,
                      Data);
            SplitWorkIntoSquares(RenderBuffer, Node, Memory,
                      HalfCores, HalfWidth, Height, Pitch, OffsetX+HalfWidth, OffsetY,
                      Data+HalfCores);
        }
        else
        {
            s32 HalfHeight = Height/2;
            SplitWorkIntoSquares(RenderBuffer, Node, Memory,
                      HalfCores, Width, HalfHeight, Pitch, OffsetX, OffsetY,
                      Data);
            SplitWorkIntoSquares(RenderBuffer, Node, Memory,
                      HalfCores, Width, HalfHeight, Pitch, OffsetX, OffsetY+HalfHeight,
                      Data+HalfCores);
        }
    }
}

inline void
SplitWorkIntoHorizontalStrips(render_buffer *RenderBuffer, binary_node *Node, void *Memory,
                              u32 CoreCount, s32 Width, s32 Height, s32 Pitch, render_tree_data *Data)
{
    s32 OffsetY = 0;
    r32 InverseCoreCount = 1.0f / CoreCount;
    for(u32 CoreIndex = 0;
        CoreIndex < CoreCount;
        ++CoreIndex)
    {
        render_tree_data *CoreData = Data + CoreIndex;

        s32 NextOffsetY = RoundS32(Height*(CoreIndex+1)*InverseCoreCount);
        
        CoreData->RenderBuffer = RenderBuffer;
        CoreData->Node = Node;
        CoreData->OffsetX = 0;
        CoreData->OffsetY = OffsetY;
        CoreData->BackBuffer.Height = NextOffsetY-OffsetY;
        CoreData->BackBuffer.Width = Width;
        CoreData->BackBuffer.Pitch = Pitch;
        CoreData->BackBuffer.Memory = Memory;

        OffsetY = NextOffsetY;
    }
}

inline void
SplitWorkIntoVerticalStrips(render_buffer *RenderBuffer, binary_node *Node, void *Memory,
                            u32 CoreCount, s32 Width, s32 Height, s32 Pitch, render_tree_data *Data)
{
    s32 OffsetX = 0;
    r32 InverseCoreCount = 1.0f / CoreCount;
    for(u32 CoreIndex = 0;
        CoreIndex < CoreCount;
        ++CoreIndex)
    {
        render_tree_data *CoreData = Data + CoreIndex;

        s32 NextOffsetX = RoundS32(Width*(CoreIndex+1)*InverseCoreCount);
        
        CoreData->RenderBuffer = RenderBuffer;
        CoreData->Node = Node;
        CoreData->OffsetX = OffsetX;
        CoreData->OffsetY = 0;
        CoreData->BackBuffer.Height = Height;
        CoreData->BackBuffer.Width = NextOffsetX-OffsetX;
        CoreData->BackBuffer.Pitch = Pitch;
        CoreData->BackBuffer.Memory = Memory;

        OffsetX = NextOffsetX;
    }
}

internal void
RenderBufferToBackBuffer(render_buffer *RenderBuffer, loaded_bitmap *BackBuffer)
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
            case RenderCommand_Bitmap:
            {
                PackedBufferAdvance(Data, render_bitmap_data, Cursor);
                Insert(&RenderBuffer->Arena, &SortTree, Data->SortKey, Index);
            } break;

            case RenderCommand_Rectangle:
            {
                PackedBufferAdvance(Data, render_rectangle_data, Cursor);
                Insert(&RenderBuffer->Arena, &SortTree, Data->SortKey, Index);
            } break;
            
            case RenderCommand_Line:
            {
                PackedBufferAdvance(Data, render_line_data, Cursor);
                Insert(&RenderBuffer->Arena, &SortTree, Data->SortKey, Index);
            } break;
            
            case RenderCommand_Clear:
            {
                PackedBufferAdvance(Data, render_clear_data, Cursor);
                Insert(&RenderBuffer->Arena, &SortTree, -REAL32_MAX, Index);
            } break;

            InvalidDefaultCase;
        }
    }
    if(SortTree)
    {
        render_tree_data RenderTreeData[64];
        thread_progress ThreadProgress[64];
        // TODO(chris): Optimize this for the number of logical cores.
        u32 CoreCount = 64;
#if 0
        SplitWorkIntoVerticalStrips(RenderBuffer, SortTree, BackBuffer->Memory, CoreCount,
            BackBuffer->Width, BackBuffer->Height, BackBuffer->Pitch,
            RenderTreeData);
#else
        SplitWorkIntoSquares(RenderBuffer, SortTree, BackBuffer->Memory, CoreCount,
            BackBuffer->Width, BackBuffer->Height, BackBuffer->Pitch, 0, 0,
            RenderTreeData);
#endif
        
        for(u32 RenderChunkIndex = 1;
            RenderChunkIndex < CoreCount;
            ++RenderChunkIndex)
        {
            render_tree_data *Data = RenderTreeData + RenderChunkIndex;
            thread_progress *Progress = ThreadProgress + RenderChunkIndex;
            PlatformPushThreadWork(RenderTreeCallback, Data, Progress);
        }
        RenderTreeCallback(&RenderTreeData[0]);
        ThreadProgress[0].Finished = true;

        b32 Finished;
        do
        {
            Finished = true;
            for(u32 ProgressIndex = 0;
                ProgressIndex < CoreCount;
                ++ProgressIndex)
            {
                thread_progress *Progress = ThreadProgress + ProgressIndex;
                Finished &= Progress->Finished;
            }
        } while(!Finished);
    }
}
