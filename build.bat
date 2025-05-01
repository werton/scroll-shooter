@echo off
setlocal enabledelayedexpansion

:: Initialize variables
set clean=0
set debug_build=0
set code_only=0
set run_emul=0
set paused=0

:: Parse command line arguments (both with and without dash prefix)
for %%x in (%*) do (
    set "arg=%%~x"
    if /i "!arg!"=="clean" set clean=1
    if /i "!arg!"=="cleanrelease" set clean=1
    if /i "!arg!"=="debug" (
        set debug_build=1
        set multi_core_flag=-j4
    )
    if /i "!arg!"=="code-only" set code_only=1
    if /i "!arg!"=="run" set run_emul=1
    if /i "!arg!"=="pause" set paused=1
)


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
set args=%multi_core_flag% %args%

set build_result=1

if !clean! == 1 (goto clean) else (goto build)

:clean
:: Run build via Make. If return code is not 0, show error message and exit
(CALL "%smd_dev_path%\devkit\sgdk\sgdk_current\bin\make.exe" -f "%smd_dev_path%\devkit\sgdk\sgdk_current\makefile.gen" %args%) || (set build_result=0)
goto print

:build
:: Run build via Make. If return code is not 0, show error message and exit
CALL "%~dp0timecmd.bat" (CALL "%smd_dev_path%\devkit\sgdk\sgdk_current\bin\make.exe" -f "%smd_dev_path%\devkit\sgdk\sgdk_current\makefile.gen" %args%) || (set build_result=0)

:print
:: Enable ANSI color codes in console
reg add "HKCU\Console" /v VirtualTerminalLevel /t REG_DWORD /d 1 /f >nul

if %build_result%==1 (
    @ECHO.
    @ECHO.
    if %clean% == 1 (
    	echo [32mClean done.[0m
    ) else (
    	echo [32mBuild done.[0m
    )
    @ECHO.
    @ECHO.
) else (
    @ECHO.
    @ECHO.
    echo [31mBUILD FAILED. STOPPED.[0m
    echo press any key.
    @PAUSE >nul
    exit /b 1
)


:: Launch emulator if requested
if %run_emul% == 1 (
    echo Launching emulator...    
    CALL "%~dp0run.bat"
)

if %paused% == 1 (
@PAUSE >nul
)

goto end

:end
endlocal
exit /b 1