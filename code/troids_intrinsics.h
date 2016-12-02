#if !defined(TROIDS_INTRINSICS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

typedef struct bitscan_result
{
    b32 Found;
    u32 Index;
} bitscan_result;

#if COMPILER_MSVC

inline bitscan_result
BitScanForward(u32 Mask)
{
    bitscan_result Result = {};
    Result.Found = _BitScanForward((unsigned long *)(&Result.Index), Mask);
    return(Result);
}

inline u32
AtomicIncrement(volatile u32 *Value)
{
    u32 Result;
    Result = InterlockedIncrement(Value);
    return(Result);
}

inline b32
AtomicCompareExchange(volatile u32 *Destination, u32 Exchange, u32 Comparand)
{
    b32 Result;
    Result = (InterlockedCompareExchange(Destination, Exchange, Comparand) == Comparand);
    return(Result);
}

inline u64
GetCurrentThreadID(void)
{
    u64 Result;
    Result = (u64)GetCurrentThreadId();
    return(Result);
}

#define rdtsc() (u64)__rdtsc()

#elif COMPILER_LLVM

inline bitscan_result
BitScanForward(u32 Mask)
{
    bitscan_result Result = {};
    Result.Found = (Mask != 0);
    Result.Index = __builtin_ctz(Mask);
    return(Result);
}

inline u32
AtomicIncrement(volatile u32 *Value)
{
    u32 Result;
    Result = __sync_fetch_and_add(Value, 1) + 1;
    return(Result);
}

inline b32
AtomicCompareExchange(volatile u32 *Destination, u32 Exchange, u32 Comparand)
{
    b32 Result;
    Result = (__sync_val_compare_and_swap(Destination, Comparand, Exchange) == Comparand);
    return(Result);
}

inline u64
GetCurrentThreadID(void)
{
    u64 Result;
    pthread_threadid_np(pthread_self(), &Result);
    return(Result);
}

#define rdtsc() (u64)mach_absolute_time()

#endif

#define TROIDS_INTRINSICS_H
#endif
