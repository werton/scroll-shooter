start /b %~dp0copy_res_src.bat

"%~dp0..\..\devkit\sgdk\sgdk_current\bin\make.exe" -j8 -f "%~dp0..\..\devkit\sgdk\sgdk_current\makefile.gen" %*

pause

start /b %~dp0..\..\devkit\emuls\run_current_emul.bat %~dp0\out\rom.bin


