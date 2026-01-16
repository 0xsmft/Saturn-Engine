-- Game Project premake template.
-- __PROJECT_NAME__

workspace "__PROJECT_NAME__"
	architecture "x64"
	startproject "__PROJECT_NAME__"

	configurations { "Debug", "Release", "Dist" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}

-- NOTE SATURN_DIR environment variable always points to the root dir of Saturn
local SaturnDir = os.getenv('SATURN_DIR')
-- Now, replace the "//" with "/"
SaturnDir = SaturnDir:gsub( "\\", "/" )

-- NOTE: This acts as the Saturn Engine module!
group "Engine"
project "Saturn"
	kind "Makefile"
	language "C++"
	cppdialect "C++23"
	staticruntime "on"
	location "Build"

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
	location "Build"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{wks.location}/Source/**.h",
		"%{wks.location}/Source/**.cpp",
		"%{wks.location}/Source/**.cs"
	}

	removefiles 
	{ 
		"Generated/**.Gen.cpp", 
		"Generated/**.Gen.h" 
	}
	
	filter "system:windows"
		systemversion "latest"

		filter "configurations:Debug"
			runtime "Debug"
			symbols "on"

			debugcommand ( SaturnDir .. "/bin/Debug-windows-x86_64/Saturn-Editor/Saturn-Editor.exe" )
			debugargs { "%{wks.location}/%{prj.name}.sproject" }
			debugdir ( SaturnDir .. "/Saturn-Editor" )

			buildcommands
			{
				SaturnDir .. "/bin/Debug-windows-x86_64/SaturnBuildTool/SaturnBuildTool.exe /BUILD /NAME:%{prj.name} /Win64 /Debug /PROJECT:%{wks.location}"
			}

			rebuildcommands 
			{
				SaturnDir .. "/bin/Debug-windows-x86_64/SaturnBuildTool/SaturnBuildTool.exe /REBUILD /NAME:%{prj.name} /Win64 /Debug /PROJECT:%{wks.location}"
			}

			cleancommands
			{
				SaturnDir .. "/bin/Debug-windows-x86_64/SaturnBuildTool/SaturnBuildTool.exe /CLEAN /NAME:%{prj.name} /Win64 /Debug /PROJECT:%{wks.location}"
			}

		filter "configurations:Release"
			runtime "Release"
			optimize "on"

			debugcommand ( SaturnDir .. "/bin/Release-windows-x86_64/Saturn-Editor/Saturn-Editor.exe" )
			debugargs    { "%{wks.location}/%{prj.name}.sproject" }
			debugdir     ( SaturnDir .. "/Saturn-Editor" )

			buildcommands
			{
				SaturnDir .. "/bin/Release-windows-x86_64/SaturnBuildTool/SaturnBuildTool.exe /BUILD /NAME:%{prj.name} /Win64 /Release /PROJECT:%{wks.location}"
			}

			rebuildcommands 
			{
				SaturnDir .. "/bin/Release-windows-x86_64/SaturnBuildTool/SaturnBuildTool.exe /REBUILD /NAME:%{prj.name} /Win64 /Release /PROJECT:%{wks.location}"
			}

			cleancommands
			{
				SaturnDir .. "/bin/Release-windows-x86_64/SaturnBuildTool/SaturnBuildTool.exe /CLEAN /NAME:%{prj.name} /Win64 /Release /PROJECT:%{wks.location}"
			}

		filter "configurations:Dist"
			runtime "Release"
			symbols "on"

			buildcommands
			{
				SaturnDir .. "/bin/Dist-windows-x86_64/SaturnBuildTool/SaturnBuildTool.exe /BUILD /NAME:%{prj.name} /Win64 /Dist /PROJECT:%{wks.location}"
			}

			rebuildcommands 
			{
				SaturnDir .. "/bin/Dist-windows-x86_64/SaturnBuildTool/SaturnBuildTool.exe /REBUILD /NAME:%{prj.name} /Win64 /Dist /PROJECT:%{wks.location}"
			}

			cleancommands
			{
				SaturnDir .. "/bin/Dist-windows-x86_64/SaturnBuildTool/SaturnBuildTool.exe /CLEAN /NAME:%{prj.name} /Win64 /Dist /PROJECT:%{wks.location}"
			}
