@echo off

if not exist w:\build mkdir w:\build
pushd w:\build

set CommonCompilerFlags=/Od /Zi /fp:fast /W4 /WX /nologo /wd4100 /wd4065 /wd4189
set CommonLinkerFlags=/DEBUG /WX /NOLOGO /OPT:REF

cl %CommonCompilerFlags% /LD w:\troids\code\troids.cpp /link %CommonLinkerFlags% /EXPORT:GameUpdateAndRender
cl %CommonCompilerFlags% w:\troids\code\win32_troids.cpp /link %CommonLinkerFlags% user32.lib gdi32.lib

popd
