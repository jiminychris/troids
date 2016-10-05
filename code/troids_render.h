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

enum render_flags
{
    RenderFlags_Identity = 0,
    RenderFlags_UsePipeline = 1<<0,
};

struct render_buffer
{
    u32 Width;
    u32 Height;
    r32 MetersToPixels;
    r32 NearZ;
    v3 CameraP;
    r32 CameraRot;
    projection Projection;
    projection DefaultProjection;
    memory_arena Arena;
#if TROIDS_INTERNAL
    v3 ClipCameraP;
    rectangle2 ClipRect;
#endif
};

inline v3
Project(render_buffer *RenderBuffer, v3 WorldP, v3 CameraP = {NAN})
{
    v3 Result = {};
    if(IsNaN(CameraP.x))
    {
        CameraP = RenderBuffer->CameraP;
    }
    
    switch(RenderBuffer->Projection)
    {
        case Projection_Perspective:
        {
            r32 AspectRatio = (r32)RenderBuffer->Width / (r32)RenderBuffer->Height;
            r32 Z = WorldP.z - CameraP.z;
            Assert(Z < 0.0f);
            Result.x = (1-WorldP.x/Z)*0.5f*RenderBuffer->Width;
            Result.y = (1-AspectRatio*WorldP.y/Z)*0.5f*RenderBuffer->Height;
            Result.z = WorldP.z;
#if 0
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
#endif
        } break;

        case Projection_None:
        {
            Result = WorldP;
        } break;

        InvalidDefaultCase;
    }
    return(Result);
}

inline v3
Unproject(render_buffer *RenderBuffer, v3 ScreenP, v3 CameraP = {NAN})
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
            r32 InvAspectRatio = (r32)RenderBuffer->Height / (r32)RenderBuffer->Width;
            r32 Z = ScreenP.z - CameraP.z;
            Result.z = ScreenP.z;
            Result.x = (1.0f - ScreenP.x*2.0f/(r32)RenderBuffer->Width)*Z;
            Result.y = (1.0f - ScreenP.y*2.0f/(r32)RenderBuffer->Height)*InvAspectRatio*Z;
