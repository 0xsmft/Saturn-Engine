-- Game Project premake template.
-- __PROJECT_NAME__

workspace "__PROJECT_NAME__"
	architecture "x64"
	startproject "__PROJECT_NAME__"

	configurations { "Debug", "Release", "Dist" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}

SaturnDir = os.getenv('SATURN_DIR')

group "Game"
project "__PROJECT_NAME__"
	kind "Makefile"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"Source/**.cpp",
		"Source/**.h",
		"Source/**.cs"
	}

	removefiles 
	{ 
		"**.Gen.cpp", 
		"**.Gen.h" 
	}
	
	filter "system:windows"
		systemversion "latest"

		filter "configurations:Debug"
			runtime "Debug"
			symbols "on"

			debugcommand { SaturnDir .. "/bin/Debug-windows-x86_64/Saturn-Editor/Saturn-Editor.exe" }
			debugargs    { "%{prj.location}/%{prj.name}.sproject" }
			debugdir     { SaturnDir .. "/Saturn-Editor" }

			buildcommands
			{
				"__SATURN_BT_DIR__/RT/Run.bat /BUILD /NAME:%{prj.name} /Win64 /Debug /PROJECT:%{prj.location}"
			}

			rebuildcommands 
			{
				"__SATURN_BT_DIR__/RT/Run.bat /REBUILD /NAME:%{prj.name} /Win64 /Debug /PROJECT:%{prj.location}"
			}

			cleancommands
			{
				"__SATURN_BT_DIR__/RT/Run.bat /CLEAN /NAME:%{prj.name} /Win64 /Debug /PROJECT:%{prj.location}"
			}

		filter "configurations:Release"
			runtime "Release"
			optimize "on"

			debugcommand { SaturnDir .. "/bin/Release-windows-x86_64/Saturn-Editor/Saturn-Editor.exe" }
			debugargs    { "%{prj.location}/%{prj.name}.sproject" }
			debugdir     { SaturnDir .. "/Saturn-Editor" }

			buildcommands
			{
				"__SATURN_BT_DIR__/RT/Run.bat /BUILD /NAME:%{prj.name} /Win64 /Release /PROJECT:%{prj.location}"
			}

			rebuildcommands 
			{
				"__SATURN_BT_DIR__/RT/Run.bat /REBUILD /NAME:%{prj.name} /Win64 /Release /PROJECT:%{prj.location}"
			}

			cleancommands
			{
				"__SATURN_BT_DIR__/RT/Run.bat /CLEAN /NAME:%{prj.name} /Win64 /Release /PROJECT:%{prj.location}"
			}

		filter "configurations:Dist"
			runtime "Release"
			symbols "on"

			buildcommands
			{
				"__SATURN_BT_DIR__/RT/Run.bat /BUILD /NAME:%{prj.name} /Win64 /Dist /PROJECT:%{prj.location}"
			}

			rebuildcommands 
			{
				"__SATURN_BT_DIR__/RT/Run.bat /REBUILD /NAME:%{prj.name} /Win64 /Dist /PROJECT:%{prj.location}"
			}

			cleancommands
			{
				"__SATURN_BT_DIR__/RT/Run.bat /CLEAN /NAME:%{prj.name} /Win64 /Dist /PROJECT:%{prj.location}"
			}