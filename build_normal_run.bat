
"%~dp0..\..\devkit\sgdk\sgdk_current\bin\make.exe" -f "%~dp0..\..\devkit\sgdk\sgdk_current\makefile.gen" %*


start /b %~dp0..\..\devkit\emuls\run_current_emul.bat %~dp0\out\rom.bin

