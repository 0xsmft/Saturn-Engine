workspace "Saturn"
	architecture "x64"
	startproject "Saturn-Editor"
	targetdir "build"
	warnings "Default"

	configurations { "Debug", "Release", "Dist" }

	flags { "MultiProcessorCompile" }
	editandcontinue "Off"
	debugformat "c7"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["Ruby"] = "Saturn/vendor/Ruby/src"
IncludeDir["ImGui"] = "Saturn/vendor/imgui"
IncludeDir["glm"] = "Saturn/vendor/glm"
IncludeDir["entt"] = "Saturn/vendor/entt/include"
IncludeDir["assimp"] = "Saturn/vendor/assimp/include"
IncludeDir["shaderc"] = "Saturn/vendor/shaderc/libshaderc/include"
IncludeDir["glslc"] = "Saturn/vendor/shaderc/glslc/src"
IncludeDir["SPIRV_Cross"] = "Saturn/vendor/SPIRV-Cross/src/"
IncludeDir["vma"] = "Saturn/vendor/vma/src/"
IncludeDir["ImGuizmo"] = "Saturn/vendor/ImGuizmo/src/"
IncludeDir["yaml_cpp"] = "Saturn/vendor/yaml-cpp/include/"
IncludeDir["ImguiNodeEditor"] = "Saturn/vendor/imgui_node_editor"
IncludeDir["ImSpinner"] = "Saturn/vendor/imspinner/src"
IncludeDir["Tracy"] = "Saturn/vendor/tracy/src"
IncludeDir["Filewatch"] = "Saturn/vendor/Filewatch/src"
IncludeDir["MiniAudio"] = "Saturn/vendor/miniaudio/src"
IncludeDir["PhysX"] = "Saturn/vendor/physx/include"
IncludeDir["SharedStorage"] = "Saturn-SharedStorage/src"
IncludeDir["zlib"] = "Saturn/vendor/zlib"
IncludeDir["KTX_Software"] = "Saturn/vendor/KTX-Software/include"
IncludeDir["Recast"] = "Saturn/vendor/Recast/RecastAndDetour/Include"
IncludeDir["acl"] = "Saturn/vendor/acl/include"
IncludeDir["rtm"] = "Saturn/vendor/acl/rtm/include"
IncludeDir["Freetype"] = "Saturn/vendor/freetype/include"
IncludeDir["MSDF"] = "Saturn/vendor/msdf-atlas-gen/msdfgen"
IncludeDir["MSDFAG"] = "Saturn/vendor/msdf-atlas-gen"

group "Dependencies"
	include "Saturn/vendor/imgui"
	include "Saturn/vendor/SPIRV-Cross"
	include "Saturn/vendor/yaml-cpp"
	include "Saturn/vendor/tracy"
	include "Saturn/vendor/zlib"
	include "Saturn/vendor/Recast"
	include "Saturn/vendor/freetype"
	include "Saturn/vendor/msdf-atlas-gen"

