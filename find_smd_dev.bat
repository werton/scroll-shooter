@echo off
setlocal enabledelayedexpansion

:: Проверка, что передан аргумент (имя переменной для результата)
if "%~1"=="" (
    echo Usage: %0 ^<output_variable_name^>
    exit /b 1
)

:: Получаем текущий путь и убираем завершающий слеш
set "current_path=%~dp0"
if "%current_path:~-1%"=="\" set "current_path=%current_path:~0,-1%"

:: Ищем папку smd_dev
set "smd_dev_path="
:loop
for %%i in ("%current_path%") do (
    set "folder=%%~nxi"
    if /i "!folder!"=="smd_dev" (
        set "smd_dev_path=!current_path!"
        goto found
    )
)

:: Переходим на уровень выше
for %%i in ("%current_path%") do set "parent=%%~dpi"
if "%parent%"=="%current_path%" goto not_found
set "current_path=%parent%"
if "%current_path:~-1%"=="\" set "current_path=%current_path:~0,-1%"
goto loop

:not_found
echo Error: smd_dev folder not found!
endlocal
exit /b 1

:found
:: Записываем результат в переменную, имя которой передано в %1
endlocal & set "%~1=%smd_dev_path%"
exit /b 0

