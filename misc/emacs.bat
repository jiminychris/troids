@echo off

if not defined EMACS_LOCATION (
   echo EMACS_LOCATION not set.
   exit /B
)

"%EMACS_LOCATION%\bin\runemacs.exe" -q -l W:\troids\misc\.emacs %*
