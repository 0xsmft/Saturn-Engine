@echo off

rem %1 is the buildtool location 

pushd "%~dp0"

IF NOT EXIST "..\SaturnBuildTool.exe" (
	ECHO BUIDLTOOLS NOT FOUND!
	popd
	exit /B 1
)

"..\SaturnBuildTool.exe" %*
set ERRNO=%ERRORLEVEL%

popd

rem An error code of 2 is not a failure, it means that the BuildTool has nothing todo. However MSBuild will treat an error code of two as an erorr, so we lie to it.
if %ERRNO%==2 exit /B 0	
exit /B %ERRNO%
