project "SaturnHeaderTool"
	location ""
	language "C++"
	kind "ConsoleApp"
	cppdialect "C++23"
	staticruntime "off"
	warnings "Default"

	targetdir( "../bin/" .. outputdir .. "/%{prj.name}" )
	objdir( "../bin-int/" .. outputdir .. "/%{prj.name}" )

	files
	{
		"src/**.h",
		"src/**.cpp"
	}
	
	includedirs
	{
		"src",

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
		"%{IncludeDir.PhysX}",
		"%{IncludeDir.PhysX}/pxshared",
		"%{IncludeDir.PhysX}/physx",
		"%{IncludeDir.Optick}",
		"../Saturn/vendor/vulkan/include",
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
