#if !defined(TROIDS_MATH_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include <math.h>

#define Tau 6.28318530717958647692528676655900576839433879875021f
#define Pi 0.5f*Tau

inline r32
Sin(r32 A)
{
    r32 Result = sinf(A);
    return(Result);
}

inline r32
Cos(r32 A)
{
    r32 Result = cosf(A);
    return(Result);
}

inline r32
InverseCos(r32 A)
{
    r32 Result = acosf(A);
    return(Result);
}

inline r32
InverseTan(r32 Y, r32 X)
{
    r32 Result = atan2f(Y, X);
    return(Result);
}

inline r32
InverseTan(v2 A)
{
    r32 Result = atan2f(A.y, A.x);
    return(Result);
}

inline s32
Ceiling(r32 A)
{
    s32 Result = (s32)ceilf(A);
    return(Result);
}

inline s32
Floor(r32 A)
{
    s32 Result = (s32)floorf(A);
    return(Result);
}

inline r32
Sign(r32 A)
{
    r32 Result = (A < 0) ? -1.0f : 1.0f;
    return(Result);
}

inline b32
IsNaN(r32 A)
{
    b32 Result = isnan(A);
    return(Result);
}

inline r32
SquareRoot(r32 A)
{
    r32 Result = sqrtf(A);
    return(Result);
}

inline r32
Square(r32 A)
{
    r32 Result = A*A;
    return(Result);
}

inline r32
Cube(r32 A)
{
    r32 Result = A*A*A;
    return(Result);
}

inline s32
RoundS32(r32 A)
{
    s32 Result = (s32)(A + 0.5f);
    return(Result);
}

inline u32
RoundU32(r32 A)
{
    u32 Result = (u32)(A + 0.5f);
    return(Result);
}

inline r32
AbsoluteValue(r32 A)
{
    r32 Result = fabs(A);
    return(Result);
}

inline s32
Clamp(s32 Min, s32 A, s32 Max)
{
    s32 Result = Maximum(Min, Minimum(Max, A));
    return(Result);
}

inline r32
Clamp(r32 Min, r32 A, r32 Max)
{
    r32 Result = Maximum(Min, Minimum(Max, A));
    return(Result);
}

inline r64
Clamp(r64 Min, r64 A, r64 Max)
{
    r64 Result = Maximum(Min, Minimum(Max, A));
    return(Result);
}

inline r32
Clamp01(r32 A)
{
    r32 Result = Clamp(0.0f, A, 1.0f);
    return(Result);
}

inline r64
Clamp01(r64 A)
{
    r64 Result = Clamp(0.0, A, 1.0);
    return(Result);
}

inline v2
V2(r32 X, r32 Y)
{
    v2 Result;
    Result.x = X;
    Result.y = Y;
    return(Result);
}

inline v2
V2i(s32 X, s32 Y)
{
    v2 Result;
    Result.x = (r32)X;
    Result.y = (r32)Y;
    return(Result);
}

inline v2
operator-(v2 A)
{
    v2 Result;
    Result.x = -A.x;
    Result.y = -A.y;
    return(Result);
}

inline v2
operator+(v2 A, v2 B)
{
    v2 Result;
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    return(Result);
}

inline v2
operator-(v2 A, v2 B)
{
    v2 Result;
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    return(Result);
}

inline v2
operator*(r32 C, v2 A)
{
    v2 Result;
    Result.x = C*A.x;
    Result.y = C*A.y;
    return(Result);
}

inline v2
operator*(v2 A, r32 C)
{
    v2 Result;
    Result.x = C*A.x;
    Result.y = C*A.y;
    return(Result);
}

inline v2
operator/(v2 A, r32 C)
{
    v2 Result;
    r32 InvC = 1.0f/C;
    Result.x = A.x*InvC;
    Result.y = A.y*InvC;
    return(Result);
}

inline v2
operator+=(v2 &A, v2 B)
{
    A = A + B;
    return(A);
}

inline v2
operator-=(v2 &A, v2 B)
{
    A = A - B;
    return(A);
}

inline v2
operator*=(v2 &A, r32 C)
{
    A = C*A;
    return(A);
}

inline r32
Inner(v2 A, v2 B)
{
    r32 Result = A.x*B.x + A.y*B.y;
    return(Result);
}

