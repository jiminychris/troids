/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#define TEXT_Z 200000.0f
#define HUD_Z 100000.0f

inline void
PushRenderHeader(memory_arena *Arena, render_command Command)
{
    render_command_header *Header = PushStruct(Arena, render_command_header);
    Header->Command = Command;
}

inline void
PushBitmap(render_buffer *RenderBuffer, loaded_bitmap *Bitmap, v3 Origin, v2 XAxis, v2 YAxis,
           r32 Scale, v4 Color)
{
    if(Bitmap->Height && Bitmap->Width)
    {
        PushRenderHeader(&RenderBuffer->Arena, RenderCommand_Bitmap);
        render_bitmap_data *Data = PushStruct(&RenderBuffer->Arena, render_bitmap_data);
        Data->Bitmap = Bitmap;
        Data->Origin = Origin;
        Data->XAxis = XAxis;
        Data->YAxis = YAxis;
        Data->Scale = Scale;
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
                           XAxis, YAxis, Layout->Scale, V4(0, 0, 0, 1));
                PushBitmap(RenderBuffer, Glyph, V3(Layout->P, TEXT_Z),
                           XAxis, YAxis, Layout->Scale);
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

#pragma optimize("", on)
// TODO(chris): SIMD this!
internal void
RenderBitmap(game_backbuffer *BackBuffer, loaded_bitmap *Bitmap, v2 Origin, v2 XAxis, v2 YAxis,
             r32 Scale, v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    Color.rgb *= Color.a;
    XAxis *= Scale*Bitmap->Width;
    YAxis *= Scale*Bitmap->Height;
    Origin -= Hadamard(Bitmap->Align, XAxis + YAxis);
    s32 XMin = Clamp(0, Floor(Minimum(Minimum(Origin.x, Origin.x + XAxis.x),
                                      Minimum(Origin.x + YAxis.x, Origin.x + XAxis.x + YAxis.x))),
                     BackBuffer->Width);
    s32 YMin = Clamp(0, Floor(Minimum(Minimum(Origin.y, Origin.y + XAxis.y),
                                      Minimum(Origin.y + YAxis.y, Origin.y + XAxis.y + YAxis.y))),
                     BackBuffer->Height);

    s32 XMax = Clamp(0, Ceiling(Maximum(Maximum(Origin.x, Origin.x + XAxis.x),
                                        Maximum(Origin.x + YAxis.x, Origin.x + XAxis.x + YAxis.x))),
                     BackBuffer->Width);
#if 1
    if(XMax == BackBuffer->Width)
    {
        u32 Spillover = (4-((XMax-XMin)%4));
        XMin -= Spillover;
    }
#endif
    s32 YMax = Clamp(0, Ceiling(Maximum(Maximum(Origin.y, Origin.y + XAxis.y),
                                        Maximum(Origin.y + YAxis.y, Origin.y + XAxis.y + YAxis.y))),
                     BackBuffer->Height);

    __m128 Inv255 = _mm_set_ps1(1.0f / 255.0f);
    __m128 InvXAxisLengthSq = _mm_set_ps1(1.0f / LengthSq(XAxis));
    __m128 InvYAxisLengthSq = _mm_set_ps1(1.0f / LengthSq(YAxis));
    __m128 BitmapInternalWidth = _mm_set_ps1((r32)(Bitmap->Width - 2));
    __m128 BitmapInternalHeight = _mm_set_ps1((r32)(Bitmap->Height - 2));
    __m128 BitmapWidth = _mm_set_ps1((r32)Bitmap->Width);
    __m128i BitmapHeight = _mm_set1_epi32(Bitmap->Height);
    __m128i One = _mm_set1_epi32(1);
    __m128 RealOne = _mm_set_ps1(1.0f);
    __m128 Half = _mm_set_ps1(0.5f);
    __m128i ColorMask = _mm_set1_epi32(0xFF);
    __m128 TintR = _mm_set_ps1(Color.r);
    __m128 TintG = _mm_set_ps1(Color.g);
    __m128 TintB = _mm_set_ps1(Color.b);
    __m128 TintA = _mm_set_ps1(Color.a);
    u8 *PixelRow = (u8 *)BackBuffer->Memory + (BackBuffer->Pitch*YMin);
    TIMED_LOOP_FUNCTION((YMax-YMin)*(XMax-XMin));
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Pixel = (u32 *)PixelRow + XMin;
        for(s32 X = XMin;
            X < XMax;
            X += 4)
        {
            v2 TestPointA = V2i(X + 0, Y) - Origin;
            v2 TestPointB = V2i(X + 1, Y) - Origin;
            v2 TestPointC = V2i(X + 2, Y) - Origin;
            v2 TestPointD = V2i(X + 3, Y) - Origin;

            __m128 U = _mm_mul_ps(_mm_set_ps(TestPointD.x*XAxis.x + TestPointA.y*XAxis.y,
                                             TestPointC.x*XAxis.x + TestPointB.y*XAxis.y,
                                             TestPointB.x*XAxis.x + TestPointC.y*XAxis.y,
                                             TestPointA.x*XAxis.x + TestPointD.y*XAxis.y),
                                  InvXAxisLengthSq);
            __m128 V = _mm_mul_ps(_mm_set_ps(TestPointD.x*YAxis.x + TestPointA.y*YAxis.y,
                                             TestPointC.x*YAxis.x + TestPointB.y*YAxis.y,
                                             TestPointB.x*YAxis.x + TestPointC.y*YAxis.y,
                                             TestPointA.x*YAxis.x + TestPointD.y*YAxis.y),
                                  InvYAxisLengthSq);
            __m128 Mask;
            Mask.m128_u32[0] = 0xFFFFFFFF;
            Mask.m128_u32[1] = 0xFFFFFFFF;
            Mask.m128_u32[2] = 0xFFFFFFFF;
            Mask.m128_u32[3] = 0xFFFFFFFF;

            if(U.m128_f32[0] < 0.0f || U.m128_f32[0] > 1.0f ||
               V.m128_f32[0] < 0.0f || V.m128_f32[0] > 1.0f)
            {
                Mask.m128_f32[0] = 0;
            }
            if(U.m128_f32[1] < 0.0f || U.m128_f32[1] > 1.0f ||
               V.m128_f32[1] < 0.0f || V.m128_f32[1] > 1.0f)
            {
                Mask.m128_f32[1] = 0;
            }
            if(U.m128_f32[2] < 0.0f || U.m128_f32[2] > 1.0f ||
               V.m128_f32[2] < 0.0f || V.m128_f32[2] > 1.0f)
            {
                Mask.m128_f32[2] = 0;
            }
            if(U.m128_f32[3] < 0.0f || U.m128_f32[3] > 1.0f ||
               V.m128_f32[3] < 0.0f || V.m128_f32[3] > 1.0f)
            {
                Mask.m128_f32[3] = 0;
            }
            __m128 tX = _mm_add_ps(_mm_mul_ps(U, BitmapInternalWidth), Half);
            __m128 tY = _mm_add_ps(_mm_mul_ps(V, BitmapInternalHeight), Half);

            __m128 ttX = _mm_cvtepi32_ps(_mm_cvttps_epi32(_mm_and_ps(tX, Mask)));
            __m128 ttY = _mm_cvtepi32_ps(_mm_cvttps_epi32(_mm_and_ps(tY, Mask)));

            __m128 OffsetA = _mm_add_ps(_mm_mul_ps(ttY, BitmapWidth), ttX);
            __m128 OffsetB = _mm_add_ps(OffsetA, RealOne);
            __m128 OffsetC = _mm_add_ps(OffsetA, BitmapWidth);
            __m128 OffsetD = _mm_add_ps(OffsetC, RealOne);

            u32 TexelA[] = {*((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetA).m128i_u32[0]),
                            *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetA).m128i_u32[1]),
                            *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetA).m128i_u32[2]),
                            *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetA).m128i_u32[3])};
            u32 TexelB[] = {*((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetB).m128i_u32[0]),
                            *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetB).m128i_u32[1]),
                            *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetB).m128i_u32[2]),
                            *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetB).m128i_u32[3])};
            u32 TexelC[] = {*((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetC).m128i_u32[0]),
                            *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetC).m128i_u32[1]),
                            *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetC).m128i_u32[2]),
                            *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetC).m128i_u32[3])};
            u32 TexelD[] = {*((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetD).m128i_u32[0]),
                            *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetD).m128i_u32[1]),
                            *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetD).m128i_u32[2]),
                            *((u32 *)Bitmap->Memory + _mm_cvtps_epi32(OffsetD).m128i_u32[3])};


            __m128 TexelAR = _mm_cvtepi32_ps(_mm_set_epi32((TexelA[3] >> 16)&0xFF,
                                                           (TexelA[2] >> 16)&0xFF,
                                                           (TexelA[1] >> 16)&0xFF,
                                                           (TexelA[0] >> 16)&0xFF));
            __m128 TexelAG = _mm_cvtepi32_ps(_mm_set_epi32((TexelA[3] >> 8)&0xFF,
                                                           (TexelA[2] >> 8)&0xFF,
                                                           (TexelA[1] >> 8)&0xFF,
                                                           (TexelA[0] >> 8)&0xFF));
            __m128 TexelAB = _mm_cvtepi32_ps(_mm_set_epi32((TexelA[3] >> 0)&0xFF,
                                                           (TexelA[2] >> 0)&0xFF,
                                                           (TexelA[1] >> 0)&0xFF,
                                                           (TexelA[0] >> 0)&0xFF));
            __m128 TexelAA = _mm_cvtepi32_ps(_mm_set_epi32((TexelA[3] >> 24)&0xFF,
                                                           (TexelA[2] >> 24)&0xFF,
                                                           (TexelA[1] >> 24)&0xFF,
                                                           (TexelA[0] >> 24)&0xFF));

            __m128 TexelBR = _mm_cvtepi32_ps(_mm_set_epi32((TexelB[3] >> 16)&0xFF,
                                                           (TexelB[2] >> 16)&0xFF,
                                                           (TexelB[1] >> 16)&0xFF,
                                                           (TexelB[0] >> 16)&0xFF));
            __m128 TexelBG = _mm_cvtepi32_ps(_mm_set_epi32((TexelB[3] >> 8)&0xFF,
                                                           (TexelB[2] >> 8)&0xFF,
                                                           (TexelB[1] >> 8)&0xFF,
                                                           (TexelB[0] >> 8)&0xFF));
            __m128 TexelBB = _mm_cvtepi32_ps(_mm_set_epi32((TexelB[3] >> 0)&0xFF,
                                                           (TexelB[2] >> 0)&0xFF,
                                                           (TexelB[1] >> 0)&0xFF,
                                                           (TexelB[0] >> 0)&0xFF));
            __m128 TexelBA = _mm_cvtepi32_ps(_mm_set_epi32((TexelB[3] >> 24)&0xFF,
                                                           (TexelB[2] >> 24)&0xFF,
                                                           (TexelB[1] >> 24)&0xFF,
                                                           (TexelB[0] >> 24)&0xFF));

            __m128 TexelCR = _mm_cvtepi32_ps(_mm_set_epi32((TexelC[3] >> 16)&0xFF,
                                                           (TexelC[2] >> 16)&0xFF,
                                                           (TexelC[1] >> 16)&0xFF,
                                                           (TexelC[0] >> 16)&0xFF));
            __m128 TexelCG = _mm_cvtepi32_ps(_mm_set_epi32((TexelC[3] >> 8)&0xFF,
                                                           (TexelC[2] >> 8)&0xFF,
                                                           (TexelC[1] >> 8)&0xFF,
                                                           (TexelC[0] >> 8)&0xFF));
            __m128 TexelCB = _mm_cvtepi32_ps(_mm_set_epi32((TexelC[3] >> 0)&0xFF,
                                                           (TexelC[2] >> 0)&0xFF,
                                                           (TexelC[1] >> 0)&0xFF,
                                                           (TexelC[0] >> 0)&0xFF));
            __m128 TexelCA = _mm_cvtepi32_ps(_mm_set_epi32((TexelC[3] >> 24)&0xFF,
                                                           (TexelC[2] >> 24)&0xFF,
                                                           (TexelC[1] >> 24)&0xFF,
                                                           (TexelC[0] >> 24)&0xFF));

            __m128 TexelDR = _mm_cvtepi32_ps(_mm_set_epi32((TexelD[3] >> 16)&0xFF,
                                                           (TexelD[2] >> 16)&0xFF,
                                                           (TexelD[1] >> 16)&0xFF,
                                                           (TexelD[0] >> 16)&0xFF));
            __m128 TexelDG = _mm_cvtepi32_ps(_mm_set_epi32((TexelD[3] >> 8)&0xFF,
                                                           (TexelD[2] >> 8)&0xFF,
                                                           (TexelD[1] >> 8)&0xFF,
                                                           (TexelD[0] >> 8)&0xFF));
            __m128 TexelDB = _mm_cvtepi32_ps(_mm_set_epi32((TexelD[3] >> 0)&0xFF,
                                                           (TexelD[2] >> 0)&0xFF,
                                                           (TexelD[1] >> 0)&0xFF,
                                                           (TexelD[0] >> 0)&0xFF));
            __m128 TexelDA = _mm_cvtepi32_ps(_mm_set_epi32((TexelD[3] >> 24)&0xFF,
                                                           (TexelD[2] >> 24)&0xFF,
                                                           (TexelD[1] >> 24)&0xFF,
                                                           (TexelD[0] >> 24)&0xFF));

            __m128 tU = _mm_sub_ps(tX, ttX);
            __m128 tV = _mm_sub_ps(tY, ttY);
            __m128 tUInv = _mm_sub_ps(RealOne, tU);
            __m128 tVInv = _mm_sub_ps(RealOne, tV);

            __m128 LerpABR = _mm_add_ps(_mm_mul_ps(TexelAR, tUInv), _mm_mul_ps(TexelBR, tU));
            __m128 LerpABG = _mm_add_ps(_mm_mul_ps(TexelAG, tUInv), _mm_mul_ps(TexelBG, tU));
            __m128 LerpABB = _mm_add_ps(_mm_mul_ps(TexelAB, tUInv), _mm_mul_ps(TexelBB, tU));
            __m128 LerpABA = _mm_add_ps(_mm_mul_ps(TexelAA, tUInv), _mm_mul_ps(TexelBA, tU));

            __m128 LerpCDR = _mm_add_ps(_mm_mul_ps(TexelCR, tUInv), _mm_mul_ps(TexelDR, tU));
            __m128 LerpCDG = _mm_add_ps(_mm_mul_ps(TexelCG, tUInv), _mm_mul_ps(TexelDG, tU));
            __m128 LerpCDB = _mm_add_ps(_mm_mul_ps(TexelCB, tUInv), _mm_mul_ps(TexelDB, tU));
            __m128 LerpCDA = _mm_add_ps(_mm_mul_ps(TexelCA, tUInv), _mm_mul_ps(TexelDA, tU));

            __m128 LerpR = _mm_and_ps(_mm_add_ps(_mm_mul_ps(LerpABR, tVInv),
                                                    _mm_mul_ps(LerpCDR, tV)), Mask);
            __m128 LerpG = _mm_and_ps(_mm_add_ps(_mm_mul_ps(LerpABG, tVInv),
                                                    _mm_mul_ps(LerpCDG, tV)), Mask);
            __m128 LerpB = _mm_and_ps(_mm_add_ps(_mm_mul_ps(LerpABB, tVInv),
                                                    _mm_mul_ps(LerpCDB, tV)), Mask);
            __m128 LerpA = _mm_and_ps(_mm_add_ps(_mm_mul_ps(LerpABA, tVInv),
                                                    _mm_mul_ps(LerpCDA, tV)), Mask);

            __m128 DA = _mm_sub_ps(RealOne, _mm_mul_ps(Inv255, _mm_mul_ps(LerpA, TintA)));

            // TODO(chris): Crashed here once when drawing in the top right corner. Check that out.
            __m128i Pixels = _mm_set_epi32(*(Pixel + 3),
                                           *(Pixel + 2),
                                           *(Pixel + 1),
                                           *(Pixel + 0));
            __m128 DR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Pixels, 16), ColorMask));
            __m128 DG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Pixels, 8), ColorMask));
            __m128 DB = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Pixels, 0), ColorMask));

            __m128i ResultR = _mm_slli_epi32(_mm_cvtps_epi32(
                                                 _mm_add_ps(
                                                     _mm_mul_ps(TintR, LerpR),
                                                     _mm_mul_ps(DA, DR))), 16);
            __m128i ResultG = _mm_slli_epi32(_mm_cvtps_epi32(
                                                 _mm_add_ps(
                                                     _mm_mul_ps(TintG, LerpG),
                                                     _mm_mul_ps(DA, DG))), 8);
            __m128i ResultB = _mm_slli_epi32(_mm_cvtps_epi32(
                                                 _mm_add_ps(
                                                     _mm_mul_ps(TintB, LerpB),
                                                     _mm_mul_ps(DA, DB))), 0);

            if((LerpA.m128_f32[0] > 0.0f) &&
               (LerpA.m128_f32[1] > 0.0f) &&
               (LerpA.m128_f32[2] > 0.0f) &&
               (LerpA.m128_f32[3] > 0.0f))
            {
                int A = 0;
            }

            *(Pixel + 0) = ResultR.m128i_u32[0] | ResultG.m128i_u32[0] | ResultB.m128i_u32[0];
            *(Pixel + 1) = ResultR.m128i_u32[1] | ResultG.m128i_u32[1] | ResultB.m128i_u32[1];
            *(Pixel + 2) = ResultR.m128i_u32[2] | ResultG.m128i_u32[2] | ResultB.m128i_u32[2];
            *(Pixel + 3) = ResultR.m128i_u32[3] | ResultG.m128i_u32[3] | ResultB.m128i_u32[3];
            Pixel += 4;
        }
        PixelRow += BackBuffer->Pitch;
    }
}

