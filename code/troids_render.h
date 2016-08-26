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

struct render_bitmap_data
{
    loaded_bitmap *Bitmap;
    v2 Origin;
    v2 XAxis;
    v2 YAxis;
    r32 Scale;
    v4 Color;
};

struct render_rectangle_data
{
    rectangle2 Rect;
    v4 Color;
};

struct render_line_data
{
    v2 PointA;
    v2 PointB;
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

inline void
PushBitmap(render_buffer *RenderBuffer, loaded_bitmap *Bitmap, v2 Origin, v2 XAxis, v2 YAxis,
           r32 Scale, v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f));

inline void
PushRectangle(render_buffer *RenderBuffer, rectangle2 Rect, v4 Color);

inline void
PushLine(render_buffer *RenderBuffer, v2 PointA, v2 PointB, v4 Color);

inline void
PushClear(render_buffer *RenderBuffer, v4 Color);

struct text_layout
{
    loaded_font *Font;
    v2 P;
    r32 Scale;
    v4 Color;
};

internal void
PushText(render_buffer *RenderBuffer, text_layout *Layout, u32 TextLength, char *Text);

internal void
RenderBufferToBackBuffer(render_buffer *RenderBuffer, game_backbuffer *BackBuffer);

#define TROIDS_RENDER_H
#endif
