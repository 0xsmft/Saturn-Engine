project "Tracy"
	kind "StaticLib"
	language "C++"
	cppdialect "C++23"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	defines 
	{
		"_CRT_SECURE_NO_WARNINGS"
	}

	links 
	{
		"ws2_32",
		"dbghelp"
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
		defines { "TRACY_ENABLE", "TRACY_MANUAL_LIFETIME", "TRACY_DELAYED_INIT" }

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
		defines { "TRACY_ENABLE", "TRACY_MANUAL_LIFETIME", "TRACY_DELAYED_INIT" }

	filter "configurations:Dist"
		runtime "Release"
		optimize "on"
		symbols "off"