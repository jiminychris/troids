#if !defined(TROIDS_ASSETS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

typedef struct tra_bitmap
{
    s32 Height;
    s32 Width;
    v2 Align;
    s32 Pitch;
    u32 MemoryOffset;
} tra_bitmap;

typedef struct game_assets
{
    u32 FontCount;
    loaded_font *Fonts;
    u32 BitmapCount;
    tra_bitmap *Bitmaps;

    void *Memory;

    // NOTE(chris): Stuff below this is not part of the file format

    memory_arena Arena;
} asset_file_header;

inline loaded_bitmap
GetBitmap(game_assets *Assets, bitmap_id ID)
{
    loaded_bitmap Result = {};
    if(ID.Value)
    {
        tra_bitmap *Bitmap = Assets->Bitmaps + ID.Value;

        Result.Height = Bitmap->Height;
        Result.Width = Bitmap->Width;
        Result.Align = Bitmap->Align;
        Result.Pitch = Bitmap->Pitch;
        Result.Memory = (u8 *)Assets->Memory + Bitmap->MemoryOffset;
    }

    return(Result);
}

#define TROIDS_ASSETS_H
#endif
