@echo off
cd /d %~dp0
title Subir cambios a GitHub - HBOblivion
color 0A
echo ========================================
echo    SUBIENDO CAMBIOS A GITHUB...
echo ========================================
echo.

echo [1/3] Preparando los archivos...
git add .

echo.
set /p mensaje="Escribe una breve descripcion de lo que cambiaste y dale a Enter: "
if "%mensaje%"=="" set mensaje=Actualizacion automatica

echo.
echo [2/3] Guardando los cambios...
git commit -m "%mensaje%"

echo.
echo [3/3] Subiendo a internet (GitHub)...
git push

echo.
echo ========================================
echo    ¡TODO LISTO Y SUBIDO CON EXITO!
echo ========================================
pause