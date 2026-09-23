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

		filter { "system:windows", "configurations:Debug or configurations:Debug-ASan" }
			defines "SAT_DEBUG"
			runtime "Debug"
			symbols "on"

			postbuildcommands 
			{
				'{COPYFILE} "../Saturn/vendor/assimp/bin/Debug/assimp-vc143-mtd.dll" "%{cfg.targetdir}"',
				'{COPYFILE} "../bin/Debug-windows-x86_64/Saturn-SharedStorage/Saturn-SharedStorage.dll" "%{cfg.targetdir}"'
			}

		filter { "system:windows", "configurations:Release" }
			postbuildcommands 
			{ 
				'{COPYFILE} "../Saturn/vendor/assimp/bin/Release/assimp-vc143-mt.dll" "%{cfg.targetdir}"',
				'{COPYFILE} "../bin/Release-windows-x86_64/Saturn-SharedStorage/Saturn-SharedStorage.dll" "%{cfg.targetdir}"'
			}

		filter "configurations:Dist"
			kind "WindowedApp"

		filter { "system:windows", "configurations:Release or configurations:Dist" }
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
			"NativeFileDialogExtended",
			"ImTimeline",

			"Saturn-SharedStorage",
		}

		libdirs
		{
			"../Saturn/vendor/assimp/bin",
			os.getenv('VULKAN_SDK') .. "/lib",
		}

		if os.target() == "linux" then
			AppendPkgConfigLibraries("gtk+-3.0")
		end

		filter { "system:linux", "configurations:Debug" }
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

	filter "system:macosx"
		kind "WindowedApp"

		runpathdirs 
		{
			"%{cfg.targetdir}",
			os.getenv('VULKAN_SDK') .. "/lib",
			"../Saturn/vendor/assimp/bin/"
		}

		defines
		{
			"SAT_PLATFORM_MACOS"
		}

		files 
		{
			"../Saturn/src/Saturn/Entry/Unix/**.cpp",
		}

		libdirs
		{
			"../Saturn/vendor/assimp/bin",
			os.getenv('VULKAN_SDK') .. "/lib",
		}

		links 
		{
			"vulkan",
			"assimp",
			"shaderc_shared",

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
			"NativeFileDialogExtended",
			"ImTimeline",

			"Saturn-SharedStorage",

			"Cocoa.framework",
			"CoreFoundation.framework",
			"IOKit.framework",
			"CoreVideo.framework",
			"QuartzCore.framework",
			"UniformTypeIdentifiers.framework",
		}

		externalincludedirs
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

		filter "action:xcode4"
			files 
			{
				"Dist/macOS/Info.plist",
				"Dist/macOS/Entitlements.plist",
			}

			xcodebuildsettings
			{
				["PRODUCT_BUNDLE_IDENTIFIER"] = 'dev.0xsmft.Saturn',
				["CODE_SIGN_STYLE"] = "Automatic",
				["INFOPLIST_FILE"] = "Dist/macOS/Info.plist",
				["CODE_SIGN_ENTITLEMENTS"] = "Dist/macOS/Entitlements.plist",
				["LD_RUNPATH_SEARCH_PATHS"] = "$(inherited) @executable_path/../Frameworks @rpath /System/Library/Frameworks",
			}

		filter {}

		filter { "system:macosx", "configurations:Debug" }
			local vulkanSDKPath = os.getenv('VULKAN_SDK')

			postbuildcommands 
			{
				'{MKDIR} "%{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Frameworks"',
				'{MKDIR} "%{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Resources"',

				"cp -Rv ../Saturn-ProjectBrowser/content %{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Resources/",

			    '{COPYFILE} "../Saturn-Editor/content/Icons/SaturnIcon.icns" "%{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Resources/"',

				'{COPYFILE} "../bin/Debug-macosx-AARCH64/Saturn-SharedStorage/libSaturn-SharedStorage.dylib" "%{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Frameworks/"',
				'{COPYFILE} "../Saturn/vendor/assimp/bin/libassimp.dylib" "%{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Frameworks/"',
				'{COPYFILE} "../Saturn/vendor/assimp/bin/libassimp.5.dylib" "%{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Frameworks/"',
				'{COPYFILE} "' .. vulkanSDKPath .. '/lib/libshaderc_shared.1.dylib" "%{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Frameworks/"',
				'{COPYFILE} "' .. vulkanSDKPath .. '/lib/libvulkan.1.dylib" "%{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Frameworks/"',
			}

		filter { "system:macosx", "configurations:Release" }
			local vulkanSDKPath = os.getenv('VULKAN_SDK')

			postbuildcommands 
			{
				'{MKDIR} "%{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Frameworks"',
				'{MKDIR} "%{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Resources"',
				
				"cp -Rv ../Saturn-ProjectBrowser/content %{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Resources/",

			    '{COPYFILE} "../Saturn-Editor/content/Icons/SaturnIcon.icns" "%{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Resources/"',

				'{COPYFILE} "../bin/Release-macosx-AARCH64/Saturn-SharedStorage/libSaturn-SharedStorage.dylib" "%{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Frameworks/"',
				'{COPYFILE} "../Saturn/vendor/assimp/bin/libassimp.dylib" "%{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Frameworks/"',
				'{COPYFILE} "../Saturn/vendor/assimp/bin/libassimp.5.dylib" "%{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Frameworks/"',
				'{COPYFILE} "' .. vulkanSDKPath .. '/lib/libshaderc_shared.1.dylib" "%{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Frameworks/"',
				'{COPYFILE} "' .. vulkanSDKPath .. '/lib/libvulkan.1.dylib" "%{cfg.targetdir}/Saturn-ProjectBrowser.app/Contents/Frameworks/"',
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
