using System;

namespace SaturnBuildTool
{
    /// <summary>
    /// This class is a helper class to reduce the boilerplate code in the Game's MyProject.Development.cs file
    /// </summary>
    public class GameDevelopmentTarget : Target
    {
        public override void Init()
        {
            base.Init();

            Architectures = new[] { ArchitectureKind.x86_64 };
            BuildConfigs = new[] { ConfigKind.Debug, ConfigKind.Release };
            OutputType = LinkerOutput.SharedLibrary;

            Modules.Add( "Saturn" );

            PreprocessorDefines.AddRange( new string[] {
                "GLM_ENABLE_EXPERIMENTAL",
                "SATURN_SS_IMPORT",
                "TRACY_ENABLE",
                "TRACY_DELAYED_INIT",
                "TRACY_MANUAL_LIFETIME",
                "SAT_RBY_INCLUDE_VULKAN",
                "KHRONOS_STATIC",
                "JPH_DEBUG_RENDERER",
                "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED",
                "JPH_EXTERNAL_PROFILE",
                "JPH_ENABLE_ASSERTS"
            } );

            Links.AddRange( new string[] {
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
                "Saturn-SharedStorage",
                "Saturn",
            } );

            // Include directories relative to root folder (solution directory)
            Includes.AddRange( new string[] {
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
                "Saturn/vendor/JoltPhysics/Jolt",
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

            AppendPlatformRequirements();
        }

        private void AppendPlatformRequirements() 
        {
            switch( Shared.Platform.PlatformType ) 
            {
                default: break;

                case PlatformType.Windows:
                {
                    Links.AddRange( new string[] { "ole32", "kernel32", "comdlg32", "shell32" } );
                } break;
            }
        }

    }
}
