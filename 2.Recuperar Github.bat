@echo off
title Volver a la ultima version de GitHub
color 0C
echo ========================================
echo    ¡ATENCION! ESTO BORRARA TUS CAMBIOS MALOS
echo    Y DEJARA EL PROYECTO IGUAL QUE EN GITHUB.
echo ========================================
echo.
set /p confirmar="¿Estas 100%% seguro? Escribe S y dale a Enter (o cierra la ventana para cancelar): "
if /i not "%confirmar%"=="S" goto cancelar

echo.
echo [1/2] Conectando con GitHub...
git fetch origin

echo.
echo [2/2] Borrando lo malo y restaurando...
git reset --hard origin/main
git clean -fd

echo.
echo ========================================
echo    ¡LISTO! TODO VUELVE A COMO ESTABA EN GITHUB
echo ========================================
goto fin

:cancelar
echo.
echo Operacion cancelada. Tus cambios siguen intactos.

:fin
echo.
pause