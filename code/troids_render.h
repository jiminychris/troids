#if !defined(TROIDS_RENDER_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

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

inline v2
Project(render_buffer *RenderBuffer, v3 WorldP)
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

inline v3
Unproject(render_buffer *RenderBuffer, v2 ScreenP, r32 Z)
{
    v3 Result = {};
    switch(RenderBuffer->Projection)
    {
        case Projection_Orthographic:
        {
            r32 DistanceFromCamera = Z - RenderBuffer->CameraP.z;
            r32 ScaleFactor = -(5.0f*DistanceFromCamera);
            r32 PixelsToMeters = 1.0f / RenderBuffer->MetersToPixels;
            v3 WorldP = V3((ScreenP - 0.5f*V2i(RenderBuffer->Width,
                                            RenderBuffer->Height))
                           *ScaleFactor*PixelsToMeters, DistanceFromCamera);

            r32 CosRot = Cos(-RenderBuffer->CameraRot);
            r32 SinRot = Sin(-RenderBuffer->CameraRot);
            {
                m33 RotMat =
                    {
                        CosRot, SinRot, 0,
                        -SinRot, CosRot, 0,
                        0, 0, 1,
                    };
                WorldP = RotMat*WorldP;
            }

            Result = WorldP + RenderBuffer->CameraP;
        } break;

        case Projection_None:
        {
            Result = V3(ScreenP, 0.0f);
        } break;

        InvalidDefaultCase;
    }
    return(Result);
}

