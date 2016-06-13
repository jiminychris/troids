@echo off

if not exist w:\build mkdir w:\build
pushd w:\build

set CommonCompilerFlags=/Od /Zi /fp:fast /W4 /WX /nologo /wd4100 /wd4065
set CommonLinkerFlags=/DEBUG /WX /NOLOGO /OPT:REF user32.lib

cl %CommonCompilerFlags% w:\troids\code\win32_troids.cpp /link %CommonLinkerFlags%

popd
