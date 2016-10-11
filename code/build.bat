@echo off

if not exist w:\build mkdir w:\build
pushd w:\build

set Release=1
set Portable=1

if %Release%==1 (
set Internal=0
set DebugDisplay=0
set Slow=0
set Rumble=1
set Optimize=/O2
) else (
set Internal=1
set DebugDisplay=1
set Slow=1
set Rumble=0
set Optimize=/O2
)

if %Portable%==1 (
set DataPath=\"data\"
) else (
set DataPath=\"..\\troids\\data\"
)

set CommonCompilerFlags=%Optimize% -DTROIDS_RUMBLE=%Rumble% -DTROIDS_INTERNAL=%Internal% -DTROIDS_SLOW=%Slow% -DTROIDS_DEBUG_DISPLAY=%DebugDisplay% -DDATA_PATH=%DataPath% /Zi /fp:fast /W4 /WX /nologo /wd4100 /wd4065 /wd4189 /wd4201 /wd4505
set CommonLinkerFlags=/DEBUG /WX /NOLOGO /OPT:REF

if exist troids.dll move troids.dll temp.dll 1> nul
echo BUILDING > pdb.lock
if %Internal%==1 (
cl %CommonCompilerFlags% /LD w:\troids\code\troids.cpp /link %CommonLinkerFlags% /EXPORT:GameUpdateAndRender /EXPORT:GameGetSoundSamples /EXPORT:DebugCollate /PDB:troids_%RANDOM%.pdb
) else (
cl %CommonCompilerFlags% /LD w:\troids\code\troids.cpp /link %CommonLinkerFlags% /EXPORT:GameUpdateAndRender /EXPORT:GameGetSoundSamples /PDB:troids_%RANDOM%.pdb
)

del pdb.lock
cl %CommonCompilerFlags% w:\troids\code\win32_troids.cpp /link %CommonLinkerFlags% user32.lib gdi32.lib winmm.lib opengl32.lib

popd