enum render_command
{
    RenderCommand_bitmap,
    RenderCommand_aligned_rectangle,
    RenderCommand_triangle,
    RenderCommand_clear,
#if TROIDS_INTERNAL
    RenderCommand_DEBUGrectangle,
    RenderCommand_DEBUGtriangle,
    RenderCommand_DEBUGcircle,
    RenderCommand_DEBUGline,
#endif
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

struct render_aligned_rectangle_data
{
    r32 SortKey;
    rectangle2 Rect;
    v4 Color;
};

struct render_triangle_data
{
    r32 SortKey;
    v2 A;
    v2 B;
    v2 C;
    v4 Color;
};

struct render_clear_data
{
    v3 Color;
};

#define PushRenderHeader(Name, Arena, CommandInit)                      \
    PushStruct((Arena), render_command_header)->Command = RenderCommand_##CommandInit; \
    render_##CommandInit##_data *Name = PushStruct((Arena), render_##CommandInit##_data);

inline void
PushBitmap(render_buffer *RenderBuffer, loaded_bitmap *Bitmap, v3 P, v2 XAxis, v2 YAxis,
           v2 Dim, v4 Color = V4(1, 1, 1, 1))
{
    if(Bitmap && Bitmap->Height && Bitmap->Width)
    {
        PushRenderHeader(Data, &RenderBuffer->Arena, bitmap);

        XAxis*=Dim.x;
        YAxis*=Dim.y;
        v2 Align = Bitmap->Align;
        v3 Origin = P - V3(Hadamard(Align, XAxis + YAxis), 0);
                
        XAxis = Project(RenderBuffer, Origin + V3(XAxis, 0));
        YAxis = Project(RenderBuffer, Origin + V3(YAxis, 0));
        v2 ScreenOrigin = Project(RenderBuffer, Origin);
        XAxis -= ScreenOrigin;
        YAxis -= ScreenOrigin;
        
        Data->Bitmap = Bitmap;
        Data->Origin = ScreenOrigin;
        Data->XAxis = XAxis;
        Data->YAxis = YAxis;
        Data->Color = Color;
        Data->SortKey = P.z;
    }
}

inline rectangle2
PushRectangle(render_buffer *RenderBuffer, v3 P, v2 Dim, v4 Color)
{
    PushRenderHeader(Data, &RenderBuffer->Arena, aligned_rectangle);

    rectangle2 Rect = MinDim(P.xy, Dim);
    Data->Rect = Rect;
    Data->Color = Color;
    Data->SortKey = P.z;
    return(Rect);
}

inline void
PushTriangle(render_buffer *RenderBuffer, v3 A, v3 B, v3 C, v4 Color)
{
    PushRenderHeader(Data, &RenderBuffer->Arena, triangle);

    Data->A = Project(RenderBuffer, A);
    Data->B = Project(RenderBuffer, B);
    Data->C = Project(RenderBuffer, C);
    Data->Color = Color;
    // TODO(chris): How to handle sorting with 3D triangles?
    Data->SortKey = A.z;
}

inline void
PushClear(render_buffer *RenderBuffer, v3 Color)
{
    PushRenderHeader(Data, &RenderBuffer->Arena, clear);

    Data->Color = Color;
}

#if TROIDS_INTERNAL
struct render_DEBUGrectangle_data
{
    r32 SortKey;
    v2 Origin;
    v2 XAxis;
    v2 YAxis;
    v4 Color;
};

struct render_DEBUGtriangle_data
{
    r32 SortKey;
    v2 A;
    v2 B;
    v2 C;
    v4 Color;
};

struct render_DEBUGcircle_data
{
    r32 SortKey;
    r32 Radius;
    v2 P;
    v4 Color;
};

struct render_DEBUGline_data
{
    r32 SortKey;
    v2 A;
    v2 B;
    v4 Color;
};


inline void
DEBUGPushRectangle(render_buffer *RenderBuffer, v3 P, v2 XAxis, v2 YAxis, v2 Dim, v4 Color,
                   v2 Align = V2(0.5f, 0.5f))
{
    PushRenderHeader(Data, &RenderBuffer->Arena, DEBUGrectangle);

    XAxis*=Dim.x;
    YAxis*=Dim.y;
    v3 Origin = P - V3(Hadamard(Align, XAxis + YAxis), 0);
                
    XAxis = Project(RenderBuffer, Origin + V3(XAxis, 0));
    YAxis = Project(RenderBuffer, Origin + V3(YAxis, 0));
    v2 ScreenOrigin = Project(RenderBuffer, Origin);
    XAxis -= ScreenOrigin;
    YAxis -= ScreenOrigin;
    
    Data->Origin = ScreenOrigin;
    Data->XAxis = XAxis;
    Data->YAxis = YAxis;
    Data->Color = Color;
    Data->SortKey = Origin.z;
}

inline void
DEBUGPushTriangle(render_buffer *RenderBuffer, v3 A, v3 B, v3 C, v4 Color)
{
    PushRenderHeader(Data, &RenderBuffer->Arena, DEBUGtriangle);

    Data->A = Project(RenderBuffer, A);
    Data->B = Project(RenderBuffer, B);
    Data->C = Project(RenderBuffer, C);
    Data->Color = Color;
    // TODO(chris): How to handle sorting with 3D triangles?
    Data->SortKey = A.z;
}

inline void
DEBUGPushCircle(render_buffer *RenderBuffer, v3 P, r32 Radius, v4 Color)
{
    PushRenderHeader(Data, &RenderBuffer->Arena, DEBUGcircle);

    Data->P = Project(RenderBuffer, P);
    Data->Radius = Length(Project(RenderBuffer, P + V3(Radius, 0, 0)) - Data->P);
    Data->Color = Color;
    Data->SortKey = P.z;
}

// TODO(chris): How do I sort lines that move through z? This goes for bitmaps and rectangles also.
inline void
DEBUGPushLine(render_buffer *RenderBuffer, v3 A, v3 B, v4 Color)
{
    PushRenderHeader(Data, &RenderBuffer->Arena, DEBUGline);

    Data->A = Project(RenderBuffer, A);
    Data->B = Project(RenderBuffer, B);
    Data->Color = Color;
    Data->SortKey = 0.5f*(A.z + B.z);
}

inline void
DEBUGPushLine(render_buffer *RenderBuffer, v3 Origin, m33 RotationMatrix,
              v3 PointA, v3 PointB, v2 Dim, v4 Color)
{
    DEBUGPushLine(RenderBuffer,
                  Origin + Hadamard(V3(Dim, 0), RotationMatrix*PointA),
                  Origin + Hadamard(V3(Dim, 0), RotationMatrix*PointB),
                  Color);
}
#endif

#define INVERTED_COLOR V4(0, 0, 0, -REAL32_MAX)
inline b32
IsInvertedColor(v4 Color)
{
    b32 Result = (Color.a == -REAL32_MAX);
    return(Result);
}

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

internal void
RenderBufferToBackBuffer(render_buffer *RenderBuffer, loaded_bitmap *BackBuffer);

#define TROIDS_RENDER_H
#endif
