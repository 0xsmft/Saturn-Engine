project "Tracy"
	kind "StaticLib"
	language "C++"
	cppdialect "C++23"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/tracy/TracyClient.cpp"
	}

	links 
	{
		"ws2_32",
		"dbghelp"
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
		staticruntime "On"

		defines 
		{
			"TRACY_NO_CRASH_HANDLER"
		}

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