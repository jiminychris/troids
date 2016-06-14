#if !defined(TROIDS_MATH_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include <math.h>

r32 Sin(r32 A)
{
    r32 Result = sinf(A);
    return(Result);
}

#define TROIDS_MATH_H
#endif
