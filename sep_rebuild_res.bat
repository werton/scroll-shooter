@echo off
setlocal enabledelayedexpansion

:: Initialize variables
set clean=0
set debug_build=0
set code_only=0
set run_emul=0

:: Get SMD_DEV_PATH path
call find_smd_dev.bat SMD_DEV_PATH
if errorlevel 1 (
    echo Error: smd_dev folder not found!
    pause
    exit /b 1
)

:: Process arguments - remove 'run' flag and add -jN flag for make
set args=%*
set args=%args:run=%


set build_result=1

CALL "%~dp0build.bat" clean

CALL "%smd_dev_path%\devkit\sgdk\sgdk_current\bin\make.exe" -f "%smd_dev_path%\devkit\sgdk\sgdk_current\makefile_0.gen" %args%

:: Enable ANSI color codes in console
reg add "HKCU\Console" /v VirtualTerminalLevel /t REG_DWORD /d 1 /f >nul

if !errorlevel!==0 (
    @ECHO.
    @ECHO.
	echo [32mResources build done.[0m
    @ECHO.
    @ECHO.
 )  
endlocal
