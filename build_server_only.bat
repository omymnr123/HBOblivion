@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64
msbuild "d:\HB Server\Helbreath-Heldenian-Project-Development\Sources\Server\Server.vcxproj" /p:Configuration=Release /p:Platform=x64
