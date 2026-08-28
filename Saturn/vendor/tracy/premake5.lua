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

	filter "system:windows"
		systemversion "latest"
		staticruntime "off"

		defines 
		{
			"_CRT_SECURE_NO_WARNINGS"
		}

		links 
		{
			"ws2_32",
			"dbghelp"
		}

	filter "system:linux"
		systemversion "latest"
		staticruntime "off"

		defines 
		{
			"TRACY_NO_CRASH_HANDLER"
		}

		files 
		{
			"src/libbacktrace/**.cpp",
			"src/libbacktrace/**.hpp"
		}

		includedirs 
		{
			"src/libbacktrace"
		}

	filter "configurations:Debug or configurations:Debug-ASan"
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