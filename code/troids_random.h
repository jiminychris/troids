#if !defined(TROIDS_RANDOM_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

struct seed
{
    u64 Value;
};

inline seed
Seed(u64 Value)
{
    seed Result;
    Result.Value = Value;
    return(Result);
}

inline u64
RandomU64(seed *Seed)
{
    u64 Result = 6364136223846793005*Seed->Value+1442695040888963407;
    Seed->Value = Result;
    return(Result);
}

global_variable r32 InvU64Max = 1.0f / (r32)18446744073709551615;

inline r32
Random01(seed *Seed)
{
    r32 Result = Clamp01((r32)RandomU64(Seed)*InvU64Max);
    return(Result);
}

inline r32
RandomBetween(seed *Seed, r32 Min, r32 Max)
{
    Assert(Min <= Max);
    r32 Result = Min + Random01(Seed)*(Max-Min);
    return(Result);
}

inline s32
RandomBetween(seed *Seed, s32 Min, s32 Max)
{
    Assert(Min <= Max);
    u32 Mod = 1 + Max-Min;
    s32 Result = Min + RandomU64(Seed)%Mod;
    return(Result);
}

inline u32
RandomIndex(seed *Seed, u32 Count)
{
    s32 Result = RandomU64(Seed)%Count;
    return(Result);
}

#define TROIDS_RANDOM_H
#endif
