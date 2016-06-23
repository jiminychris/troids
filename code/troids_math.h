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

#define TROIDS_MATH_H
#endif
