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

inline r32
Sin(r32 A)
{
    r32 Result = sinf(A);
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

inline s32
Minimum(s32 A, s32 B)
{
    if(B < A)
    {
        A = B;
    }
    return(A);
}

inline s32
Maximum(s32 A, s32 B)
{
    if(B > A)
    {
        A = B;
    }
    return(A);
}

inline r32
Minimum(r32 A, r32 B)
{
    if(B < A)
    {
        A = B;
    }
    return(A);
}

inline r32
Maximum(r32 A, r32 B)
{
    if(B > A)
    {
        A = B;
    }
    return(A);
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

inline r32
Clamp01(r32 A)
{
    r32 Result = Clamp(0.0f, A, 1.0f);
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
operator+(v2 A, v2 B)
{
    v2 Result;
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
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
operator+=(v2 &A, v2 B)
{
    A = A + B;
    return(A);
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

#define TROIDS_MATH_H
#endif
