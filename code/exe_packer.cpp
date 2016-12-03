/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

#include "troids_platform.h"
#include <stdio.h>

#define OUT_PATH "w:\\build\\troids.exe"
#define EXE_PATH "w:\\build\\win32_troids.exe"
#define ASSET_PATH "w:\\troids\\data\\"

int main(int ArgCount, char **Args)
{
    FILE *EXEFile, *OutFile, *AssetFile;
    errno_t EXEFileOpenResult = fopen_s(&EXEFile, EXE_PATH, "rb");
    errno_t OutFileOpenResult = fopen_s(&OutFile, OUT_PATH, "wb");
    // TODO(chris): If there are more assets, this needs to be more systematic.
    errno_t AssetFileOpenResult = fopen_s(&AssetFile, ASSET_PATH"assets.tra", "rb");

    Assert(EXEFileOpenResult == 0);
    Assert(OutFileOpenResult == 0);
    Assert(AssetFileOpenResult == 0);

    char Buffer[4096];

    size_t BytesRead;
    do
    {
        BytesRead = fread(Buffer, 1, sizeof(Buffer), EXEFile);
        fwrite(Buffer, 1, BytesRead, OutFile);
    } while(BytesRead);
    do
    {
        BytesRead = fread(Buffer, 1, sizeof(Buffer), AssetFile);
        fwrite(Buffer, 1, BytesRead, OutFile);
    } while(BytesRead);
    u32 AssetSize = ftell(AssetFile);
    fwrite(&AssetSize, sizeof(u32), 1, OutFile);
    
    return 0;
}
