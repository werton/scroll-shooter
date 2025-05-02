@echo off
setlocal enabledelayedexpansion

set TOTAL_TIMER=%time%

:: Get SMD_DEV_PATH path
call find_smd_dev.bat smd_dev_path
if errorlevel 1 (
    echo Error: smd_dev folder not found!
    pause
    exit /b 1
)

:: Initialize variables
set args=""
set clean=0
set res=0
set code=0
set sep=0
set release=0
set run=0
set paused=1
set build_result=0

:: Process arguments - remove custom flag
set args=%*

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

if "!args!"=="" (set args="release" & set release=1)

if %res%==1 set args=%args:res=%
if %sep%==1 set args=%args:sep=%
if %clean%==1 set args=%args:clean=%
if %code%==1 set args=%args:code=%
if %run%==1 set args=%args:run=%
if %paused%==0 set args=%args:nopause=%

:: Удаляем пробелы в начале
:left_trim
if "!args:~0,1!"==" " (
    set args=!args:~1!
    goto left_trim
)

:: Удаляем пробелы в конце
:right_trim
if "!args:~-1!"==" " (
    set args=!args:~0,-1!
    goto right_trim
)

echo args: [!args!]


:: Enable ANSI color codes in console
reg add "HKCU\Console" /v VirtualTerminalLevel /t REG_DWORD /d 1 /f >nul


:main
::call %~dp0timer.bat PASS_TIMER begin
set PASS_TIMER=%time%

if !clean!==1 (goto clean)

if %sep%==1 (
	if %res%==1 goto build_res
	if %code%==1 goto build_code
) else (	
	if not %release%==0 (goto build_single) else (goto run)
)


@echo.
if !build_result!==1 (
	echo [32mBuild done.[0m
	call "%~dp0timer.bat" TOTAL_TIMER stop	"TOTAL TIME"	
) else (
    echo [31mBUILD FAILED. STOPPED.[0m
	call "%~dp0timer.bat" TOTAL_TIMER stop	"TOTAL TIME"	    	
    echo press any key.
    @PAUSE >nul
    exit /b 1
)

:run
:: Launch emulator if requested
if !run! == 1 (
    @echo.
    @echo.
    echo Launching emulator...    
    call "%smd_dev_path%\devkit\emuls\run_current_emul.bat" "%~dp0out\rom.bin"
)

if !paused! == 1 (
    echo press any key.
	@PAUSE >nul
)

:exit
endlocal
exit /b 0


:clean
:: delete .d .rs files in res folder
call "%~dp0clean_files.bat" "%~dp0res" d rs
:: clean target
call "%smd_dev_path%\devkit\sgdk\sgdk_current\bin\make.exe" -f "%smd_dev_path%\devkit\sgdk\sgdk_current\makefile.gen" "clean"
set clean=0
@echo.
echo [32mClean done.[0m	
@echo.
if !release!==0 (goto exit)    
goto main


:build_single
echo Building all single makefile...
(call "%smd_dev_path%\devkit\sgdk\sgdk_current\bin\make.exe" -f "%smd_dev_path%\devkit\sgdk\sgdk_current\makefile.gen" !args!) && (set build_result=1)
set release=0
if !build_result!==1 echo [32mBuilding ALL - done (one makefile)[0m
call "%~dp0timer.bat" PASS_TIMER stop	"BUILD TIME"		
goto main


:build_res
echo Building resources separated...
(call "%smd_dev_path%\devkit\sgdk\sgdk_current\bin\make.exe" -f "%smd_dev_path%\devkit\sgdk\sgdk_current\makefile_0.gen" !args!) && (set build_result=1)
set res=0
if !build_result!==1 echo [32mBuilding RES - done (separated makefile)[0m
call "%~dp0timer.bat" PASS_TIMER stop	"Resources build time"	
goto main


:build_code
echo Building code separated makefile...
(call "%smd_dev_path%\devkit\sgdk\sgdk_current\bin\make.exe" -f "%smd_dev_path%\devkit\sgdk\sgdk_current\makefile_1.gen" !args!) && (set build_result=1)
set code=0
if !build_result!==1 echo [32mBuilding CODE - done (separated makefile)[0m
call "%~dp0timer.bat" PASS_TIMER stop	"Code build time"	
goto main
