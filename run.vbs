Set WshShell = CreateObject("WScript.Shell")
WshShell.Run "cmd /c build.bat run nopause", 0, True