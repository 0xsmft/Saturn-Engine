using System;
using System.IO;

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

            Includes.AddRange( new string[] {
                Path.Combine( "Saturn", "src" ),
                Path.Combine( "Saturn-SharedStorage", "src" ),
                Path.Combine( "Saturn", "vendor", "imgui" ),
                Path.Combine( "Saturn", "vendor", "glm" ),
                Path.Combine( "Saturn", "vendor", "entt", "include" ),
                Path.Combine( "Saturn", "vendor", "assimp","include" ),
                Path.Combine( "Saturn", "vendor", "shaderc", "libshaderc", "include" ),
                Path.Combine( "Saturn", "vendor", "shaderc", "glslc", "src" ),
                Path.Combine( "Saturn", "vendor", "SPIRV-Cross", "src" ),
                Path.Combine( "Saturn", "vendor", "vma", "src" ),
                Path.Combine( "Saturn", "vendor", "ImGuizmo", "src" ),
                Path.Combine( "Saturn", "vendor", "yaml-cpp", "include" ),
                Path.Combine( "Saturn", "vendor", "imgui_node_editor" ),
                Path.Combine( "Saturn", "vendor", "imspinner", "src" ),
                Path.Combine( "Saturn", "vendor", "tracy", "src" ),
                Path.Combine( "Saturn", "vendor", "Filewatch", "src" ),
                Path.Combine( "Saturn", "vendor", "miniaudio", "src" ),
                Path.Combine( "Saturn", "vendor", "JoltPhysics", "Jolt" ),
                Path.Combine( "Saturn", "vendor", "zlib" ),
                Path.Combine( "Saturn", "vendor", "Recast", "RecastAndDetour", "Include" ),
                Path.Combine( "Saturn", "vendor", "stb" ),
                Path.Combine( "Saturn", "vendor", "spdlog", "include" ),
                Path.Combine( "Saturn", "vendor", "acl", "include" ),
                Path.Combine( "Saturn", "vendor", "acl", "rtm", "include" ),
                Path.Combine( "Saturn", "vendor", "nativefiledialogext", "src", "include", "nativefiledialog" ),
            } );

            string vkSdk = VulkanSDKPath.FindVulkanSDKPath();
            Includes.AddRange( new string[] {
                Path.Combine( vkSdk, "include", "vulkan" ),
                Path.Combine( vkSdk, "include" ),
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
                    Links.AddRange( new string[] { "ole32", "kernel32", "comdlg32", "Gdi32", "shell32", "Advapi32" } );
                } break;
            }
        }

    }
}
