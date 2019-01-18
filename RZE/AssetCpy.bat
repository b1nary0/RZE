@echo off
REM using crappy stuff here until a better build pipeline exists -- this serves its purpose atm

set debugExecPath="_Build/Debug/x64/"
set debugAssetPath="_Build/Debug/x64/Assets"
set debugConfigPath="_Build/Debug/x64/Config"
set releaseExecPath="_Build/Release/x64/"
set releaseAssetPath="_Build\Release\x64\Assets"
set releaseConfigPath="_Build\Release\x64\Config"

if exist %debugExecPath% (
	if not exist %debugAssetPath% (mkdir %debugAssetPath%)
	if not exist %debugConfigPath% (mkdir %debugConfigPath%)
	
	xcopy /y /d /s /f Assets %debugAssetPath%
	xcopy /y /d /s /f Config %debugConfigPath%
)
if exist %releaseExecPath% (
	if not exist %releaseAssetPath% (mkdir %releaseAssetPath%)
	if not exist %releaseConfigPath% (mkdir %releaseConfigPath%)
	
	xcopy /y /d /s /f Assets %releaseAssetPath%
	xcopy /y /d /s /f Config %releaseConfigPath%
)
