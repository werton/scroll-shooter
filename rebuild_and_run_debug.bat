::CALL "%~dp0build.bat" clean && CALL "%~dp0build.bat" debug && CALL "%~dp0run.bat"
CALL "%~dp0build.bat" clean debug run
ping 127.0.0.1 -n 2 > nul
