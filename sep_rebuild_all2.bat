@echo off
setlocal enabledelayedexpansion

set paused=1

:: Parse command line arguments (both with and without dash prefix)
if "%~1"=="nopause" set paused=0

:: Run build via Make. If return code is not 0, show error message and exit
call %~dp0timecmd.bat (call %~dp0sep_rebuild_res2.bat nopause) && (call %~dp0sep_build_code2.bat nopause)

if !paused!==1  (
	echo.
	echo Done. Press any key.
	@PAUSE >nul
)

endlocal
exit /b 0



