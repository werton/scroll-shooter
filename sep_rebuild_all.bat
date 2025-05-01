@echo off
setlocal enabledelayedexpansion

set paused=1

:: Parse command line arguments (both with and without dash prefix)
for %%x in (%*) do (
	echo arg: "!arg!"
    set "arg=%%~x"
    if /i "!arg:-=!"=="nopause" set paused=0
)


set build_result=0

:: Run build via Make. If return code is not 0, show error message and exit
call %~dp0timecmd.bat call %~dp0sep_rebuild_res.bat && call %~dp0sep_build_code.bat

if %errorlevel% equ 0 (set build_result=1)


if !paused!==1  (
	echo.
	echo.
	echo Complete. Press any key.
	@PAUSE >nul
)


endlocal
exit /b 1



