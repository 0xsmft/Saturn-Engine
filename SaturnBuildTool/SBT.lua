project "SaturnBuildTool"
	language "C#"
	kind "ConsoleApp"
	links { "System" }
	defines { "SAT_BUILDTOOOL_NETFRAMEWORK" }

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.cs"
	}

	postbuildcommands
	{
		'{COPY} "../../../SaturnBuildTool/RT" "RT/"'
	}

	filter { "configurations:Debug" }
		symbols "On"

 	filter { "configurations:Release" }
		optimize "On"
   		symbols "On"

	filter { "configurations:Dist" }
		optimize "On"
  		symbols "Off"
