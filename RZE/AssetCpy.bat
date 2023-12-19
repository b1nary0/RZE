@echo off
REM using crappy stuff here until a better build pipeline exists -- this serves its purpose atm

set debugExecPath=%~dp0_Build\Debug\
set debugAssetPath=%~dp0_Build\Debug\Assets
set debugConfigPath=%~dp0_Build\Debug\Config
set debugProjectDataPath=%~dp0_Build\Debug\ProjectData
set releaseExecPath=%~dp0_Build\Release\
set releaseAssetPath=%~dp0_Build\Release\Assets
set releaseConfigPath=%~dp0_Build\Release\Config
set releaseProjectDataPath=%~dp0_Build\Release\ProjectData

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