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

r32 Sin(r32 A)
{
    r32 Result = sinf(A);
    return(Result);
}

#define TROIDS_MATH_H
#endif
