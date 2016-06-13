@echo off

if not defined EMACS_LOCATION (
   echo Emacs location not set.
   exit /B
)

"%EMACS_LOCATION%\bin\runemacs.exe" -q -l W:\troids\misc\.emacs %*