inline v2
Perp(v2 A)
{
    v2 Result;
    Result.x = -A.y;
    Result.y = A.x;
    return(Result);
}

inline r32
Length(v2 A)
{
    r32 Result = SquareRoot(A.x*A.x + A.y*A.y);
    return(Result);
}

inline r32
LengthSq(v2 A)
{
    r32 Result = A.x*A.x + A.y*A.y;
    return(Result);
}

inline v2
Project(v2 A, v2 B)
{
    v2 Result = A;
    r32 LengthBSq = LengthSq(B);
    if(LengthBSq > 0.0f)
    {
        Result = (B*Inner(A, B))/LengthBSq;
    }
    return(Result);
}

inline v2
Hadamard(v2 A, v2 B)
{
    v2 Result;
    Result.x = A.x*B.x;
    Result.y = A.y*B.y;
    return(Result);
}

inline v2
Normalize(v2 A)
{
    v2 Result = A/Length(A);
    return(Result);
}

inline v3
V3(r32 X, r32 Y, r32 Z)
{
    v3 Result;
    Result.x = X;
    Result.y = Y;
    Result.z = Z;
    return(Result);
}

inline v3
V3i(s32 X, s32 Y, s32 Z)
{
    v3 Result;
    Result.x = (r32)X;
    Result.y = (r32)Y;
    Result.z = (r32)Z;
    return(Result);
}

inline v3
V3(v2 XY, r32 Z)
{
    v3 Result;
    Result.x = XY.x;
    Result.y = XY.y;
    Result.z = Z;
    return(Result);
}

inline v3
operator-(v3 A)
{
    v3 Result;
    Result.x = -A.x;
    Result.y = -A.y;
    Result.z = -A.z;
    return(Result);
}

inline v3
operator+(v3 A, v3 B)
{
    v3 Result;
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    return(Result);
}

inline v3
operator-(v3 A, v3 B)
{
    v3 Result;
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;
    return(Result);
}

inline v3
operator*(r32 C, v3 A)
{
    v3 Result;
    Result.x = C*A.x;
    Result.y = C*A.y;
    Result.z = C*A.z;
    return(Result);
}

inline v3
operator*(v3 A, r32 C)
{
    v3 Result;
    Result.x = C*A.x;
    Result.y = C*A.y;
    Result.z = C*A.z;
    return(Result);
}

inline v3
operator/(v3 A, r32 C)
{
    v3 Result;
    r32 InvC = 1.0f/C;
    Result.x = A.x*InvC;
    Result.y = A.y*InvC;
    Result.z = A.z*InvC;
    return(Result);
}

inline v3
operator+=(v3 &A, v3 B)
{
    A = A + B;
    return(A);
}

inline v3
operator*=(v3 &A, r32 C)
{
    A = C*A;
    return(A);
}

inline r32
Inner(v3 A, v3 B)
{
    r32 Result = A.x*B.x + A.y*B.y + A.z*B.z;
    return(Result);
}

inline r32
LengthSq(v3 A)
{
    r32 Result = A.x*A.x + A.y*A.y + A.z*A.z;
    return(Result);
}

inline r32
Length(v3 A)
{
    r32 Result = SquareRoot(A.x*A.x + A.y*A.y + A.z*A.z);
    return(Result);
}

inline v3
Project(v3 A, v3 B)
{
    v3 Result = A;
    r32 LengthBSq = LengthSq(B);
    if(LengthBSq > 0.0f)
    {
        Result = (B*Inner(A, B))/LengthBSq;
    }
    return(Result);
}

inline v3
Hadamard(v3 A, v3 B)
{
    v3 Result;
    Result.x = A.x*B.x;
    Result.y = A.y*B.y;
    Result.z = A.z*B.z;
    return(Result);
}

inline v4
V4(r32 X, r32 Y, r32 Z, r32 W)
{
    v4 Result;
    Result.x = X;
    Result.y = Y;
    Result.z = Z;
    Result.w = W;
    return(Result);
}

inline v4
V4i(u32 X, u32 Y, u32 Z, u32 W)
{
    v4 Result;
    Result.x = (r32)X;
    Result.y = (r32)Y;
    Result.z = (r32)Z;
    Result.w = (r32)W;
    return(Result);
}

inline v4
operator+(v4 A, v4 B)
{
    v4 Result;
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    Result.w = A.w + B.w;
    return(Result);
}

