buildtooloutputdir = "%{cfg.buildcfg}-%{cfg.system}-AnyCPU"

include (path.join( "Saturn-Editor", "content", "Templates", "PremakeCSExtensions.lua"))

workspace "SaturnBuildTool"
	startproject "SaturnBuildTool"
	warnings "Default"
	configurations { "Debug-ASan", "Debug", "Release", "Dist" }

project "SaturnBuildTool"
	language "C#"
	kind "ConsoleApp"
	dotnetframework "net9.0"

	targetdir ("../bin/" .. buildtooloutputdir .. "/%{prj.name}")
	objdir ("../bin-int/" .. buildtooloutputdir .. "/%{prj.name}")

	files
	{
		"src/**.cs"
	}

	postbuildcommands
	{
		'{COPY} "../../../SaturnBuildTool/RT" "RT/"'
	}

	propertytags {
        { "AppendTargetFrameworkToOutputPath", "false" },
        { "Version", "5.1.0" },
        { "Company", "Saturn" }
	}

	nuget { "Microsoft.CodeAnalysis.CSharp:5.9.0" }
	
	filter { "configurations:Debug-ASan" }
		buildcommands
		{
			"dotnet build -c Debug-ASan SaturnBuildTool.sln"
		}

	filter { "configurations:Debug" }
		buildcommands
		{
			"dotnet build -c Debug SaturnBuildTool.sln"
		}

 	filter { "configurations:Release" }
		buildcommands
		{
			"dotnet build -c Release SaturnBuildTool.sln"
		}

	filter { "configurations:Dist" }
		buildcommands
		{
			"dotnet build -c Dist SaturnBuildTool.sln"
		}

