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
    Projection_Perspective,
};

struct render_buffer
{
    u32 Width;
    u32 Height;
    r32 MetersToPixels;
    v3 CameraP;
#if TROIDS_INTERNAL
    v3 ClipCameraP;
#endif
    r32 CameraRot;
    projection Projection;
    projection DefaultProjection;
    memory_arena Arena;
};

inline v2
Project(render_buffer *RenderBuffer, v3 WorldP, v3 CameraP = {NAN})
{
    v2 Result = {};
    if(IsNaN(CameraP.x))
    {
        CameraP = RenderBuffer->CameraP;
    }
    
    switch(RenderBuffer->Projection)
    {
        case Projection_Perspective:
        {
            v3 NewP = WorldP - CameraP;
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
Unproject(render_buffer *RenderBuffer, v2 ScreenP, r32 Z, v3 CameraP = {NAN})
{
    if(IsNaN(CameraP.x))
    {
        CameraP = RenderBuffer->CameraP;
    }
    v3 Result = {};
    switch(RenderBuffer->Projection)
    {
        case Projection_Perspective:
        {
            r32 DistanceFromCamera = Z - CameraP.z;
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

            Result = WorldP + CameraP;
        } break;

        case Projection_None:
        {
            Result = V3(ScreenP, 0.0f);
        } break;

        InvalidDefaultCase;
    }
    return(Result);
}

struct project_triangle_result
{
    b32 Clipped;
    v2 A;
    v2 B;
    v2 C;
};

inline project_triangle_result
ProjectTriangle(render_buffer *RenderBuffer, v3 A, v3 B, v3 C)
{
    project_triangle_result Result;
    Result.Clipped = true;
#if DEBUG_CAMERA
    Result.A = Project(RenderBuffer, A, RenderBuffer->ClipCameraP);
    Result.B = Project(RenderBuffer, B, RenderBuffer->ClipCameraP);
    Result.C = Project(RenderBuffer, C, RenderBuffer->ClipCameraP);
#else
    Result.A = Project(RenderBuffer, A);
    Result.B = Project(RenderBuffer, B);
    Result.C = Project(RenderBuffer, C);
#endif

    rectangle2 ClipRect = MinMax(V2i(0, 0), V2i(RenderBuffer->Width, RenderBuffer->Height));

    if(Inside(ClipRect, Result.A) || Inside(ClipRect, Result.B) || Inside(ClipRect, Result.C))
    {
        Result.Clipped = false;
    }
    else
    {
        v2 AB = Result.B-Result.A;
        v2 BC = Result.C-Result.B;
        v2 CA = Result.A-Result.C;
        v2 Horiz = V2i(RenderBuffer->Width, 0);
        v2 Vert = V2i(0, RenderBuffer->Height);
        v2 Origin = V2i(0, 0);

        r32 ABCrossHoriz = Cross(AB, Horiz);
        r32 BCCrossHoriz = Cross(BC, Horiz);
        r32 CACrossHoriz = Cross(CA, Horiz);
        r32 ABCrossVert = Cross(AB, Vert);
        r32 BCCrossVert = Cross(BC, Vert);
        r32 CACrossVert = Cross(CA, Vert);

        if(ABCrossHoriz != 0.0f)
        {
            r32 Inv = 1.0f / ABCrossHoriz;
            r32 U1 = Cross(Origin-Result.A, AB)*Inv;
            r32 T1 = Cross(Origin-Result.A, Horiz)*Inv;
            r32 U2 = Cross(Vert-Result.A, AB)*Inv;
            r32 T2 = Cross(Vert-Result.A, Horiz)*Inv;
            if((0.0f <= U1 && U1 <= 1.0f && 0.0f <= T1 && T1 <= 1.0f) ||
               (0.0f <= U2 && U2 <= 1.0f && 0.0f <= T2 && T2 <= 1.0f))
            {
                Result.Clipped = false;
            }
        }
        if(BCCrossHoriz != 0.0f)
        {
            r32 Inv = 1.0f / BCCrossHoriz;
            r32 U1 = Cross(Origin-Result.B, BC)*Inv;
            r32 T1 = Cross(Origin-Result.B, Horiz)*Inv;
            r32 U2 = Cross(Vert-Result.B, BC)*Inv;
            r32 T2 = Cross(Vert-Result.B, Horiz)*Inv;
            if((0.0f <= U1 && U1 <= 1.0f && 0.0f <= T1 && T1 <= 1.0f) ||
               (0.0f <= U2 && U2 <= 1.0f && 0.0f <= T2 && T2 <= 1.0f))
            {
                Result.Clipped = false;
            }
        }
        if(CACrossHoriz != 0.0f)
        {
            r32 Inv = 1.0f / CACrossHoriz;
            r32 U1 = Cross(Origin-Result.C, CA)*Inv;
            r32 T1 = Cross(Origin-Result.C, Horiz)*Inv;
            r32 U2 = Cross(Vert-Result.C, CA)*Inv;
            r32 T2 = Cross(Vert-Result.C, Horiz)*Inv;
            if((0.0f <= U1 && U1 <= 1.0f && 0.0f <= T1 && T1 <= 1.0f) ||
               (0.0f <= U2 && U2 <= 1.0f && 0.0f <= T2 && T2 <= 1.0f))
            {
                Result.Clipped = false;
            }
        }
        if(ABCrossVert != 0.0f)
        {
            r32 Inv = 1.0f / ABCrossVert;
            r32 U1 = Cross(Origin-Result.A, AB)*Inv;
            r32 T1 = Cross(Origin-Result.A, Vert)*Inv;
            r32 U2 = Cross(Horiz-Result.A, AB)*Inv;
            r32 T2 = Cross(Horiz-Result.A, Vert)*Inv;
            if((0.0f <= U1 && U1 <= 1.0f && 0.0f <= T1 && T1 <= 1.0f) ||
               (0.0f <= U2 && U2 <= 1.0f && 0.0f <= T2 && T2 <= 1.0f))
            {
                Result.Clipped = false;
            }
        }
        if(BCCrossVert != 0.0f)
        {
            r32 Inv = 1.0f / BCCrossVert;
            r32 U1 = Cross(Origin-Result.B, BC)*Inv;
            r32 T1 = Cross(Origin-Result.B, Vert)*Inv;
            r32 U2 = Cross(Horiz-Result.B, BC)*Inv;
            r32 T2 = Cross(Horiz-Result.B, Vert)*Inv;
            if((0.0f <= U1 && U1 <= 1.0f && 0.0f <= T1 && T1 <= 1.0f) ||
               (0.0f <= U2 && U2 <= 1.0f && 0.0f <= T2 && T2 <= 1.0f))
            {
                Result.Clipped = false;
            }
        }
        if(CACrossVert != 0.0f)
        {
            r32 Inv = 1.0f / CACrossVert;
            r32 U1 = Cross(Origin-Result.C, CA)*Inv;
            r32 T1 = Cross(Origin-Result.C, Vert)*Inv;
            r32 U2 = Cross(Horiz-Result.C, CA)*Inv;
            r32 T2 = Cross(Horiz-Result.C, Vert)*Inv;
            if((0.0f <= U1 && U1 <= 1.0f && 0.0f <= T1 && T1 <= 1.0f) ||
               (0.0f <= U2 && U2 <= 1.0f && 0.0f <= T2 && T2 <= 1.0f))
            {
                Result.Clipped = false;
            }
        }
    }

#if DEBUG_CAMERA
    Result.A = Project(RenderBuffer, A);
    Result.B = Project(RenderBuffer, B);
    Result.C = Project(RenderBuffer, C);
#endif

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

    v2 Min = Project(RenderBuffer, P);
    v2 Max = Project(RenderBuffer, P + V3(Dim, 0));
    rectangle2 Rect = MinMax(Min, Max);
    Data->Rect = Rect;
    Data->Color = Color;
    Data->SortKey = P.z;
    return(Rect);
}

inline void
PushBorder(render_buffer *RenderBuffer, v3 P, v2 Dim, v4 Color)
{
    v3 TopLeft = P;
    TopLeft.y += Dim.y;
    v3 BottomRight = P;
    BottomRight.x += Dim.x;
    PushRectangle(RenderBuffer, P, V2(Dim.x, 1), Color);
    PushRectangle(RenderBuffer, TopLeft, V2(Dim.x, 1), Color);
    PushRectangle(RenderBuffer, P, V2(1, Dim.y), Color);
    PushRectangle(RenderBuffer, BottomRight, V2(1, Dim.y), Color);
}

inline void
PushTriangle(render_buffer *RenderBuffer, v3 A, v3 B, v3 C, v4 Color)
{
    project_triangle_result Projection = ProjectTriangle(RenderBuffer, A, B, C);
    if(!Projection.Clipped)
    {
        PushRenderHeader(Data, &RenderBuffer->Arena, triangle);

        Data->A = Projection.A;
        Data->B = Projection.B;
        Data->C = Projection.C;
        Data->Color = Color;
        // TODO(chris): How to handle sorting with 3D triangles?
        Data->SortKey = A.z;
    }
    else
    {
        int A = 0;
    }
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
