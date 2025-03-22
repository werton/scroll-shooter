"C:\Dev\smd_dev\devkit\sgdk\sgdk_current\bin\make.exe" -f "C:\Dev\smd_dev\devkit\sgdk\sgdk_current\makefile.gen" %1 %2 
start /b C:\Dev\smd_dev\devkit\emuls\run_current_emul.bat %~dp0\out\rom.bin
ping 127.0.0.1 -n 2 > nul
