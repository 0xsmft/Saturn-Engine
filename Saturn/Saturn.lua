project "Saturn"
	location ""
	kind "StaticLib"
	language "C++"
	cppdialect "C++23"
	staticruntime "off"
	warnings "Default"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "sppch.h"
	pchsource "src/sppch.cpp"

	files
	{
		"src/**.h",
		"src/**.cpp",
		"vendor/stb/**.cpp",
		"vendor/stb/**.h",	
		"vendor/vma/src/**.cpp",
		"vendor/vma/src/**.h",
		"vendor/vulkan/**.h",
		"vendor/glm/glm/**.hpp",
		"vendor/glm/glm/**.inl",
		"vendor/ImGuizmo/src/**.cpp",
		"vendor/ImGuizmo/src/**.h",
	}

	removefiles 
	{
		"src/%{prj.name}/Entry/macOS/**.cpp",
		"src/%{prj.name}/Entry/Unix/**.cpp",
		"src/%{prj.name}/Entry/Windows/**.cpp",
	}

	defines
	{
		"GLM_ENABLE_EXPERIMENTAL",
		"SATURN_SS_IMPORT",
		"TRACY_ENABLE",
		"TRACY_DELAYED_INIT",
		"TRACY_MANUAL_LIFETIME",
		"SAT_RBY_INCLUDE_VULKAN"
	}

	includedirs
	{
		"src",
		"vendor/stb",
		"vendor/spdlog/include",
		"vendor/vulkan/include",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.glslc}",
		"%{IncludeDir.shaderc}",
		"%{IncludeDir.SPIRV_Cross}",
		"%{IncludeDir.vma}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.ImguiNodeEditor}",
		"%{IncludeDir.ImSpinner}",
		"%{IncludeDir.Tracy}",
		"%{IncludeDir.MiniAudio}",
		"%{IncludeDir.Filewatch}",
		"%{IncludeDir.zlib}",
		"%{IncludeDir.JoltPhys}",
		"%{IncludeDir.KTX_Software}",
		"%{IncludeDir.Recast}",
		"%{IncludeDir.acl}",
		"%{IncludeDir.rtm}",
		"%{IncludeDir.freetype}",
		"%{IncludeDir.MSDF}",
		"%{IncludeDir.MSDFAG}",
		"%{IncludeDir.NativeFileDialogExtended}",
		"%{IncludeDir.ImTimeline}",

		"%{IncludeDir.SharedStorage}"
	}

	links 
	{
		"ImGui",
		"SPIRV-Cross",
		"yaml-cpp",
		"Tracy",
		"zlib",
		"Recast",
		"Freetype",
		"MSDFGen",
		"MSDF-Atlas-Gen",
		"JoltPhysics",
		"NativeFileDialogExtended",
		"ImTimeline",

		"Saturn-SharedStorage"
	}

	filter { "options:onlineapi=steam" }
		includedirs
		{
			"%{IncludeDir.Steamworks}",
		}

		defines 
		{
			"SAT_WITH_STEAM"
		}

	filter "files:vendor/ImGuizmo/src/ImGuizmo/**.cpp"
		flags { "NoPCH" }

	filter "system:not windows"
		systemversion "latest"
		
	filter "system:linux"
		systemversion "latest"

		links 
		{
			"pthread",
			"dl",
			"m",
			"X11",
			"Xrandr",
			"vulkan",
			"vulkan-1"
		}

		defines
		{
			"SAT_PLATFORM_LINUX"
		}

		filter { "options:onlineapi=steam", "system:linux" }
			links
			{
				"vendor/steamworks/Bin/Linux/libsteam_api.so"
			}

	filter "system:windows"
		systemversion "latest"

		links 
		{
			"dwmapi",
			"vendor/vulkan/bin/vulkan-1.lib"
		}

		defines
		{
			"SAT_PLATFORM_WINDOWS",
			"_CRT_SECURE_NO_WARNINGS"
		}

		files 
		{
			"%{prj.name}/visualisers/*.natvis"
		}

		filter { "options:onlineapi=steam", "system:windows" }
			links
			{
				"vendor/steamworks/Bin/Windows/steam_api64.lib"
			}

		filter "configurations:Debug"
			defines "SAT_DEBUG"
			runtime "Debug"
			symbols "on"

			links 
			{
				"vendor/assimp/bin/Debug/assimp-vc143-mtd.lib",

				"vendor/shaderc/bin/Debug-Windows/shaderc.lib",
				"vendor/shaderc/bin/Debug-Windows/shaderc_util.lib",
				"vendor/shaderc/bin/Debug-Windows/glslangd.lib",
				"vendor/shaderc/bin/Debug-Windows/SPIRV-Tools.lib",
			}

		filter "configurations:Release"
			defines "SAT_RELEASE"
			runtime "Release"
			links
			{
				"vendor/assimp/bin/Release/assimp-vc143-mt.lib",
				"vendor/shaderc/bin/Release-Windows/shaderc.lib",
				"vendor/shaderc/bin/Release-Windows/shaderc_util.lib",
				"vendor/shaderc/bin/Release-Windows/glslang.lib",
				"vendor/shaderc/bin/Release-Windows/SPIRV-Tools.lib"
			}

		filter "configurations:Dist"
			defines "SAT_DIST"
			runtime "Release"
			optimize "on"
			symbols "off"

			removelinks { "Tracy", "Freetype", "MSDFGen", "MSDF-Atlas-Gen", "SPIRV-Cross" }
			removedefines { "TRACY_ENABLE", "TRACY_DELAYED_INIT", "TRACY_MANUAL_LIFETIME", "SATURN_SS_IMPORT" }
			removefiles { "vendor/ImGuizmo/src/**.cpp", "vendor/ImGuizmo/src/**.h" }

			defines { "SATURN_SS_STATIC" }
			links { "Saturn-SharedStorage" }
	
	filter "configurations:Debug or configurations:Release"
		defines
		{
		    "JPH_DEBUG_RENDERER",
            "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED",
            "JPH_EXTERNAL_PROFILE",
			"JPH_ENABLE_ASSERTS"
		}