// TODO(chris): SIMD this!
internal void
RenderTranslucentRectangle(game_backbuffer *BackBuffer, rectangle2 Rect, v4 Color)
{
    TIMED_FUNCTION();
    s32 XMin = Clamp(0, RoundS32(Rect.Min.x), BackBuffer->Width);
    s32 YMin = Clamp(0, RoundS32(Rect.Min.y), BackBuffer->Height);
    s32 XMax = Clamp(0, RoundS32(Rect.Max.x), BackBuffer->Width);
    s32 YMax = Clamp(0, RoundS32(Rect.Max.y), BackBuffer->Height);

    Color.rgb *= 255.0f;

    Color.rgb *= Color.a;
    r32 SR = Color.r;
    r32 SG = Color.g;
    r32 SB = Color.b;
    r32 DA = 1.0f - Color.a;

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
            r32 DR = (r32)((*Dest >> 16) & 0xFF);
            r32 DG = (r32)((*Dest >> 8) & 0xFF);
            r32 DB = (r32)((*Dest >> 0) & 0xFF);
            *Dest++ = ((RoundU32(DA*DR + SR) << 16) | 
                       (RoundU32(DA*DG + SG) << 8) | 
                       (RoundU32(DA*DB + SB) << 0));
        }
        PixelRow += BackBuffer->Pitch;
    }
}

