@echo off

if not defined VCVARSALL (
   echo VCVARSALL not set.
   exit /B
)

call "%VCVARSALL%" x64
set path=W:\troids\misc;%PATH%