#if 0
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
#endif

        } break;

        case Projection_None:
        {
            Result = ScreenP;
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
                
        XAxis = Project(RenderBuffer, Origin + V3(XAxis, 0)).xy;
        YAxis = Project(RenderBuffer, Origin + V3(YAxis, 0)).xy;
        v2 ScreenOrigin = Project(RenderBuffer, Origin).xy;
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

    v2 Min = Project(RenderBuffer, P).xy;
    v2 Max = Project(RenderBuffer, P + V3(Dim, 0)).xy;
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

// NOTE(chris): A polygon clipped against a plane can result in at most one extra vertex
// So a triangle clipped against 6 planes can give 3+6=9 total vertices
#define MAX_CLIP_VERTICES 16
struct clipped_polygon
{
    s32 VertexCount;
    r32 X[MAX_CLIP_VERTICES];
    r32 Y[MAX_CLIP_VERTICES];
    r32 Z[MAX_CLIP_VERTICES];
};

enum clip_dimension
{
    ClipDimension_X,
    ClipDimension_Y,
    ClipDimension_Z,
};

enum clip_direction
{
    ClipDirection_LessThan,
    ClipDirection_GreaterThan,
};

internal void
Clip(clipped_polygon *Polygon, r32 Plane, clip_dimension Dimension, clip_direction Direction)
{
    TIMED_FUNCTION();
    if(Polygon->VertexCount > 0)
    {
        r32 Multiplier = 1.0f;
        if(Direction == ClipDirection_GreaterThan)
        {
            Multiplier = -1.0f;
        }
        r32 *Scalars;
        switch(Dimension)
        {
            case ClipDimension_X:
            {
                Scalars = Polygon->X;
            } break;
        
            case ClipDimension_Y:
            {
                Scalars = Polygon->Y;
            } break;
        
            default:
            {
                Scalars = Polygon->Z;
            } break;
        }
        Polygon->X[Polygon->VertexCount] = Polygon->X[0];
        Polygon->Y[Polygon->VertexCount] = Polygon->Y[0];
        Polygon->Z[Polygon->VertexCount] = Polygon->Z[0];
    
        s32 Out = MAX_CLIP_VERTICES;
        s32 In = MAX_CLIP_VERTICES;
        b32 AllPointsInside = true;
        for(s32 VertexIndex = 0;
            VertexIndex < Polygon->VertexCount;
            ++VertexIndex)
        {
            r32 A = Multiplier*Scalars[VertexIndex];
            r32 B = Multiplier*Scalars[VertexIndex+1];
            r32 P = Multiplier*Plane;
            if(A < P && B >= P)
            {
                Out = VertexIndex;
            }
            else if(A >= P && B < P)
            {
                In = VertexIndex;
            }
            AllPointsInside &= (A <= P);
        }
        if(!AllPointsInside)
        {
            if(Out == MAX_CLIP_VERTICES)
            {
                // NOTE(chris): Nothing crossed in or out, so all vertices lie outside
                Polygon->VertexCount = 0;
            }
            else
            {
                r32 A = Scalars[Out];
                r32 B = Scalars[Out+1];
                r32 tOut = (Plane - A)/(B-A);
                r32 InvtOut = 1.0f - tOut;
                r32 NewOutX = InvtOut*Polygon->X[Out] + tOut*Polygon->X[Out+1];
                r32 NewOutY = InvtOut*Polygon->Y[Out] + tOut*Polygon->Y[Out+1];
                r32 NewOutZ = InvtOut*Polygon->Z[Out] + tOut*Polygon->Z[Out+1];

                r32 C = Scalars[In];
                r32 D = Scalars[In+1];
                r32 tIn = (Plane - C)/(D-C);
                r32 InvtIn = 1.0f - tIn;
                r32 NewInX = InvtIn*Polygon->X[In] + tIn*Polygon->X[In+1];
                r32 NewInY = InvtIn*Polygon->Y[In] + tIn*Polygon->Y[In+1];
                r32 NewInZ = InvtIn*Polygon->Z[In] + tIn*Polygon->Z[In+1];

                // TODO(chris): Make sure this actually copies everything.
                clipped_polygon Buffer = *Polygon;
                if(Out < In)
                {
                    Polygon->VertexCount = Out + 1;
                    Polygon->X[Polygon->VertexCount] = NewOutX;
                    Polygon->Y[Polygon->VertexCount] = NewOutY;
                    Polygon->Z[Polygon->VertexCount++] = NewOutZ;
                    Polygon->X[Polygon->VertexCount] = NewInX;
                    Polygon->Y[Polygon->VertexCount] = NewInY;
                    Polygon->Z[Polygon->VertexCount++] = NewInZ;
                    for(s32 CopyIndex = In + 1;
                        CopyIndex < Buffer.VertexCount;
                        ++CopyIndex)
                    {
                        Polygon->X[Polygon->VertexCount] = Buffer.X[CopyIndex];
                        Polygon->Y[Polygon->VertexCount] = Buffer.Y[CopyIndex];
                        Polygon->Z[Polygon->VertexCount++] = Buffer.Z[CopyIndex];
                    }
                }
                else
                {
                    Polygon->VertexCount = 0;
                    Polygon->X[Polygon->VertexCount] = NewInX;
                    Polygon->Y[Polygon->VertexCount] = NewInY;
                    Polygon->Z[Polygon->VertexCount++] = NewInZ;
                    for(s32 CopyIndex = In + 1;
                        CopyIndex <= Out;
                        ++CopyIndex)
                    {
                        Polygon->X[Polygon->VertexCount] = Buffer.X[CopyIndex];
                        Polygon->Y[Polygon->VertexCount] = Buffer.Y[CopyIndex];
                        Polygon->Z[Polygon->VertexCount++] = Buffer.Z[CopyIndex];
                    }
                    Polygon->X[Polygon->VertexCount] = NewOutX;
                    Polygon->Y[Polygon->VertexCount] = NewOutY;
                    Polygon->Z[Polygon->VertexCount++] = NewOutZ;
                }
            }
        }
    }
}

inline void
ProjectPolygon(render_buffer *RenderBuffer, clipped_polygon *Polygon, v3 CameraP = {NAN})
{
    for(s32 VertexIndex = 0;
        VertexIndex < Polygon->VertexCount;
        ++VertexIndex)
    {
        v3 InVertex = V3(Polygon->X[VertexIndex], Polygon->Y[VertexIndex], Polygon->Z[VertexIndex]);
        v3 OutVertex = Project(RenderBuffer, InVertex, CameraP);
        Polygon->X[VertexIndex] = OutVertex.x;
        Polygon->Y[VertexIndex] = OutVertex.y;
        Polygon->Z[VertexIndex] = OutVertex.z;
    }
}

inline void
PushTriangle(render_buffer *RenderBuffer, v3 A, v3 B, v3 C, v4 Color)
{
    TIMED_FUNCTION();
#if DEBUG_CAMERA
    v3 ClipCameraP = RenderBuffer->ClipCameraP;
    r32 MinX = RenderBuffer->ClipRect.Min.x;
    r32 MaxX = RenderBuffer->ClipRect.Max.x;
    r32 MinY = RenderBuffer->ClipRect.Min.y;
    r32 MaxY = RenderBuffer->ClipRect.Max.y;
#else
    v3 ClipCameraP = RenderBuffer->CameraP;
    r32 MinX = 0.0f;
    r32 MaxX = (r32)RenderBuffer->Width;
    r32 MinY = 0.0f;
    r32 MaxY = (r32)RenderBuffer->Height;
#endif
    clipped_polygon Polygon;
    Polygon.VertexCount = 3;
    Polygon.X[0] = A.x;
    Polygon.Y[0] = A.y;
    Polygon.Z[0] = A.z;
    Polygon.X[1] = B.x;
    Polygon.Y[1] = B.y;
    Polygon.Z[1] = B.z;
    Polygon.X[2] = C.x;
    Polygon.Y[2] = C.y;
    Polygon.Z[2] = C.z;
    Clip(&Polygon, ClipCameraP.z-RenderBuffer->NearZ, ClipDimension_Z, ClipDirection_LessThan);
    ProjectPolygon(RenderBuffer, &Polygon);
    Clip(&Polygon, MaxX, ClipDimension_X, ClipDirection_LessThan);
    Clip(&Polygon, MaxY, ClipDimension_Y, ClipDirection_LessThan);
    Clip(&Polygon, MinX, ClipDimension_X, ClipDirection_GreaterThan);
    Clip(&Polygon, MinY, ClipDimension_Y, ClipDirection_GreaterThan);

    v3 ProjectedA = V3(Polygon.X[0], Polygon.Y[0], Polygon.Z[0]);
    for(s32 TriangleIndex = 0;
        TriangleIndex < Polygon.VertexCount-2;
        ++TriangleIndex)
    {
        PushRenderHeader(Data, &RenderBuffer->Arena, triangle);

        Data->A = ProjectedA.xy;
        Data->B = V2(Polygon.X[TriangleIndex+1], Polygon.Y[TriangleIndex+1]);
        Data->C = V2(Polygon.X[TriangleIndex+2], Polygon.Y[TriangleIndex+2]);
        Data->Color = Color;
        // TODO(chris): How to handle sorting with 3D triangles?
        Data->SortKey = 0.333333f*(ProjectedA.z + Polygon.Z[TriangleIndex+1] + Polygon.Z[TriangleIndex+2]);
    }
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
                
    XAxis = Project(RenderBuffer, Origin + V3(XAxis, 0)).xy;
    YAxis = Project(RenderBuffer, Origin + V3(YAxis, 0)).xy;
    v2 ScreenOrigin = Project(RenderBuffer, Origin).xy;
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

    Data->A = Project(RenderBuffer, A).xy;
    Data->B = Project(RenderBuffer, B).xy;
    Data->C = Project(RenderBuffer, C).xy;
    Data->Color = Color;
    // TODO(chris): How to handle sorting with 3D triangles?
    Data->SortKey = 0.333333f*(A.z + B.z + C.z);
}

inline void
DEBUGPushCircle(render_buffer *RenderBuffer, v3 P, r32 Radius, v4 Color)
{
    PushRenderHeader(Data, &RenderBuffer->Arena, DEBUGcircle);

    Data->P = Project(RenderBuffer, P).xy;
    Data->Radius = Length(Project(RenderBuffer, P + V3(Radius, 0, 0)).xy - Data->P);
    Data->Color = Color;
    Data->SortKey = P.z;
}

// TODO(chris): How do I sort lines that move through z? This goes for bitmaps and rectangles also.
inline void
DEBUGPushLine(render_buffer *RenderBuffer, v3 A, v3 B, v4 Color)
{
    PushRenderHeader(Data, &RenderBuffer->Arena, DEBUGline);

    Data->A = Project(RenderBuffer, A).xy;
    Data->B = Project(RenderBuffer, B).xy;
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

inline void
SplitWorkIntoSquares(render_chunk *RenderChunks, u32 CoreCount, s32 Width, s32 Height,
                     s32 OffsetX, s32 OffsetY)
{
    if(CoreCount == 1)
    {
        RenderChunks->OffsetX = OffsetX;
        RenderChunks->OffsetY = OffsetY;
        RenderChunks->BackBuffer.Height = Height;
        RenderChunks->BackBuffer.Width = Width;
        RenderChunks->CoverageBuffer.Height = RenderChunks->BackBuffer.Height;
        RenderChunks->CoverageBuffer.Width = RenderChunks->BackBuffer.Width;
        RenderChunks->SampleBuffer.Height = RenderChunks->BackBuffer.Height;
        RenderChunks->SampleBuffer.Width = SAMPLE_COUNT*RenderChunks->BackBuffer.Width;
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
            SplitWorkIntoSquares(RenderChunks, HalfCores, HalfWidth, Height, OffsetX, OffsetY);
            SplitWorkIntoSquares(RenderChunks+HalfCores, HalfCores, HalfWidth, Height, OffsetX+HalfWidth, OffsetY);
        }
        else
        {
            s32 HalfHeight = Height/2;
            SplitWorkIntoSquares(RenderChunks, HalfCores, Width, HalfHeight, OffsetX, OffsetY);
            SplitWorkIntoSquares(RenderChunks+HalfCores, HalfCores, Width, HalfHeight, OffsetX, OffsetY+HalfHeight);
        }
    }
}

inline void
SplitWorkIntoHorizontalStrips(render_chunk *RenderChunks, u32 CoreCount, s32 Width, s32 Height)
{
    s32 OffsetY = 0;
    r32 InverseCoreCount = 1.0f / CoreCount;
    for(u32 CoreIndex = 0;
        CoreIndex < CoreCount;
        ++CoreIndex)
    {
        render_chunk *RenderChunk = RenderChunks + CoreIndex;

        s32 NextOffsetY = RoundS32(Height*(CoreIndex+1)*InverseCoreCount);
        
        RenderChunk->OffsetX = 0;
        RenderChunk->OffsetY = OffsetY;
        RenderChunk->BackBuffer.Height = NextOffsetY-OffsetY;
        RenderChunk->BackBuffer.Width = Width;
        RenderChunks->CoverageBuffer.Height = RenderChunk->BackBuffer.Height;
        RenderChunks->CoverageBuffer.Width = RenderChunks->BackBuffer.Width;
        RenderChunks->SampleBuffer.Height = RenderChunk->BackBuffer.Height;
        RenderChunks->SampleBuffer.Width = SAMPLE_COUNT*RenderChunks->BackBuffer.Width;

        OffsetY = NextOffsetY;
    }
}

inline void
SplitWorkIntoVerticalStrips(render_chunk *RenderChunks, u32 CoreCount, s32 Width, s32 Height)
{
    s32 OffsetX = 0;
    r32 InverseCoreCount = 1.0f / CoreCount;
    for(u32 CoreIndex = 0;
        CoreIndex < CoreCount;
        ++CoreIndex)
    {
        render_chunk *RenderChunk = RenderChunks + CoreIndex;

        s32 NextOffsetX = RoundS32(Width*(CoreIndex+1)*InverseCoreCount);
        
        RenderChunk->OffsetX = OffsetX;
        RenderChunk->OffsetY = 0;
        RenderChunk->BackBuffer.Height = Height;
        RenderChunk->BackBuffer.Width = NextOffsetX-OffsetX;
        RenderChunks->CoverageBuffer.Height = RenderChunk->BackBuffer.Height;
        RenderChunks->CoverageBuffer.Width = RenderChunks->BackBuffer.Width;
        RenderChunks->SampleBuffer.Height = RenderChunk->BackBuffer.Height;
        RenderChunks->SampleBuffer.Width = SAMPLE_COUNT*RenderChunks->BackBuffer.Width;

        OffsetX = NextOffsetX;
    }
}

#define TROIDS_RENDER_H
#endif
