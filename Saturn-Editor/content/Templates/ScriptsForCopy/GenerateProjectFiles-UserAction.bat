@echo off
pushd %~dp0\..\
call premake5.exe %*
popd