group "Engine"
project "Saturn"
	location "Saturn"
	kind "StaticLib"
	language "C++"
	cppdialect "C++23"
	staticruntime "off"
	warnings "Default"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "sppch.h"
	pchsource "Saturn/src/sppch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/stb/**.cpp",
		"%{prj.name}/vendor/stb/**.h",	
		"%{prj.name}/vendor/vma/src/**.cpp",
		"%{prj.name}/vendor/vma/src/**.h",
		"%{prj.name}/vendor/vulkan/**.h",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",
		"%{prj.name}/vendor/ImGuizmo/src/**.cpp",
		"%{prj.name}/vendor/ImGuizmo/src/**.h",
	}

	removefiles 
	{
		"%{prj.name}/src/%{prj.name}/Entry/macOS/**.cpp",
		"%{prj.name}/src/%{prj.name}/Entry/Unix/**.cpp",
		"%{prj.name}/src/%{prj.name}/Entry/Windows/**.cpp",
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
		
		"SAT_RBY_INCLUDE_VULKAN",
		"KHRONOS_STATIC"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/stb/",
		"%{prj.name}/vendor/spdlog/include",
		"%{prj.name}/vendor/vulkan/include",
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

	filter "files:Saturn/vendor/ImGuizmo/src/ImGuizmo/**.cpp"
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
			"Saturn/vendor/vulkan/bin/vulkan-1.lib"
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
				"Saturn/vendor/assimp/bin/Debug/assimp-vc143-mtd.lib",

				"Saturn/vendor/shaderc/bin/Debug-Windows/shaderc.lib",
				"Saturn/vendor/shaderc/bin/Debug-Windows/shaderc_util.lib",
				"Saturn/vendor/shaderc/bin/Debug-Windows/glslangd.lib",
				"Saturn/vendor/shaderc/bin/Debug-Windows/SPIRV-Tools.lib",

				-- PhysX
				"Saturn/vendor/physx/bin/Debug/LowLevel_static_64.lib",
				"Saturn/vendor/physx/bin/Debug/LowLevelAABB_static_64.lib",
				"Saturn/vendor/physx/bin/Debug/LowLevelDynamics_static_64.lib",
				"Saturn/vendor/physx/bin/Debug/PhysX_64.lib",
				"Saturn/vendor/physx/bin/Debug/PhysXCharacterKinematic_static_64.lib",
				"Saturn/vendor/physx/bin/Debug/PhysXCommon_64.lib",
				"Saturn/vendor/physx/bin/Debug/PhysXCooking_64.lib",
				"Saturn/vendor/physx/bin/Debug/PhysXExtensions_static_64.lib",
				"Saturn/vendor/physx/bin/Debug/PhysXFoundation_64.lib",
				"Saturn/vendor/physx/bin/Debug/PhysXPvdSDK_static_64.lib",
				"Saturn/vendor/physx/bin/Debug/PhysXTask_static_64.lib",
				"Saturn/vendor/physx/bin/Debug/PhysXVehicle_static_64.lib",
				"Saturn/vendor/physx/bin/Debug/SceneQuery_static_64.lib",
				"Saturn/vendor/physx/bin/Debug/SimulationController_static_64.lib",
			}

		filter "configurations:Release"
			defines "SAT_RELEASE"
			runtime "Release"
		--	optimize "on"
			links { "Saturn/vendor/assimp/bin/Release/assimp-vc143-mt.lib" }

		filter "configurations:Dist"
			defines "SAT_DIST"
			runtime "Release"
			optimize "on"
			symbols "off"

			removelinks { "Tracy" }
			removedefines { "TRACY_ENABLE", "TRACY_DELAYED_INIT", "TRACY_MANUAL_LIFETIME", "SATURN_SS_IMPORT" }
			removefiles { "%{prj.name}/vendor/ImGuizmo/src/**.cpp", "%{prj.name}/vendor/ImGuizmo/src/**.h" }

			defines { "SATURN_SS_STATIC" }
			links { "Saturn-SharedStorage" }

		filter "configurations:Release or configurations:Dist"
			links 
			{
				"Saturn/vendor/shaderc/bin/Release-Windows/shaderc.lib",
				"Saturn/vendor/shaderc/bin/Release-Windows/shaderc_util.lib",
				"Saturn/vendor/shaderc/bin/Release-Windows/glslang.lib",
				"Saturn/vendor/shaderc/bin/Release-Windows/SPIRV-Tools.lib",
				"Saturn/vendor/shaderc/bin/Release-Windows/glslc.lib",

				"Saturn/vendor/physx/bin/Release/LowLevel_static_64.lib",
				"Saturn/vendor/physx/bin/Release/LowLevelAABB_static_64.lib",
				"Saturn/vendor/physx/bin/Release/LowLevelDynamics_static_64.lib",
				"Saturn/vendor/physx/bin/Release/PhysX_64.lib",
				"Saturn/vendor/physx/bin/Release/PhysXCharacterKinematic_static_64.lib",
				"Saturn/vendor/physx/bin/Release/PhysXCommon_64.lib",
				"Saturn/vendor/physx/bin/Release/PhysXCooking_64.lib",
				"Saturn/vendor/physx/bin/Release/PhysXExtensions_static_64.lib",
				"Saturn/vendor/physx/bin/Release/PhysXFoundation_64.lib",
				"Saturn/vendor/physx/bin/Release/PhysXPvdSDK_static_64.lib",
				"Saturn/vendor/physx/bin/Release/PhysXTask_static_64.lib",
				"Saturn/vendor/physx/bin/Release/PhysXVehicle_static_64.lib",
				"Saturn/vendor/physx/bin/Release/SceneQuery_static_64.lib",
				"Saturn/vendor/physx/bin/Release/SimulationController_static_64.lib",
			}

---------------------------------------------------------------------------------------------------------------------------

