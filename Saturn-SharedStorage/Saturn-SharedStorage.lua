project "Saturn-SharedStorage"
	location ""
	kind "SharedLib"
	language "C++"
	cppdialect "C++23"
	staticruntime "off"
	warnings "Default"
		
	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"src",
		"../Saturn/src",
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
