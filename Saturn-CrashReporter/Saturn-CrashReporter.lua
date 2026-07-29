project "Saturn-CrashReporter"
	location ""
	language "C++"
	cppdialect "C++23"
	staticruntime "off"
	warnings "Default"
	kind "ConsoleApp"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "sppch.h"
	pchsource "../Saturn/src/sppch.cpp"

	defines
	{
		"TRACY_ENABLE",
		"TRACY_DELAYED_INIT",
		"TRACY_MANUAL_LIFETIME",
		-- Disable crash reporting for the crash reporter.
		"SAT_WITH_CRASHCATCH=0"
	}

	files
	{
		"src/**.h",
		"src/**.cpp",

		"../Saturn/src/sppch.cpp"
	}

	includedirs
	{
		"../Saturn/vendor/spdlog/include",
		"../Saturn/src",
		"../Saturn/vendor",
		"../Saturn/vendor/vulkan/include",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.glslc}",
		"%{IncludeDir.shaderc}",
		"%{IncludeDir.SPIRV_Cross}",
		"%{IncludeDir.vma}",
		"%{IncludeDir.JoltPhys}",
		"%{IncludeDir.Optick}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.ImSpinner}",
		"%{IncludeDir.Filewatch}",
		"%{IncludeDir.MiniAudio}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.ImguiNodeEditor}",
		"%{IncludeDir.Tracy}",
		"%{IncludeDir.KTX_Software}",
		"%{IncludeDir.Recast}",
		"%{IncludeDir.acl}",
		"%{IncludeDir.rtm}",
		"%{IncludeDir.freetype}",
		"%{IncludeDir.MSDF}",
		"%{IncludeDir.MSDFAG}",
		"%{IncludeDir.ImTimeline}",
		"%{IncludeDir.libzip}",
		"%{IncludeDir.ImGuiColorTextEdit}",
		"%{IncludeDir.CrashCatch}",

		"%{IncludeDir.SharedStorage}"
	}

	links
	{
		"Saturn"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"SAT_PLATFORM_WINDOWS",
			"SATURN_SS_IMPORT",
			"_CRT_SECURE_NO_WARNINGS",
		}

		files 
		{
			"../Saturn/visualisers/*.natvis",
			"../Saturn/src/Saturn/Entry/Windows/**.cpp",
		}

		filter "configurations:Debug"
			defines "SAT_DEBUG"
			runtime "Debug"
			symbols "on"

			filter { "configurations:Debug", "system:windows" }
				postbuildcommands 
				{ 
					'{COPYFILE} "../Saturn/vendor/assimp/bin/Debug/assimp-vc143-mtd.dll" "%{cfg.targetdir}"',
					'{COPYFILE} "../bin/Debug-windows-x86_64/Saturn-SharedStorage/Saturn-SharedStorage.dll" "%{cfg.targetdir}"',
				}

		filter "configurations:Release"
			defines "SAT_RELEASE"
			runtime "Release"
			optimize "on"
			symbols "Off"
			kind "WindowedApp"
			defines { "SAT_WITH_VALIDATION_LAYERS=0" }
			
			filter { "configurations:Release", "system:windows" }
			postbuildcommands
			{ 
				'{COPYFILE} "../Saturn/vendor/assimp/bin/Release/assimp-vc143-mt.dll" "%{cfg.targetdir}"',
				'{COPYFILE} "../bin/Release-windows-x86_64/Saturn-SharedStorage/Saturn-SharedStorage.dll" "%{cfg.targetdir}"',
			}
		
			defines { "SAT_WINDOWS_USE_WINMAIN" }

		filter "configurations:Dist"
			defines "SAT_DIST"
			runtime "Release"
			optimize "on"
			symbols "Off"
			kind "WindowedApp"

			removedefines { "SATURN_SS_IMPORT" }
			defines { "SATURN_SS_STATIC" }

	filter "system:linux"
		systemversion "latest"

		defines
		{
			"SAT_PLATFORM_LINUX"
		}

		files 
		{
			"../Saturn/src/Saturn/Entry/Unix/**.cpp",
		}

		filter "configurations:Debug"
			defines "SAT_DEBUG"
			runtime "Debug"
			symbols "on"

		filter "configurations:Release"
			defines "SAT_RELEASE"
			runtime "Release"
			optimize "on"

		filter "configurations:Dist"
			defines "SAT_DIST"
			runtime "Release"
			optimize "on"

	filter "system:Mac"
		systemversion "latest"

		defines
		{
			"SAT_PLATFORM_MACOS"
		}

		files 
		{
		}

		filter "configurations:Debug"
			defines "SAT_DEBUG"
			runtime "Debug"
			symbols "on"

		filter "configurations:Release"
			defines "SAT_RELEASE"
			runtime "Release"
			optimize "on"

		filter "configurations:Dist"
			defines "SAT_DIST"
			runtime "Release"
			optimize "on"
