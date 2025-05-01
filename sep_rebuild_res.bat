@echo off
setlocal enabledelayedexpansion

:: Initialize variables
set clean=0
set debug_build=0
set code_only=0
set run_emul=0

:: Get current path and remove trailing backslash
set "current_path=%~dp0"
if "%current_path:~-1%"=="\" set "current_path=%current_path:~0,-1%"

:: Split path and search for smd_dev folder
set "smd_dev_path="
:loop
for %%i in ("%current_path%") do (
    set "folder=%%~nxi"
    if /i "!folder!"=="smd_dev" (
        set "smd_dev_path=!current_path!"
        goto found
    )
)

:: Move one level up in directory tree
for %%i in ("%current_path%") do set "parent=%%~dpi"
if "%parent%"=="%current_path%" goto not_found
set "current_path=%parent%"
if "%current_path:~-1%"=="\" set "current_path=%current_path:~0,-1%"
goto loop

:not_found
echo Error: smd_dev folder not found!
exit /b 1

:found
echo smd_dev folder located at: %smd_dev_path%
echo args: %*

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
