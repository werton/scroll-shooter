@echo off

set build_result=0

call %~dp0timecmd.bat (call "%~dp0sep_rebuild_all.bat" nopause)


CALL "%~dp0run.bat"
@PAUSE >nul

