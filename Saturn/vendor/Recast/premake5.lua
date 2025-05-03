project "Recast"
	kind "StaticLib"
	language "C++"
	cppdialect "C++23"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"Recast/Include/*.h",
		"Recast/Source/*.cpp"
	}

    includedirs
    {
        "Recast/Include"
    }

	filter "system:windows"
		systemversion "latest"
		staticruntime "On"

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

project "Detour"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"
    
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
    
    files
    {
        "Detour/Include/*.h",
        "Detour/Source/*.cpp"
    }
    
    includedirs
    {
        "Detour/Include"
    }

    filter "system:windows"
        systemversion "latest"
        staticruntime "On"
    
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