@echo off
START /B /WAIT %~dp0Make/Premake/premake5.exe --file=%~dp0Make/Premake/premake5.lua vs2017 > output.txt