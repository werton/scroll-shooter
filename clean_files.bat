@echo off
setlocal enabledelayedexpansion

:: Check for minimum arguments
if "%~1"=="" (
    echo Usage: %~nx0 [folder] [ext1] [ext2] ...
    echo Example: %~nx0 "C:\Temp" tmp log bak
    exit /b 1
)

:: Set target folder
set "target_folder=%~1"
shift /1

:: Verify folder exists
if not exist "%target_folder%\" (
    echo Error: Folder "%target_folder%" does not exist
    exit /b 1
)

:: Initialize counters
set /a total_files_found=0
set /a total_files_deleted=0

:: Process each extension
:process_extensions
if "%~1"=="" goto extensions_done

set "current_ext=%~1"

:: Actually delete the files
for /r "%target_folder%" %%f in (*.%current_ext%) do (
    del /f /q "%%f" >nul 2>&1
)


shift /1
goto process_extensions

:extensions_done
endlocal