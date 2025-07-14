-- Game Project premake template.

workspace "__PROJECT_NAME__"
	architecture "x64"
	startproject "__PROJECT_NAME__"

	configurations { "Debug", "Release", "Dist" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}

-- NOTE SATURN_DIR environment variable always points to the root dir of Saturn
local SaturnDir = os.getenv('SATURN_DIR')
SaturnDir = SaturnDir:gsub( "\\", "/" )

-- Engine
group "Engine"
project "Saturn"
	kind "Makefile"
	language "C++"
	cppdialect "C++23"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	debugcommand ( "" )
	buildcommands   ( "@rem nothing to build" )
	rebuildcommands ( "@rem nothing to build" )
	cleancommands   ( "@rem nothing to build" )

	files 
	{
		SaturnDir .. "/Saturn/src/**.cpp",
		SaturnDir .. "/Saturn/src/**.h",
		SaturnDir .. "/Saturn/src/**.cs",
	}

-- __PROJECT_NAME__
group "Game"
project "__PROJECT_NAME__"
	kind "Makefile"
	language "C++"
	cppdialect "C++23"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"Source/**.h",
		"Source/**.cpp",
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

			debugcommand ( SaturnDir .. "/bin/Release-windows-x86_64/Saturn-Editor/Saturn-Editor.exe" )
			debugargs { "%{prj.location}/%{prj.name}.sproject" }
			debugdir ( SaturnDir .. "/Saturn-Editor" )

			buildcommands
			{
				"__SATURN_BT_DIR__Debug/RT/Run.bat /BUILD /NAME:%{prj.name} /Win64 /Debug /PROJECT:%{prj.location}"
			}

			rebuildcommands 
			{
				"__SATURN_BT_DIR__Debug/RT/Run.bat /REBUILD /NAME:%{prj.name} /Win64 /Debug /PROJECT:%{prj.location}"
			}

			cleancommands
			{
				"__SATURN_BT_DIR__Debug/RT/Run.bat /CLEAN /NAME:%{prj.name} /Win64 /Debug /PROJECT:%{prj.location}"
			}

		filter "configurations:Release"
			runtime "Release"
			optimize "on"

			debugcommand ( SaturnDir .. "/bin/Release-windows-x86_64/Saturn-Editor/Saturn-Editor.exe" )
			debugargs    { "%{prj.location}/%{prj.name}.sproject" }
			debugdir     ( SaturnDir .. "/Saturn-Editor" )

			buildcommands
			{
				"__SATURN_BT_DIR__/SaturnBuildTool/RT/Run.bat /BUILD /NAME:%{prj.name} /Win64 /Release /PROJECT:%{prj.location}"
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
				"__SATURN_BT_DIR__RT/Run.bat /BUILD /NAME:%{prj.name} /Win64 /Dist /PROJECT:%{prj.location}"
			}

			rebuildcommands 
			{
				"__SATURN_BT_DIR__RT/Run.bat /REBUILD /NAME:%{prj.name} /Win64 /Dist /PROJECT:%{prj.location}"
			}

			cleancommands
			{
				"__SATURN_BT_DIR__RT/Run.bat /CLEAN /NAME:%{prj.name} /Win64 /Dist /PROJECT:%{prj.location}"
			}