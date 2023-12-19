@echo off
REM using crappy stuff here until a better build pipeline exists -- this serves its purpose atm

set debugExecPath=%~dp0_build\Debug\
set debugAssetPath=%~dp0_build\Debug\Assets
set debugConfigPath=%~dp0_build\Debug\Config
set debugProjectDataPath=%~dp0_build\Debug\ProjectData
set releaseExecPath=%~dp0_build\Release\
set releaseAssetPath=%~dp0_build\Release\Assets
set releaseConfigPath=%~dp0_build\Release\Config
set releaseProjectDataPath=%~dp0_build\Release\ProjectData

if exist %debugExecPath% (
	if not exist %debugAssetPath% (mkdir %debugAssetPath%)
	if not exist %debugConfigPath% (mkdir %debugConfigPath%)
	if not exist %debugProjectDataPath% (mkdir %debugProjectDataPath%)
	
	xcopy /y /d /s /f %~dp0Assets %debugAssetPath%
	xcopy /y /d /s /f %~dp0Config %debugConfigPath%
	xcopy /y /d /s /f %~dp0ProjectData %debugProjectDataPath%
)
if exist %releaseExecPath% (
	if not exist %releaseAssetPath% (mkdir %releaseAssetPath%)
	if not exist %releaseConfigPath% (mkdir %releaseConfigPath%)
	if not exist %releaseProjectDataPath% (mkdir %releaseProjectDataPath%)
	
	xcopy /y /d /s /f %~dp0Assets %releaseAssetPath%
	xcopy /y /d /s /f %~dp0Config %releaseConfigPath%
	xcopy /y /d /s /f %~dp0ProjectData %releaseProjectDataPath%
)