inline v4
operator*(r32 C, v4 A)
{
    v4 Result;
    Result.x = C*A.x;
    Result.y = C*A.y;
    Result.z = C*A.z;
    Result.w = C*A.w;
    return(Result);
}

inline v4
operator*(v4 A, r32 C)
{
    v4 Result;
    Result.x = C*A.x;
    Result.y = C*A.y;
    Result.z = C*A.z;
    Result.w = C*A.w;
    return(Result);
}

inline v4
operator*=(v4 &A, r32 C)
{
    A = C*A;
    return(A);
}

inline u32
InvertColor(u32 PackedColor)
{
    u32 Result = (((255-((PackedColor >> 16) & 0xFF)) << 16) |
                  ((255-((PackedColor >> 8) & 0xFF)) << 8) |
                  ((255-((PackedColor >> 0) & 0xFF)) << 0) |
                  (255 << 24));
    return(Result);
}

inline rectangle2
CenterDim(v2 Center, v2 Dim)
{
    v2 Radius = 0.5f*Dim;
    rectangle2 Result;
    Result.Min = Center - Radius;
    Result.Max = Center + Radius;
    return(Result);
}

inline rectangle2
MinDim(v2 Min, v2 Dim)
{
    rectangle2 Result;
    Result.Min = Min;
    Result.Max = Result.Min + Dim;
    return(Result);
}

inline rectangle2
MinMax(v2 Min, v2 Max)
{
    rectangle2 Result;
    Result.Min = Min;
    Result.Max = Max;
    return(Result);
}

inline rectangle2
AddRadius(rectangle2 Rect, r32 Radius)
{
    rectangle2 Result;
    Result.Min = V2(Rect.Min.x - Radius, Rect.Min.y - Radius);
    Result.Max = V2(Rect.Max.x + Radius, Rect.Max.y + Radius);
    return(Result);
}

inline rectangle2
AddRadius(rectangle2 Rect, v2 Radius)
{
    rectangle2 Result;
    Result.Min = V2(Rect.Min.x - Radius.x, Rect.Min.y - Radius.y);
    Result.Max = V2(Rect.Max.x + Radius.x, Rect.Max.y + Radius.y);
    return(Result);
}

inline v2
GetDim(rectangle2 Rect)
{
    v2 Result;
    Result.x = Rect.Max.x - Rect.Min.x;
    Result.y = Rect.Max.y - Rect.Min.y;
    return(Result);
}

inline v2
GetCenter(rectangle2 Rect)
{
    v2 Result;
    Result.x = (Rect.Max.x + Rect.Min.x)*0.5f;
    Result.y = (Rect.Max.y + Rect.Min.y)*0.5f;
    return(Result);
}

inline rectangle2
TopLeftDim(v2 TopLeft, v2 Dim)
{
    rectangle2 Result;
    Result.Min = V2(TopLeft.x, TopLeft.y - Dim.y);
    Result.Max = V2(TopLeft.x + Dim.x, TopLeft.y);
    return(Result);
}

inline b32
InsideX(rectangle2 Rect, v2 Point)
{
    b32 Result = ((Point.x >= Rect.Min.x) && (Point.x <= Rect.Max.x));

    return(Result);
}

inline b32
InsideY(rectangle2 Rect, v2 Point)
{
    b32 Result = ((Point.y >= Rect.Min.y) && (Point.y <= Rect.Max.y));

    return(Result);
}

inline b32
InsideX(rectangle2 Rect, r32 X)
{
    b32 Result = ((X >= Rect.Min.x) && (X <= Rect.Max.x));

    return(Result);
}

inline b32
InsideY(rectangle2 Rect, r32 Y)
{
    b32 Result = ((Y >= Rect.Min.y) && (Y <= Rect.Max.y));

    return(Result);
}

inline b32
Inside(rectangle2 Rect, v2 Point)
{
    b32 Result = (Point.x >= Rect.Min.x &&
                  Point.x < Rect.Max.x &&
                  Point.y >= Rect.Min.y &&
                  Point.y < Rect.Max.y);

    return(Result);
}

inline r32
Unlerp(r32 A, r32 X, r32 B)
{
    r32 Result = (X - A) / (B - A);
    Result = Clamp01(Result);
    return(Result);
}

inline r64
Unlerp(r64 A, r64 X, r64 B)
{
    r64 Result = (X - A) / (B - A);
    Result = Clamp01(Result);
    return(Result);
}

