buildtooloutputdir = "%{cfg.buildcfg}-%{cfg.system}-AnyCPU"

include (path.join( "Saturn-Editor", "content", "Templates", "PremakeCSExtensions.lua"))

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

	filter { "configurations:Debug" }
		symbols "On"

 	filter { "configurations:Release" }
		optimize "On"
   		symbols "On"

	filter { "configurations:Dist" }
		optimize "On"
  		symbols "Off"
