@echo off
setlocal enabledelayedexpansion

:: Get SMD_DEV_PATH path
call find_smd_dev.bat smd_dev_path
if errorlevel 1 (
    echo Error: smd_dev folder not found!
    pause
    exit /b 1
)

:: Initialize variables
set clean=0
set res=0
set code=0
set sep=0
set release=0
set run=0
set paused=1
set build_result=0

:: Process arguments - remove custom flag
set args="%*"

:: Parse command line arguments (both with and without dash prefix)
for %%x in (%*) do (
    set "arg=%%~x"
    if /i "!arg!"=="clean" set clean=1
    if /i "!arg!"=="res" set sep=1 & set res=1 & set release=1
    if /i "!arg!"=="code" set sep=1 & set code=1 & set release=1      
    if /i "!arg!"=="sep" set sep=1 & set res=1 & set code=1 & set release=1
    if /i "!arg!"=="release" set release=1
    if /i "!arg!"=="debug" set release=2
    if /i "!arg!"=="run" set run=1
    if /i "!arg!"=="nopause" set paused=0
)

if !args!=="" set args="release" & set release=1

if !res!==1 set args=%args:res=%
if !code!==1 set args=%args:code=%
if !run!==1 set args=%args:run=%

::set "args=!args:  = !"

echo args: [!args!]

:: Enable ANSI color codes in console
reg add "HKCU\Console" /v VirtualTerminalLevel /t REG_DWORD /d 1 /f >nul

:main
if !clean!==1 (goto clean)

if %sep%==1 (
	if %res%==1 goto build_res
	if %code%==1 goto build_code
) else (	
	if not %release%==0 goto build
)


@echo.
if !build_result!==1 (
	echo [32mBuild done.[0m	
) else (
    echo [31mBUILD FAILED. STOPPED.[0m
    echo press any key.
    @PAUSE >nul
    exit /b 1
)


:: Launch emulator if requested
if !run! == 1 (
    @echo.
    @echo.
    echo Launching emulator...    
    call "%~dp0run.bat"
)

if !paused! == 1 (
    echo press any key.
	@PAUSE >nul
)

:exit
endlocal
exit /b 0


:clean
call "%smd_dev_path%\devkit\sgdk\sgdk_current\bin\make.exe" -f "%smd_dev_path%\devkit\sgdk\sgdk_current\makefile.gen" "clean"
set clean=0
@echo.
@echo.
echo [32mClean done.[0m
@echo.
@echo.
if !release!==0 (goto exit)    

goto main


:build
echo Building all single makefile...
call "%~dp0timecmd.bat" (call "%smd_dev_path%\devkit\sgdk\sgdk_current\bin\make.exe" -f "%smd_dev_path%\devkit\sgdk\sgdk_current\makefile.gen" !args!) && (set build_result=1)
set release=0
if !build_result!==1 echo [32mBuilding ALL - done (one makefile)[0m
goto main


:build_res
echo Building resources separated...
call "%smd_dev_path%\devkit\sgdk\sgdk_current\bin\make.exe" -f "%smd_dev_path%\devkit\sgdk\sgdk_current\makefile_0.gen" && set build_result=1
set res=0
if !build_result!==1 echo [32mBuilding RES - done (separated makefile)[0m
goto main


:build_code
echo Building code separated makefile...
call "%smd_dev_path%\devkit\sgdk\sgdk_current\bin\make.exe" -f "%smd_dev_path%\devkit\sgdk\sgdk_current\makefile_1.gen" && set build_result=1
set code=0
@echo.
if !build_result!==1 echo [32mBuilding CODE - done (separated makefile)[0m
goto main
