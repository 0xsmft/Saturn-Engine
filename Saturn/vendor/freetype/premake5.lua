project "Freetype"
	kind "StaticLib"
	language "C"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
        "include/ft2build.h",
		"include/freetype/*.h",
		"include/freetype/config/*.h",
		"include/freetype/internal/*.h",
		"src/autofit/autofit.c",
		"src/base/ftbase.c",
		"src/base/ftbbox.c",
		"src/base/ftbdf.c",
		"src/base/ftbitmap.c",
		"src/base/ftcid.c",
		"src/base/ftdebug.c",
		"src/base/ftfstype.c",
		"src/base/ftgasp.c",
		"src/base/ftglyph.c",
		"src/base/ftgxval.c",
		"src/base/ftinit.c",
		"src/base/ftmm.c",
		"src/base/ftotval.c",
		"src/base/ftpatent.c",
		"src/base/ftpfr.c",
		"src/base/ftstroke.c",
		"src/base/ftsynth.c",
		"src/base/ftsystem.c",
		"src/base/fttype1.c",
		"src/base/ftwinfnt.c",
		"src/bdf/bdf.c",
		"src/bzip2/ftbzip2.c",
		"src/cache/ftcache.c",
		"src/cff/cff.c",
		"src/cid/type1cid.c",
		"src/gzip/ftgzip.c",
		"src/lzw/ftlzw.c",
		"src/pcf/pcf.c",
		"src/pfr/pfr.c",
		"src/psaux/psaux.c",
		"src/pshinter/pshinter.c",
		"src/psnames/psnames.c",
		"src/raster/raster.c",
		"src/sdf/sdf.c",
		"src/sfnt/sfnt.c",
		"src/smooth/smooth.c",
		"src/truetype/truetype.c",
		"src/type1/type1.c",
		"src/type42/type42.c",
		"src/winfonts/winfnt.c"
    }

    defines 
    {
		"FT2_BUILD_LIBRARY",
    }

    includedirs 
    {
        "include"
    }

	filter "system:windows"
		systemversion "latest"
		staticruntime "On"

        defines
	    {
		    "_CRT_SECURE_NO_WARNINGS",
		    "_CRT_NONSTDC_NO_WARNINGS",
	    }

	filter "system:linux"
		pic "On"
		systemversion "latest"
		cppdialect "C++2a"
		staticruntime "On"

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
