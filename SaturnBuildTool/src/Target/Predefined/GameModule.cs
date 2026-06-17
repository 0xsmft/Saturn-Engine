using System;
using System.IO;
using System.Linq;

namespace SaturnBuildTool
{
    /// <summary>
    /// This class is a helper class for Games to use when defining their Module.
    /// It helps reduce the boilerplate code in Game's MyProject.Module.cs file.
    /// </summary>
    public class GameModule : Module
    {
        public override void Init()
        {
            base.Init();

            Name = "Saturn";

            // This is so shit, this should be handled by the Target.
            if( Shared.ProjectInfo.CurrentConfigKind == ConfigKind.Dist )
            {
                OutputType = LinkerOutput.Executable;
            }
            else
            {
                OutputType = LinkerOutput.SharedLibrary;
            }

            // Include directories relative to root folder (solution directory)
            string saturnDir = Shared.ProjectInfo.SaturnDir;

            Includes.AddRange( new string[] {
                "Build/Generated", // Game auto-generated files.
                Path.Combine( saturnDir, "Saturn/src" ),
                Path.Combine( saturnDir, "Saturn-SharedStorage/src" ),
                Path.Combine( saturnDir, "Saturn/vendor/imgui" ),
                Path.Combine( saturnDir, "Saturn/vendor/glm" ),
                Path.Combine( saturnDir, "Saturn/vendor/entt/include" ),
                Path.Combine( saturnDir, "Saturn/vendor/assimp/include" ),
                Path.Combine( saturnDir, "Saturn/vendor/shaderc/libshaderc/include" ),
                Path.Combine( saturnDir, "Saturn/vendor/shaderc/glslc/src" ),
                Path.Combine( saturnDir, "Saturn/vendor/SPIRV-Cross/src" ),
                Path.Combine( saturnDir, "Saturn/vendor/vma/src" ),
                Path.Combine( saturnDir, "Saturn/vendor/ImGuizmo/src" ),
                Path.Combine( saturnDir, "Saturn/vendor/yaml-cpp/include" ),
                Path.Combine( saturnDir, "Saturn/vendor/imgui_node_editor" ),
                Path.Combine( saturnDir, "Saturn/vendor/imspinner/src" ),
                Path.Combine( saturnDir, "Saturn/vendor/tracy/src" ),
                Path.Combine( saturnDir, "Saturn/vendor/Filewatch/src" ),
                Path.Combine( saturnDir, "Saturn/vendor/miniaudio/src" ),
                Path.Combine( saturnDir, "Saturn/vendor/JoltPhysics/Jolt" ),
                Path.Combine( saturnDir, "Saturn/vendor/zlib" ),
                Path.Combine( saturnDir, "Saturn/vendor/Recast/RecastAndDetour/Include" ),
                Path.Combine( saturnDir, "Saturn/vendor/stb" ),
                Path.Combine( saturnDir, "Saturn/vendor/vulkan/include" ),
                Path.Combine( saturnDir, "Saturn/vendor/spdlog/include" ),
                Path.Combine( saturnDir, "Saturn/vendor/acl/include" ),
                Path.Combine( saturnDir, "Saturn/vendor/acl/rtm/include" ),
                Path.Combine( saturnDir, "Saturn/vendor/nativefiledialogext/src/include/nativefiledialog" ),
            } );

            // LibraryPaths
            // Add saturn and all of saturns deps.
            string binPath = Path.Combine( saturnDir, "bin" );
            binPath = Path.Combine( binPath, VendorBinaries.GetRootBinPath( binPath ) );

            var binPaths = Enum.GetValues( typeof( VendorProject ) )
                .Cast<VendorProject>()
                .Select( v => VendorBinaries.GetBinPath( v ) )
                .ToArray();

            LibraryPaths.AddRange( new string[] {
                Path.Combine( binPath, "Saturn" ),
                Path.Combine( binPath, "Saturn-SharedStorage" ),
            } );

            LibraryPaths.AddRange( binPaths );

            // Use Saturn PCH
            PCH = new PCHInfo( "sppch.h", Path.Combine( saturnDir, "Saturn/src/sppch.cpp" ) );

            // LIBARIES
            Links.Add( "vulkan-1" );

            AddConfigLinks();
            AddPlatformLinks();
        }

        private void AddConfigLinks() 
        {
            switch( Shared.ProjectInfo.CurrentConfigKind ) 
            {
                default: break;

                case ConfigKind.Debug:
                    {
                        Links.AddRange( new string[] {
                            "shaderc_sharedd",
                            "shaderc_utild",
                            "glslangd",
                            "SPIRV-Toolsd",
                        } );
                    } break;

                case ConfigKind.Release:
                    {
                        Links.AddRange( new string[] {
                            "shaderc_shared",
                            "shaderc_util",
                            "glslang",
                            "SPIRV-Tools",
                        } );
                    } break;
            }
        }

        private void AddPlatformLinks() 
        {
            switch( Shared.Platform.PlatformType ) 
            {
                default: break;

                case PlatformType.Windows:
                {
                    // TODO: Use non-rooted path!
                    string saturnDir = Shared.ProjectInfo.SaturnDir;

                    if( Shared.ProjectInfo.CurrentConfigKind == ConfigKind.Debug ) 
                    {
                        Links.Add( Path.Combine( saturnDir, "Saturn/vendor/assimp/bin/Debug/assimp-vc143-mtd.lib" ) );
                    }
                    else if( Shared.ProjectInfo.CurrentConfigKind == ConfigKind.Release )
                    {
                        Links.Add( Path.Combine( saturnDir, "Saturn/vendor/assimp/bin/Release/assimp-vc143-mt.lib" ) );
                    }
                } break;
            }
        }
    }
}
