@echo off
color 0A
echo ========================================================
echo   LIMPIANDO MEMORIA Y COMPILANDO HELBREATH (RELEASE)
echo ========================================================
echo.

call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64

echo.
echo [1/2] Limpiando archivos viejos y basura de compilacion...
msbuild "d:\HB Server\Helbreath-Heldenian-Project-Development\Sources\Helbreath.sln" /p:Configuration=Release /p:Platform=x64 /t:Clean

echo.
echo [2/2] Compilando el codigo desde cero...
msbuild "d:\HB Server\Helbreath-Heldenian-Project-Development\Sources\Helbreath.sln" /p:Configuration=Release /p:Platform=x64 /t:Build

echo.
echo Proceso terminado. Revisa si hay 0 errores arriba.
pause