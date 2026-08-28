include "msdfgen"

project "MSDF-Atlas-Gen"
	kind "StaticLib"
	language "C++"
	cppdialect "C++23"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"msdf-atlas-gen/*.h",
		"msdf-atlas-gen/*.cpp",
		"msdf-atlas-gen/*.hpp"
	}

    includedirs 
    {
        "msdf-atlas-gen",
        "msdfgen",
        "msdfgen/include"
    }

	filter "system:windows"
		systemversion "latest"
		staticruntime "off"

        defines
	    {
		    "_CRT_SECURE_NO_WARNINGS"
	    }

	filter "system:linux"
		pic "On"
		systemversion "latest"
		staticruntime "off"

	filter "configurations:Debug or configurations:Debug-ASan"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		runtime "Release"
		optimize "on"
		symbols "off"