project "Saturn-Editor"
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
		"SAT_HAS_EDITOR",
		"TRACY_ENABLE",
		"TRACY_DELAYED_INIT",
		"TRACY_MANUAL_LIFETIME",
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

	filter { "options:onlineapi=steam" }
		includedirs
		{
			"%{IncludeDir.Steamworks}"
		}

		defines 
		{
			"SAT_WITH_STEAM"
		}

	filter "configurations:Debug-ASan"
		sanitize { "Address" }

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

		filter { "options:onlineapi=steam", "system:windows" }
			postbuildcommands
			{
				'{COPYFILE} "../Saturn/vendor/steamworks/Bin/Windows/steam_api64.dll" "%{cfg.targetdir}"'
			}

		filter { "configurations:Debug or configurations:Debug-ASan" }
			defines "SAT_DEBUG"
			runtime "Debug"
			symbols "on"

			filter { "configurations:Debug or configurations:Debug-ASan", "system:windows" }
				postbuildcommands 
				{ 
					'{COPYFILE} "../Saturn/vendor/assimp/bin/Debug/assimp-vc143-mtd.dll" "%{cfg.targetdir}"',
					'{COPYFILE} "../bin/Debug-windows-x86_64/Saturn-SharedStorage/Saturn-SharedStorage.dll" "%{cfg.targetdir}"',
				}

				links 
				{
					"../Saturn/vendor/libzip/bin/Debug-Windows/libzip.lib"					
				}

		filter "configurations:Release"
			defines "SAT_RELEASE"
			runtime "Release"
		--	optimize "on"

			filter { "configurations:Release", "system:windows" }
				postbuildcommands 
				{ 
					'{COPYFILE} "../bin/Release-windows-x86_64/Saturn-SharedStorage/Saturn-SharedStorage.dll" "%{cfg.targetdir}"',
				}

				links 
				{
					"../Saturn/vendor/libzip/bin/Release-Windows/libzip.lib"					
				}

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