inline r32
Lerp(r32 A, r32 t, r32 B)
{
    r32 tInv = 1.0f - t;
    r32 Result;
    Result = A*tInv + B*t;
    return(Result);
}

inline v4
Lerp(v4 A, r32 t, v4 B)
{
    r32 tInv = 1.0f - t;
    v4 Result;
    Result.x = A.x*tInv + B.x*t;
    Result.y = A.y*tInv + B.y*t;
    Result.z = A.z*tInv + B.z*t;
    Result.w = A.w*tInv + B.w*t;
    return(Result);
}

inline m22
operator*(m22 A, m22 B)
{
    m22 Result =
    {
        A.E[0]*B.E[0] + A.E[1]*B.E[2], A.E[0]*B.E[1] + A.E[1]*B.E[3], 
        A.E[2]*B.E[0] + A.E[3]*B.E[2], A.E[2]*B.E[1] + A.E[3]*B.E[3],
    };
    return(Result);
}

inline v2
operator*(m22 A, v2 X)
{
    v2 Result =
    {
        A.E[0]*X.x + A.E[1]*X.y,
        A.E[2]*X.x + A.E[3]*X.y,
    };
    return(Result);
}

inline m33
operator*(m33 A, m33 B)
{
    m33 Result =
    {
        A.E[0]*B.E[0] + A.E[1]*B.E[3] + A.E[2]*B.E[6], A.E[0]*B.E[1] + A.E[1]*B.E[4] + A.E[2]*B.E[7], A.E[0]*B.E[2] + A.E[1]*B.E[5] + A.E[2]*B.E[8], 
        A.E[3]*B.E[0] + A.E[4]*B.E[3] + A.E[5]*B.E[6], A.E[3]*B.E[1] + A.E[4]*B.E[4] + A.E[5]*B.E[7], A.E[3]*B.E[2] + A.E[4]*B.E[5] + A.E[5]*B.E[8], 
        A.E[6]*B.E[0] + A.E[7]*B.E[3] + A.E[8]*B.E[6], A.E[6]*B.E[1] + A.E[7]*B.E[4] + A.E[8]*B.E[7], A.E[6]*B.E[2] + A.E[7]*B.E[5] + A.E[8]*B.E[8], 
    };
    return(Result);
}

inline v3
operator*(m33 A, v3 X)
{
    v3 Result =
    {
        A.E[0]*X.x + A.E[1]*X.y + A.E[2]*X.z,
        A.E[3]*X.x + A.E[4]*X.y + A.E[5]*X.z,
        A.E[6]*X.x + A.E[7]*X.y + A.E[8]*X.z,
    };
    return(Result);
}

inline v3
RotateZ(v3 A, r32 Angle)
{
    r32 CosRot = Cos(Angle);
    r32 SinRot = Sin(Angle);
    m33 RotMat =
    {
        CosRot, -SinRot, 0,
        SinRot, CosRot, 0,
        0, 0, 1,
    };
    v3 Result = RotMat*A;
    return(Result);
}

inline v2
RotateZ(v2 A, r32 Angle)
{
    r32 CosRot = Cos(Angle);
    r32 SinRot = Sin(Angle);
    m22 RotMat =
    {
        CosRot, -SinRot,
        SinRot, CosRot,
    };
    v2 Result = RotMat*A;
    return(Result);
}

inline v4
sRGB255ToLinear1(v4 Color)
{
    r32 Inv255 = 1.0f / 255.0f;

    v4 Result;
    Result.r = Square(Color.r*Inv255);
    Result.g = Square(Color.g*Inv255);
    Result.b = Square(Color.b*Inv255);
    Result.a = Color.a*Inv255;
    
    return(Result);
}

inline v4
sRGB1ToLinear1(v4 Color)
{
    v4 Result;
    Result.r = Square(Color.r);
    Result.g = Square(Color.g);
    Result.b = Square(Color.b);
    Result.a = Color.a;
    
    return(Result);
}

inline v4
Linear1TosRGB255(v4 Color)
{
    v4 Result;
    Result.r = SquareRoot(Color.r);
    Result.g = SquareRoot(Color.g);
    Result.b = SquareRoot(Color.b);
    Result.a = Color.a;

    Result *= 255.0f;
    return(Result);
}

#define TROIDS_MATH_H
#endif
