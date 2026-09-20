#!/bin/sh

# BuildTool
premake5 vs2022 --file=SaturnBuildTool/SBT-ForNonWindows.lua

# Saturn
premake5 gmake2 --cc=clang --file=premake5-AArch.lua