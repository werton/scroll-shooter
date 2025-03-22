CALL "%~dp0build.bat" clean
CALL "%~dp0build.bat" release
ping 127.0.0.1 -n 2 > nul
