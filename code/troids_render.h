#if !defined(TROIDS_RENDER_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

enum render_command
{
    RenderCommand_Bitmap,
    RenderCommand_Rectangle,
    RenderCommand_Line,
    RenderCommand_Clear,
};

struct render_command_header
{
    render_command Command;
};

struct render_bitmap_data
{
    r32 SortKey;
    r32 Scale;
    v2 XAxis;
    v2 YAxis;
    v4 Color;
    v3 Origin;
    loaded_bitmap *Bitmap;
};

struct render_rectangle_data
{
    r32 SortKey;
    rectangle2 Rect;
    v4 Color;
};

struct render_line_data
{
    r32 SortKey;
    v3 PointA;
    v3 PointB;
    v4 Color;
};

struct render_clear_data
{
    v4 Color;
};

struct render_buffer
{
    memory_arena Arena;
};

#define INVERTED_COLOR V4(0, 0, 0, -REAL32_MAX)
inline b32
IsInvertedColor(v4 Color)
{
    b32 Result = (Color.a == -REAL32_MAX);
    return(Result);
}

inline void
PushBitmap(render_buffer *RenderBuffer, loaded_bitmap *Bitmap, v3 Origin, v2 XAxis, v2 YAxis,
           r32 Scale, v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f));

inline rectangle2
PushRectangle(render_buffer *RenderBuffer, v3 P, v2 Dim, v4 Color);

inline void
PushLine(render_buffer *RenderBuffer, v3 PointA, v3 PointB, v4 Color);

inline void
PushClear(render_buffer *RenderBuffer, v4 Color);

struct text_layout
{
    loaded_font *Font;
    v2 P;
    r32 Scale;
    v4 Color;
};

internal rectangle2
DrawText(render_buffer *RenderBuffer, text_layout *Layout, u32 TextLength, char *Text);

internal rectangle2
DrawButton(render_buffer *RenderBuffer, text_layout *Layout, u32 TextLength, char *Text);

internal void
RenderBufferToBackBuffer(render_buffer *RenderBuffer, game_backbuffer *BackBuffer);

#define TROIDS_RENDER_H
#endif
