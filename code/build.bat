@echo off

if not exist w:\build mkdir w:\build
pushd w:\build

set CommonCompilerFlags=/Od -DTROIDS_RUMBLE=0 -DTROIDS_INTERNAL=1 -DTROIDS_PROFILE=1 /Zi /fp:fast /W4 /WX /nologo /wd4100 /wd4065 /wd4189 /wd4201 /wd4505
set CommonLinkerFlags=/DEBUG /WX /NOLOGO /OPT:REF

if exist troids.dll move troids.dll temp.dll 1> nul
echo BUILDING > pdb.lock
cl %CommonCompilerFlags% /LD w:\troids\code\troids.cpp /link %CommonLinkerFlags% /EXPORT:GameUpdateAndRender /EXPORT:GameGetSoundSamples /EXPORT:DebugCollate /PDB:troids_%RANDOM%.pdb
del pdb.lock
cl %CommonCompilerFlags% w:\troids\code\win32_troids.cpp /link %CommonLinkerFlags% user32.lib gdi32.lib winmm.lib opengl32.lib

popd
