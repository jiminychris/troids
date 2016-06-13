@echo off

if not defined GIT_LOCATION (
   echo Git location not set.
   exit /B
)

"%GIT_LOCATION%\bin\sh.exe" --login -i
