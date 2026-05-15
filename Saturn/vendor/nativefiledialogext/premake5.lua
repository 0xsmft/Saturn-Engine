project "NativeFileDialogExtended"
	kind "StaticLib"
	language "C++"
	cppdialect "C++23"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/include/nativefiledialog/nfd.hpp"
    }

    includedirs 
    {
        "src/include/"
    }

	filter "system:windows"
		systemversion "latest"
		staticruntime "off"

       	files
    	{
            "src/nfd_win.cpp"
	    }


	filter "system:linux"
		pic "On"
		systemversion "latest"
		staticruntime "off"

        files
    	{
            "src/nfd_gtk.cpp"
	    }

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		runtime "Release"
		optimize "on"
		symbols "off"
