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
		"PX_PHYSX_STATIC_LIB",
		"PX_GENERATE_STATIC_LIBRARIES",
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
		"%{IncludeDir.PhysX}",
		"%{IncludeDir.PhysX}/pxshared",
		"%{IncludeDir.PhysX}/physx",
		"%{IncludeDir.KTX_Software}",
		"%{IncludeDir.Recast}",
		"%{IncludeDir.acl}",
		"%{IncludeDir.rtm}",
		"%{IncludeDir.freetype}",
		"%{IncludeDir.MSDF}",
		"%{IncludeDir.MSDFAG}",

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

		"Saturn-SharedStorage"
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

				-- PhysX
				"vendor/physx/bin/Debug/LowLevel_static_64.lib",
				"vendor/physx/bin/Debug/LowLevelAABB_static_64.lib",
				"vendor/physx/bin/Debug/LowLevelDynamics_static_64.lib",
				"vendor/physx/bin/Debug/PhysX_64.lib",
				"vendor/physx/bin/Debug/PhysXCharacterKinematic_static_64.lib",
				"vendor/physx/bin/Debug/PhysXCommon_64.lib",
				"vendor/physx/bin/Debug/PhysXCooking_64.lib",
				"vendor/physx/bin/Debug/PhysXExtensions_static_64.lib",
				"vendor/physx/bin/Debug/PhysXFoundation_64.lib",
				"vendor/physx/bin/Debug/PhysXPvdSDK_static_64.lib",
				"vendor/physx/bin/Debug/PhysXTask_static_64.lib",
				"vendor/physx/bin/Debug/PhysXVehicle_static_64.lib",
				"vendor/physx/bin/Debug/SceneQuery_static_64.lib",
				"vendor/physx/bin/Debug/SimulationController_static_64.lib",
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

		filter "configurations:Release or configurations:Dist"
			links 
			{
				"vendor/physx/bin/Release/LowLevel_static_64.lib",
				"vendor/physx/bin/Release/LowLevelAABB_static_64.lib",
				"vendor/physx/bin/Release/LowLevelDynamics_static_64.lib",
				"vendor/physx/bin/Release/PhysX_64.lib",
				"vendor/physx/bin/Release/PhysXCharacterKinematic_static_64.lib",
				"vendor/physx/bin/Release/PhysXCommon_64.lib",
				"vendor/physx/bin/Release/PhysXCooking_64.lib",
				"vendor/physx/bin/Release/PhysXExtensions_static_64.lib",
				"vendor/physx/bin/Release/PhysXFoundation_64.lib",
				"vendor/physx/bin/Release/PhysXPvdSDK_static_64.lib",
				"vendor/physx/bin/Release/PhysXTask_static_64.lib",
				"vendor/physx/bin/Release/PhysXVehicle_static_64.lib",
				"vendor/physx/bin/Release/SceneQuery_static_64.lib",
				"vendor/physx/bin/Release/SimulationController_static_64.lib"
			}
