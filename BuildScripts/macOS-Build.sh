#!/bin/sh

# SBT

dotnet build -c Debug SaturnBuildTool/SaturnBuildTool.sln

# Saturn
make -j