group "Editor"
project "Saturn-Editor"
	location "Saturn-Editor"
	language "C++"
	cppdialect "C++23"
	staticruntime "off"
	warnings "Default"
	kind "ConsoleApp"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	defines
	{
		"SAT_HAS_EDITOR",
		"TRACY_ENABLE",
		"TRACY_DELAYED_INIT",
		"TRACY_MANUAL_LIFETIME",
	}

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"Saturn/vendor/spdlog/include",
		"Saturn/src",
		"Saturn/vendor",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.glslc}",
		"%{IncludeDir.shaderc}",
		"%{IncludeDir.SPIRV_Cross}",
		"%{IncludeDir.vma}",
		"%{IncludeDir.PhysX}",
		"%{IncludeDir.PhysX}/pxshared",
		"%{IncludeDir.PhysX}/physx",
		"%{IncludeDir.Optick}",
		"Saturn/vendor/vulkan/include",
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
			"Saturn/visualisers/*.natvis",
			"Saturn/src/Saturn/Entry/Windows/**.cpp",
		}

		filter "configurations:Debug"
			defines "SAT_DEBUG"
			runtime "Debug"
			symbols "on"

			postbuildcommands 
			{ 
				'{COPYFILE} "../Saturn/vendor/assimp/bin/Debug/assimp-vc143-mtd.dll" "%{cfg.targetdir}"',
				'{COPYFILE} "../bin/Debug-windows-x86_64/Saturn-SharedStorage/Saturn-SharedStorage.dll" "%{cfg.targetdir}"',
				'{COPYFILE} "../Saturn/vendor/physx/bin/Debug/PhysXCommon_64.dll" "%{cfg.targetdir}"',
				'{COPYFILE} "../Saturn/vendor/physx/bin/Debug/PhysXFoundation_64.dll" "%{cfg.targetdir}"',
				'{COPYFILE} "../Saturn/vendor/physx/bin/Debug/PhysXCooking_64.dll" "%{cfg.targetdir}"',
				'{COPYFILE} "../Saturn/vendor/physx/bin/Debug/PhysX_64.dll" "%{cfg.targetdir}"',
			}

		filter "configurations:Release"
			defines "SAT_RELEASE"
			runtime "Release"
		--	optimize "on"

			postbuildcommands 
			{ 
				'{COPYFILE} "../bin/Release-windows-x86_64/Saturn-SharedStorage/Saturn-SharedStorage.dll" "%{cfg.targetdir}"',
			}

		filter "configurations:Dist"
			defines "SAT_DIST"
			runtime "Release"
			optimize "on"
			symbols "Off"
			kind "WindowedApp"

			removedefines { "SATURN_SS_IMPORT" }
			defines { "SATURN_SS_STATIC" }

		filter "configurations:Dist or configurations:Release"
			postbuildcommands 
			{ 
				'{COPYFILE} "../Saturn/vendor/assimp/bin/Release/assimp-vc143-mt.dll" "%{cfg.targetdir}"',
				'{COPYFILE} "../Saturn/vendor/physx/bin/Release/PhysXCommon_64.dll" "%{cfg.targetdir}"',
				'{COPYFILE} "../Saturn/vendor/physx/bin/Release/PhysXFoundation_64.dll" "%{cfg.targetdir}"',
				'{COPYFILE} "../Saturn/vendor/physx/bin/Release/PhysXCooking_64.dll" "%{cfg.targetdir}"',
				'{COPYFILE} "../Saturn/vendor/physx/bin/Release/PhysX_64.dll" "%{cfg.targetdir}"',
			}

	filter "system:linux"
		systemversion "latest"

		defines
		{
			"SAT_PLATFORM_LINUX"
		}

		files 
		{
			"Saturn/src/Saturn/Entry/Unix/**.cpp",
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


group "Tools"
project "Saturn-ProjectBrowser"
	location "Saturn-ProjectBrowser"
	language "C++"
	cppdialect "C++23"
	staticruntime "off"
	warnings "Default"
	kind "ConsoleApp"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	defines
	{
		"SATURN_SS_IMPORT",
		"TRACY_ENABLE",
		"TRACY_DELAYED_INIT",
		"TRACY_MANUAL_LIFETIME"
	}

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"Saturn/vendor/spdlog/include",
		"Saturn/src",
		"Saturn/vendor",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.glslc}",
		"%{IncludeDir.shaderc}",
		"%{IncludeDir.SPIRV_Cross}",
		"%{IncludeDir.vma}",
		"%{IncludeDir.PhysX}",
		"%{IncludeDir.PhysX}/pxshared",
		"%{IncludeDir.PhysX}/physx",
		"%{IncludeDir.Optick}",
		"Saturn/vendor/vulkan/include",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.ImSpinner}",
		"%{IncludeDir.Filewatch}",
		"%{IncludeDir.MiniAudio}",
		"%{IncludeDir.ImguiNodeEditor}",
		"%{IncludeDir.Tracy}",
		"%{IncludeDir.KTX_Software}",
		"%{IncludeDir.Recast}",
		"%{IncludeDir.freetype}",
		"%{IncludeDir.MSDF}",
		"%{IncludeDir.MSDFAG}",
		"%{IncludeDir.acl}",
		"%{IncludeDir.rtm}",

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
			"_CRT_SECURE_NO_WARNINGS"
		}
		
		files 
		{
			"Saturn/src/Saturn/Entry/Windows/**.cpp",
		}

		filter "configurations:Debug"
			defines "SAT_DEBUG"
			runtime "Debug"
			symbols "on"

			postbuildcommands 
			{
				'{COPYFILE} "../Saturn/vendor/assimp/bin/Debug/assimp-vc143-mtd.dll" "%{cfg.targetdir}"',
				
				'{COPYFILE} "../bin/Debug-windows-x86_64/Saturn-SharedStorage/Saturn-SharedStorage.dll" "%{cfg.targetdir}"'
			}

		filter "configurations:Release"
			defines "SAT_RELEASE"
			runtime "Release"
			optimize "on"

			postbuildcommands 
			{ 
				'{COPYFILE} "../bin/Release-windows-x86_64/Saturn-SharedStorage/Saturn-SharedStorage.dll" "%{cfg.targetdir}"'
			}

		filter "configurations:Dist"
			defines "SAT_DIST"
			runtime "Release"
			optimize "on"
			symbols "Off"
			kind "WindowedApp"

			removedefines { "TRACY_ENABLE", "TRACY_DELAYED_INIT", "TRACY_MANUAL_LIFETIME" }

		filter "configurations:Release or configurations:Dist"
			postbuildcommands 
			{ 
				'{COPYFILE} "../Saturn/vendor/assimp/bin/Release/assimp-vc143-mt.dll" "%{cfg.targetdir}"',
			}

	filter "system:linux"
		systemversion "latest"

		defines
		{
			"SAT_PLATFORM_LINUX"
		}

		files 
		{
			"Saturn/src/Saturn/Entry/Unix/**.cpp",
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
			"Saturn/src/Saturn/Entry/Unix/**.cpp",
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

group "Tools"
project "Saturn-SharedStorage"
	location "Saturn-SharedStorage"
	kind "SharedLib"
	language "C++"
	cppdialect "C++23"
	staticruntime "off"
	warnings "Default"
		
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"Saturn/src",
	}
	
	filter "system:windows or system:linux or system:Mac"
		systemversion "latest"
		
		filter "configurations:Debug"
			runtime "Debug"
			symbols "on"
		
		filter "configurations:Release"
			runtime "Release"
			optimize "on"
		
		filter "configurations:Dist"
			runtime "Release"
			optimize "on"
			symbols "Off"
			kind "StaticLib"
			defines { "SATURN_SS_STATIC" }

