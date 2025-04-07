
@ECHO OFF
set multi_core_flagset multi_core_flag=-j12
setlocal enabledelayedexpansion

set clean=0
set debug_build=0
set run_emul=0

for %%x in (%*) do (
  IF [%%x] == [clean] (set clean=1)
  
  IF [%%x] == [debug] (
	  set debug_build=1
	  set multi_core_flag=) 
	  
  IF [%%x] == [run] (set run_emul=1)
)

set args=%*

:: removing run flag for make and adding -jN flag for make
set args=%args:run=%

::and adding -jN flag for make
set args=%multi_core_flag% %args%



@ECHO ON

echo calling make.exe with args: %args%


if %debug_build% == 0 "%~dp0timecmd.bat" (CALL "%~dp0compile_all.bat" %args%) && (if %run_emul% == 1 (CALL "%~dp0run.bat"))
if %debug_build% == 1 "%~dp0timecmd.bat" (CALL "%~dp0compile_all_debug.bat" %args%) && (if %run_emul% == 1 (CALL "%~dp0run.bat"))




