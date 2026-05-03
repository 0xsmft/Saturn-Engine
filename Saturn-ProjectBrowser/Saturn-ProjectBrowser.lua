project "Saturn-ProjectBrowser"
	location ""
	language "C++"
	cppdialect "C++23"
	staticruntime "off"
	warnings "Default"
	kind "ConsoleApp"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

	defines
	{
		"SATURN_SS_IMPORT",
		"TRACY_ENABLE",
		"TRACY_DELAYED_INIT",
		"TRACY_MANUAL_LIFETIME"
	}

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"../Saturn/vendor/spdlog/include",
		"../Saturn/src",
		"../Saturn/vendor",
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
		"../Saturn/vendor/vulkan/include",
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

	filter "configurations:Debug-ASan"
		sanitize { "Address" }

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"SAT_PLATFORM_WINDOWS",
			"_CRT_SECURE_NO_WARNINGS"
		}
		
		files 
		{
			"../Saturn/src/Saturn/Entry/Windows/**.cpp",
		}

		filter "configurations:Debug or configurations:Debug-ASan"
			defines "SAT_DEBUG"
			runtime "Debug"
			symbols "on"

			postbuildcommands 
			{
				'{COPYFILE} "../Saturn/vendor/assimp/bin/Debug/assimp-vc143-mtd.dll" "%{cfg.targetdir}"',
				
				'{COPYFILE} "../bin/Debug-windows-x86_64/Saturn-SharedStorage/Saturn-SharedStorage.dll" "%{cfg.targetdir}"'
			}

		filter "configurations:Release"
			postbuildcommands 
			{ 
				'{COPYFILE} "../bin/Release-windows-x86_64/Saturn-SharedStorage/Saturn-SharedStorage.dll" "%{cfg.targetdir}"'
			}

		filter "configurations:Dist"
			kind "WindowedApp"

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

		links
		{
			"pthread",
			"dl",
			"m",
			"xcb",
			"xcb-keysyms",
			"Xrandr",
			"xcb-randr",
			"vulkan",

			"ImGui",
			"SPIRV-Cross",
			"yaml-cpp",
			"Tracy",
			"zlib",
			"Recast",
			"MSDF-Atlas-Gen",
			"MSDFGen",
			"Freetype",
			"JoltPhysics",

			"Saturn-SharedStorage",
		}

		libdirs
		{
			"../Saturn/vendor/assimp/bin"
		}

		filter "configurations:Debug"
			links
			{
				"assimp",
				"shaderc_shared",
				"SPIRV"
			}

		files 
		{
			"../Saturn/src/Saturn/Entry/Unix/**.cpp",
		}

	filter "system:Mac"
		systemversion "latest"

		defines
		{
			"SAT_PLATFORM_MACOS"
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
		symbols "Off"
		removedefines { "TRACY_ENABLE", "TRACY_DELAYED_INIT", "TRACY_MANUAL_LIFETIME" }
