project "MSDFGen"
	kind "StaticLib"
	language "C++"
	cppdialect "C++23"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"core/**.h",
		"core/**.hpp",
		"core/**.cpp",
		"ext/**.h",
		"ext/**.hpp",
		"ext/**.cpp",
		"lib/**.cpp",
		"include/**.h"
    }

    defines 
    {
        "MSDFGEN_USE_CPP11"   
    }

    includedirs 
    {
        "core",
        "include",
        "../../Freetype/include"
    }

    links 
    {
        "Freetype"
    }

	filter "system:windows"
		systemversion "latest"
		staticruntime "On"

        defines
	    {
		    "_CRT_SECURE_NO_WARNINGS"
	    }

	filter "system:linux"
		pic "On"
		systemversion "latest"
		cppdialect "C++2a"
		staticruntime "On"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		runtime "Release"
		optimize "on"
		symbols "off"