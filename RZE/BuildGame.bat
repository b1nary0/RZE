@echo off
REM Finds MSBuild through vswhere, so this works outside a Visual Studio developer prompt
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -find MSBuild\**\Bin\MSBuild.exe`) do set msbuildPath=%%i

if not defined msbuildPath (
	echo MSBuild not found. Install Visual Studio with the C++ workload.
	exit /b 1
)

"%msbuildPath%" "%~dp0Game\_project\Game_Win64.vcxproj" -property:Configuration=Debug -property:Platform=x64 -verbosity:minimal -nologo
