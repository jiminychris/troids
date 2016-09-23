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
    RenderCommand_bitmap,
    RenderCommand_triangle,
    RenderCommand_rectangle,
    RenderCommand_circle,
    RenderCommand_line,
    RenderCommand_clear,
};

struct render_command_header
{
    render_command Command;
};

struct render_bitmap_data
{
    r32 SortKey;
    v2 Origin;
    v2 XAxis;
    v2 YAxis;
    v4 Color;
    loaded_bitmap *Bitmap;
};

struct render_triangle_data
{
    r32 SortKey;
    v2 A;
    v2 B;
    v2 C;
    v4 Color;
};

struct render_rectangle_data
{
    r32 SortKey;
    rectangle2 Rect;
    v4 Color;
};

struct render_circle_data
{
    r32 SortKey;
    r32 Radius;
    v2 P;
    v4 Color;
};

struct render_line_data
{
    r32 SortKey;
    v2 A;
    v2 B;
    v4 Color;
};

struct render_clear_data
{
    v4 Color;
};

enum projection
{
    Projection_None,
    Projection_Orthographic,
};

struct render_buffer
{
    u32 Width;
    u32 Height;
    r32 MetersToPixels;
    v3 CameraP;
    r32 CameraRot;
    projection Projection;
    projection DefaultProjection;
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
           v2 Dim, v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f));

inline void
PushRotatedRectangle(render_buffer *RenderBuffer, v3 Origin,
                     v2 Dim, v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f));

inline rectangle2
PushRectangle(render_buffer *RenderBuffer, v3 P, v2 Dim, v4 Color);

inline void
PushLine(render_buffer *RenderBuffer, v3 Origin, m33 RotationMatrix,
         v3 PointA, v3 PointB, v2 Dim, v4 Color);

inline void
PushLine(render_buffer *RenderBuffer, v3 PointA, v3 PointB, v4 Color);

inline void
PushClear(render_buffer *RenderBuffer, v4 Color);

struct text_layout
{
    r32 Scale;
    b32 DropShadow;
    v2 P;
    v4 Color;
    loaded_font *Font;
};

struct text_measurement
{
    r32 MinX;
    r32 MaxX;
    r32 LineMaxY;
    r32 TextMaxY;
    r32 BaseLine;
    r32 TextMinY;
    r32 LineMinY;
};

inline rectangle2
GetLineRect(text_measurement TextMeasurement)
{
    rectangle2 Result = MinMax(V2(TextMeasurement.MinX, TextMeasurement.LineMinY),
                               V2(TextMeasurement.MaxX, TextMeasurement.LineMaxY));
    return(Result);
}

inline v2
GetTightCenteredOffset(text_measurement TextMeasurement)
{
    v2 AlignP = V2(TextMeasurement.MinX, TextMeasurement.BaseLine);
    v2 Center = 0.5f*V2(TextMeasurement.MinX + TextMeasurement.MaxX,
                        TextMeasurement.TextMinY + TextMeasurement.TextMaxY);
    v2 Result = AlignP - Center;
    return(Result);
}

internal text_measurement
DrawText(render_buffer *RenderBuffer, text_layout *Layout, u32 TextLength, char *Text, u32 Flags = 0);

internal rectangle2
DrawButton(render_buffer *RenderBuffer, text_layout *Layout, u32 TextLength, char *Text);

internal void
RenderBufferToBackBuffer(render_buffer *RenderBuffer, loaded_bitmap *BackBuffer);

#define TROIDS_RENDER_H
#endif
