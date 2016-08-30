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

inline void
PushRectangle(render_buffer *RenderBuffer, v3 P, v2 Dim, v4 Color)
{
    PushRenderHeader(&RenderBuffer->Arena, RenderCommand_Rectangle);
    rectangle2 Rect = MinDim(P.xy, Dim);
    render_rectangle_data *Data = PushStruct(&RenderBuffer->Arena, render_rectangle_data);
    Data->Rect = Rect;
    Data->Color = Color;
    Data->SortKey = P.z;
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

// TODO(chris): Clean this up. Make fonts and font layout more systematic.
internal v2
DrawText(render_buffer *RenderBuffer, text_layout *Layout, u32 TextLength, char *Text)
{
    v2 Result;
    r32 XInit = Layout->P.x;
    char Prev = 0;
    char *At = Text;
    for(u32 AtIndex = 0;
        AtIndex < TextLength;
        ++AtIndex)
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
            PushBitmap(RenderBuffer, Glyph, V3(Layout->P, TEXT_Z-1),
                       XAxis, YAxis, Layout->Scale*1.1f, V4(0, 0, 0, 1));
            PushBitmap(RenderBuffer, Glyph, V3(Layout->P, TEXT_Z), XAxis, YAxis, Layout->Scale);
        }
        Prev = *At++;
        Layout->P.x += Layout->Scale*GetTextAdvance(Layout->Font, Prev, *At);
    }
    Result = V2(Layout->P.x - XInit, Layout->Scale*Layout->Font->Height);
    Layout->P.x = XInit;
    Layout->P.y -= Layout->Font->LineAdvance*Layout->Scale;
    
    return(Result);
}

internal rectangle2
DrawButton(render_buffer *RenderBuffer, text_layout *Layout, u32 TextLength, char *Text)
{
    rectangle2 Result;
    v2 PInit = Layout->P;
    v2 ButtonDim = DrawText(RenderBuffer, Layout, TextLength, Text);
    Result = TopLeftDim(V2(PInit.x, PInit.y + Layout->Font->Ascent*Layout->Scale), ButtonDim);
    PushRectangle(RenderBuffer, V3(Result.Min, HUD_Z), ButtonDim, V4(1, 0, 1, 1));
    
    return(Result);
}

#pragma optimize("gts", on)
internal void
RenderBitmap(game_backbuffer *BackBuffer, loaded_bitmap *Bitmap, v2 Origin, v2 XAxis, v2 YAxis,
             r32 Scale, v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    TIMED_BLOCK(RenderBitmap);
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
    s32 YMax = Clamp(0, Ceiling(Maximum(Maximum(Origin.y, Origin.y + XAxis.y),
                                        Maximum(Origin.y + YAxis.y, Origin.y + XAxis.y + YAxis.y))),
                     BackBuffer->Height);

    r32 Inv255 = 1.0f / 255.0f;
    r32 InvXAxisLengthSq = 1.0f / LengthSq(XAxis);
    r32 InvYAxisLengthSq = 1.0f / LengthSq(YAxis);
    u8 *PixelRow = (u8 *)BackBuffer->Memory + (BackBuffer->Pitch*YMin);
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Pixel = (u32 *)PixelRow + XMin;
        for(s32 X = XMin;
            X < XMax;
            X += 1)
        {
            // NOTE(chris): Test the next four pixels and mask the ones we're using
            v2 TestPoint = V2i(X, Y) - Origin;

            r32 U = Inner(TestPoint, XAxis)*InvXAxisLengthSq;
            r32 V = Inner(TestPoint, YAxis)*InvYAxisLengthSq;

            if(U >= 0.0f &&
               U <= 1.0f &&
               V >= 0.0f &&
               V <= 1.0f)
            {
                r32 tX = U*(Bitmap->Width - 2) + 0.5f;
                r32 tY = V*(Bitmap->Height - 2) + 0.5f;

                u32 *TexelAPtr = (u32 *)Bitmap->Memory + (u32)tY*Bitmap->Width + (u32)tX;
                u32 *TexelBPtr = TexelAPtr + 1;
                u32 *TexelCPtr = TexelAPtr + Bitmap->Width;
                u32 *TexelDPtr = TexelCPtr + 1;
                v4 TexelA = V4i((*TexelAPtr >> 16) & 0xFF,
                               (*TexelAPtr >> 8) & 0xFF,
                               (*TexelAPtr >> 0) & 0xFF,
                               (*TexelAPtr >> 24) & 0xFF);
                v4 TexelB = V4i((*TexelBPtr >> 16) & 0xFF,
                               (*TexelBPtr >> 8) & 0xFF,
                               (*TexelBPtr >> 0) & 0xFF,
                               (*TexelBPtr >> 24) & 0xFF);
                v4 TexelC = V4i((*TexelCPtr >> 16) & 0xFF,
                               (*TexelCPtr >> 8) & 0xFF,
                               (*TexelCPtr >> 0) & 0xFF,
                               (*TexelCPtr >> 24) & 0xFF);
                v4 TexelD = V4i((*TexelDPtr >> 16) & 0xFF,
                               (*TexelDPtr >> 8) & 0xFF,
                               (*TexelDPtr >> 0) & 0xFF,
                               (*TexelDPtr >> 24) & 0xFF);

                r32 tU = tX - (u32)tX;
                r32 tV = tY - (u32)tY;
                v4 Texel = Lerp(Lerp(TexelA, tU, TexelB), tV, Lerp(TexelC, tU, TexelD));

                r32 DA = 1.0f - (Inv255*Texel.a*Color.a);
                u32 DR = (*Pixel >> 16) & 0xFF;
                u32 DG = (*Pixel >> 8) & 0xFF;
                u32 DB = (*Pixel >> 0) & 0xFF;
                
                *Pixel = (((RoundU32(Color.r*Texel.r + DA*DR)) << 16) |
                          ((RoundU32(Color.g*Texel.g + DA*DG)) << 8) |
                          ((RoundU32(Color.b*Texel.b + DA*DB)) << 0));
            }
            Pixel += 1;
        }
        PixelRow += BackBuffer->Pitch;
    }
}

internal void
RenderRectangle(game_backbuffer *BackBuffer, rectangle2 Rect, v4 Color)
{
    TIMED_BLOCK(RenderRectangle);
    s32 XMin = Clamp(0, RoundS32(Rect.Min.x), BackBuffer->Width);
    s32 YMin = Clamp(0, RoundS32(Rect.Min.y), BackBuffer->Height);
    s32 XMax = Clamp(0, RoundS32(Rect.Max.x), BackBuffer->Width);
    s32 YMax = Clamp(0, RoundS32(Rect.Max.y), BackBuffer->Height);

    Color.rgb *= 255.0f;

    if(Color.a == 1.0f)
    {
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
    else
    {
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
