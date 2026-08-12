@echo off
cd /d %~dp0
title Subir cambios a GitHub - HBOblivion
color 0A
echo ========================================
echo    SUBIENDO CAMBIOS A GITHUB...
echo ========================================
echo.

echo [1/4] Configurando exclusiones de directorios...
:: Añadir a .gitignore si no existen
findstr /C:"Binaries/Game Launcher Lan/" .gitignore >nul 2>&1
if errorlevel 1 echo Binaries/Game Launcher Lan/>> .gitignore

findstr /C:"Binaries/Game Lan/" .gitignore >nul 2>&1
if errorlevel 1 echo Binaries/Game Lan/>> .gitignore

:: Dejar de rastrear los archivos si ya se habian subido antes (esto NO borra tus archivos locales)
git rm -r --cached "Binaries/Game Launcher Lan" >nul 2>&1
git rm -r --cached "Binaries/Game Lan" >nul 2>&1

echo.
echo [2/4] Preparando los archivos...
git add .

echo.
set /p mensaje="Escribe una breve descripcion de lo que cambiaste y dale a Enter: "
if "%mensaje%"=="" set mensaje=Actualizacion automatica

echo.
echo [3/4] Guardando los cambios...
git commit -m "%mensaje%"

echo.
echo [4/4] Subiendo a internet (GitHub)...
git push

echo.
echo ========================================
echo    ¡TODO LISTO Y SUBIDO CON EXITO!
echo ========================================
echo Ultimo commit registrado:
git log -1 --oneline
echo.
pause