group "Trinity"
project "SaturnBuildTool"
	location "SaturnBuildTool"
	language "C#"
	kind "ConsoleApp"
	links { "System", "System.Xml" }

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.cs"
	}

	postbuildcommands
	{
		'{COPY} "../../../SaturnBuildTool/RT" "RT/"'
	}

	filter { "configurations:Debug" }
		symbols "On"

 	filter { "configurations:Release" }
		optimize "On"

	filter { "configurations:Dist" }
		optimize "On"

group "Trinity"
project "SaturnHeaderTool"
	location "SaturnHeaderTool"
	language "C++"
	kind "ConsoleApp"
	cppdialect "C++23"
	staticruntime "off"
	warnings "Default"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}
	
	includedirs
	{
		"SaturnHeaderTool/src",

		"Saturn/vendor/spdlog/include",
		"Saturn/src",
		"Saturn/vendor",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.glslc}",
		"%{IncludeDir.shaderc}",
		"%{IncludeDir.SPIRV_Cross}",
		"%{IncludeDir.vma}",
		"%{IncludeDir.PhysX}",
		"%{IncludeDir.PhysX}/pxshared",
		"%{IncludeDir.PhysX}/physx",
		"%{IncludeDir.Optick}",
		"Saturn/vendor/vulkan/include",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.ImSpinner}",
		"%{IncludeDir.Filewatch}",
		"%{IncludeDir.MiniAudio}",
		"%{IncludeDir.ImguiNodeEditor}",
		"%{IncludeDir.Tracy}",
		"%{IncludeDir.KTX_Software}",

		"%{IncludeDir.SharedStorage}"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"SAT_PLATFORM_WINDOWS"
		}

	filter "system:linux"
		systemversion "latest"

		defines
		{
			"SAT_PLATFORM_LINUX"
		}

	filter "system:Mac"
		systemversion "latest"

		defines
		{
			"SAT_PLATFORM_MACOS"
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
		symbols "Off"
