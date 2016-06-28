#if !defined(TROIDS_INTRINSICS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

// TODO(chris): Intrinsics for different platforms

#include "intrin.h"

struct bitscan_result
{
    b32 Found;
    u32 Index;
};
inline bitscan_result
BitScanForward(u32 Mask)
{
    bitscan_result Result = {};
    Result.Found = _BitScanForward((unsigned long *)(&Result.Index), Mask);
    return(Result);
}

#define TROIDS_INTRINSICS_H
#endif
