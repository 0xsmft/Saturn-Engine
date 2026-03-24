using System;
using SaturnBuildTool;

public class %PROJECT_NAME%Editor : Target
{
    public override void Init()
    {
        base.Init();

        Name = "%PROJECT_NAME%";
        Architectures = new[] { ArchitectureKind.x86_64 };

        BuildConfigs = new[] { ConfigKind.Debug, ConfigKind.Release };
        OutputType = LinkerOutput.SharedLibrary;

        // When adding modules the name of the module has to match the file name but not the folder name...
        Modules.AddRange( new string[] {
            "Saturn",
            "%PROJECT_NAME%"
        } );

        PreprocessorDefines.AddRange( new string[] {
            "_CRT_SECURE_NO_WARNINGS",
            "PX_PHYSX_STATIC_LIB",
            "PX_GENERATE_STATIC_LIBRARIES",
            "GLM_ENABLE_EXPERIMENTAL",
            "SATURN_SS_IMPORT",
            "TRACY_ENABLE",
            "TRACY_DELAYED_INIT",
            "TRACY_MANUAL_LIFETIME",
            "SAT_RBY_INCLUDE_VULKAN",
            "KHRONOS_STATIC",
        } );

          Links.AddRange( new string[] { 
            "ImGui.lib",
            "SPIRV-Cross.lib",
            "yaml-cpp.lib",
            "Tracy.lib",
            "zlib.lib",
            "Recast.lib",
            "Freetype.lib",
            "MSDFGen.lib",
            "MSDF-Atlas-Gen.lib",
            "Saturn-SharedStorage.lib",
            "ole32.lib",
            "kernel32.lib",
            "comdlg32.lib",
            "shell32.lib",
            "Saturn.lib",
        } );


        // Include directories relative to root folder (solution directory)
        Includes.AddRange( new string[] {
            "%PROJECT_NAME%/Source/%PROJECT_NAME%",
            "Saturn/src",
            "Saturn-SharedStorage/src",
            "Saturn/vendor/imgui",
            "Saturn/vendor/glm",
            "Saturn/vendor/entt/include",
            "Saturn/vendor/assimp/include",
            "Saturn/vendor/shaderc/libshaderc/include",
            "Saturn/vendor/shaderc/glslc/src",
            "Saturn/vendor/SPRIV-Cross/src",
            "Saturn/vendor/vma/src",
            "Saturn/vendor/ImGuizmo/src",
            "Saturn/vendor/yaml-cpp/include",
            "Saturn/vendor/imgui_node_editor",
            "Saturn/vendor/imspinner/src",
            "Saturn/vendor/tracy/src",
            "Saturn/vendor/Filewatch/src",
            "Saturn/vendor/miniaudio/src",
            "Saturn/vendor/physx/include",
            "Saturn/vendor/physx/include/pxshared",
            "Saturn/vendor/physx/include/physx",
            "Saturn/vendor/zlib",
            "Saturn/vendor/Recast/RecastAndDetour/Include",
            "Saturn/vendor/stb",
            "Saturn/vendor/vulkan/include",
            "Saturn/vendor/spdlog/include",
            "Saturn/vendor/freetype/include",
            "Saturn/vendor/msdf-atlas-gen",
            "Saturn/vendor/msdf-atlas-gen/msdfgen/",
            "Saturn/vendor/msdf-atlas-gen/msdfgen/core",
        } );

    }
}
