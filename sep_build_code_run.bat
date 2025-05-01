set build_result=0

:: Run build via Make. If return code is not 0, show error message and exit
call %~dp0timecmd.bat (call %~dp0sep_build_code.bat -j)

if %errorlevel% equ 0 (set build_result=1)


CALL "%~dp0run.bat"
@PAUSE >nul
