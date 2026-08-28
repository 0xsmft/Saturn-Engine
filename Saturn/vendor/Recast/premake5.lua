project "Recast"
	kind "StaticLib"
	language "C++"
	cppdialect "C++23"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"RecastAndDetour/Include/**.h",
		"RecastAndDetour/Source/**.cpp"
	}

    includedirs
    {
        "RecastAndDetour/Include/RecastShared",
        "RecastAndDetour/Include/Detour",
        "RecastAndDetour/Include/Recast"
    }

	filter "system:windows"
		systemversion "latest"
		staticruntime "off"

	filter "system:linux"
		pic "On"
		systemversion "latest"
		cppdialect "C++2a"
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