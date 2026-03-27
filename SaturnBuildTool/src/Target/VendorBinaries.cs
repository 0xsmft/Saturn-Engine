using System.IO;

namespace SaturnBuildTool
{
    public enum VendorProject
    {
        JOLT,
        YAML_CPP,
        IMGUI,
        SPIRVCROSS,
        SHADERC,
        TRACY,
        ZLIB,
        RECAST, // ...and detour
        FREETYPE,
        MSDF,
        MSDFGEN
    }

    public class VendorBinaries
    {
        public static string GetRootBinPath( string path )
        {
            switch( Shared.ProjectInfo.CurrentConfigKind )
            {
                case ConfigKind.Debug:
                    {
                        path = Path.Combine( path, "Debug-windows-x86_64" );
                    }
                    break;

                case ConfigKind.Release:
                    {
                        path = Path.Combine( path, "Release-windows-x86_64" );
                    }
                    break;

                case ConfigKind.Dist:
                    {
                        path = Path.Combine( path, "Dist-windows-x86_64" );
                    }
                    break;
            }

            return path;
        }

        // TODO
        // TEMP: There is 1000% a better way of doing this.
        public static string GetBinPath( VendorProject project )
        {
            string saturnDir = Shared.ProjectInfo.SaturnDir;
            string binPath = Path.Combine( saturnDir, "Saturn", "vendor" );

            switch( project )
            {
                case VendorProject.JOLT:
                    {
                        binPath = Path.Combine( binPath, "JoltPhysics", "bin" );

                        binPath = GetRootBinPath( binPath );

                        binPath = Path.Combine( binPath, "JoltPhysics" );
                    }
                    break;


                case VendorProject.YAML_CPP:
                    {
                        binPath = Path.Combine( binPath, "yaml-cpp", "bin" );

                        binPath = GetRootBinPath( binPath );

                        binPath = Path.Combine( binPath, "yaml-cpp" );
                    }
                    break;


                case VendorProject.IMGUI:
                    {
                        binPath = Path.Combine( binPath, "imgui", "bin" );

                        binPath = GetRootBinPath( binPath );

                        binPath = Path.Combine( binPath, "ImGui" );
                    }
                    break;


                case VendorProject.SPIRVCROSS:
                    {
                        binPath = Path.Combine( binPath, "SPIRV-Cross", "bin" );

                        binPath = GetRootBinPath( binPath );

                        binPath = Path.Combine( binPath, "SPIRV-Cross" );
                    }
                    break;


                case VendorProject.SHADERC:
                    {
                        binPath = Path.Combine( binPath, "shaderc", "bin" );

                        if( Shared.ProjectInfo.CurrentConfigKind == ConfigKind.Debug )
                        {
                            binPath = Path.Combine( binPath, "Debug-Windows" );
                        }
                        else
                        {
                            binPath = Path.Combine( binPath, "Release-Windows" );
                        }
                    }
                    break;

                case VendorProject.TRACY:
                    {
                        binPath = Path.Combine( binPath, "tracy", "bin" );

                        binPath = GetRootBinPath( binPath );

                        binPath = Path.Combine( binPath, "Tracy" );
                    }
                    break;

                case VendorProject.ZLIB:
                    {
                        binPath = Path.Combine( binPath, "zlib", "bin" );

                        binPath = GetRootBinPath( binPath );

                        binPath = Path.Combine( binPath, "zlib" );
                    }
                    break;

                case VendorProject.RECAST:
                    {
                        binPath = Path.Combine( binPath, "Recast", "bin" );

                        binPath = GetRootBinPath( binPath );

                        binPath = Path.Combine( binPath, "Recast" );
                    }
                    break;

                case VendorProject.FREETYPE:
                    {
                        binPath = Path.Combine( binPath, "freetype", "bin" );

                        binPath = GetRootBinPath( binPath );

                        binPath = Path.Combine( binPath, "Freetype" );
                    }
                    break;

                case VendorProject.MSDF:
                    {
                        binPath = Path.Combine( binPath, "msdf-atlas-gen", "msdfgen", "bin" );

                        binPath = GetRootBinPath( binPath );

                        binPath = Path.Combine( binPath, "MSDFGen" );
                    }
                    break;

                case VendorProject.MSDFGEN:
                    {
                        binPath = Path.Combine( binPath, "msdf-atlas-gen", "bin" );

                        binPath = GetRootBinPath( binPath );

                        binPath = Path.Combine( binPath, "MSDF-Atlas-Gen" );
                    }
                    break;
            }

            return binPath;
        }
    }
}