// TODO(chris): SIMD this!
internal void
RenderInvertedRectangle(game_backbuffer *BackBuffer, rectangle2 Rect)
{
    TIMED_FUNCTION();
    s32 XMin = Clamp(0, RoundS32(Rect.Min.x), BackBuffer->Width);
    s32 YMin = Clamp(0, RoundS32(Rect.Min.y), BackBuffer->Height);
    s32 XMax = Clamp(0, RoundS32(Rect.Max.x), BackBuffer->Width);
    s32 YMax = Clamp(0, RoundS32(Rect.Max.y), BackBuffer->Height);

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
RenderSolidRectangle(game_backbuffer *BackBuffer, rectangle2 Rect, v3 Color)
{
    TIMED_FUNCTION();
    s32 XMin = Clamp(0, RoundS32(Rect.Min.x), BackBuffer->Width);
    s32 YMin = Clamp(0, RoundS32(Rect.Min.y), BackBuffer->Height);
    s32 XMax = Clamp(0, RoundS32(Rect.Max.x), BackBuffer->Width);
    s32 YMax = Clamp(0, RoundS32(Rect.Max.y), BackBuffer->Height);

    Color *= 255.0f;
    u32 DestColor = ((RoundU32(Color.r) << 16) |
                     (RoundU32(Color.g) << 8) |
                     (RoundU32(Color.b) << 0));

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
RenderRectangle(game_backbuffer *BackBuffer, rectangle2 Rect, v4 Color)
{
    if(Color.a == 1.0f)
    {
        RenderSolidRectangle(BackBuffer, Rect, Color.rgb);
    }
    else if(IsInvertedColor(Color))
    {
        RenderInvertedRectangle(BackBuffer, Rect);
    }
    else
    {
        RenderTranslucentRectangle(BackBuffer, Rect, Color);
    }
}

internal void
RenderLine(game_backbuffer *BackBuffer, v2 PointA, v2 PointB, v4 Color)
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
        Min = Clamp(0, RoundS32(Start.x), BackBuffer->Width);
        Max = Clamp(0, RoundS32(End.x), BackBuffer->Width);
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
        Min = Clamp(0, RoundS32(Start.y), BackBuffer->Height);
        Max = Clamp(0, RoundS32(End.y), BackBuffer->Height);
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
Clear(game_backbuffer *BackBuffer, v4 Color)
{
    rectangle2 Rect;
    Rect.Min = V2(0, 0);
    Rect.Max = V2i(BackBuffer->Width, BackBuffer->Height);
    RenderRectangle(BackBuffer, Rect, Color);
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
RenderTree(render_buffer *RenderBuffer, binary_node *Node, game_backbuffer *BackBuffer)
{
    if(Node->Prev)
    {
        RenderTree(RenderBuffer, Node->Prev, BackBuffer);
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
                RenderBitmap(BackBuffer, Data->Bitmap, Data->Origin.xy, Data->XAxis, Data->YAxis,
                             Data->Scale, Data->Color);
            } break;

            case RenderCommand_Rectangle:
            {
                render_rectangle_data *Data = (render_rectangle_data *)(Header + 1);
                RenderRectangle(BackBuffer, Data->Rect, Data->Color);
            } break;
            
            case RenderCommand_Line:
            {
                render_line_data *Data = (render_line_data *)(Header + 1);
                RenderLine(BackBuffer, Data->PointA.xy, Data->PointB.xy, Data->Color);
            } break;
            
            case RenderCommand_Clear:
            {
                render_clear_data *Data = (render_clear_data *)(Header + 1);
                Clear(BackBuffer, Data->Color);
            } break;

            InvalidDefaultCase;
        }
    }
    if(Node->Next)
    {
        RenderTree(RenderBuffer, Node->Next, BackBuffer);
    }
}

// TODO(chris): Sorting!!!
internal void
RenderBufferToBackBuffer(render_buffer *RenderBuffer, game_backbuffer *BackBuffer)
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
        RenderTree(RenderBuffer, SortTree, BackBuffer);
    }
}
