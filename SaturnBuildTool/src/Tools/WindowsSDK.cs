using System.IO;

namespace SaturnBuildTool.Tools
{
    public static class WindowsSDK
    {
        public static string GetIncludePaths()
        {
            string WinSDK = @"C://Program Files (x86)//Windows Kits";

            if( Directory.Exists( WinSDK ) ) 
            {
                foreach( string path in Directory.GetDirectories( WinSDK ) ) 
                {
                    string includePath = Path.Combine( path, "Include" );

                    if( Directory.Exists( includePath ) ) 
                    {
                        // Version folder
                        return Directory.GetDirectories( includePath)[0];
                    }
                }
            }

            return null;
        }

        public static string GetLibraryPaths()
        {
            string WinSDK = @"C://Program Files (x86)//Windows Kits//";

            if (Directory.Exists(WinSDK))
            {
                foreach (string path in Directory.GetDirectories(WinSDK))
                {
                    string libPath = Path.Combine( path, "Lib" );

                    if (Directory.Exists(libPath))
                    {
                        // Version folder
                        // Use the best version
                        return Directory.GetDirectories(libPath)[0];
                    }
                }
            }

            return null;
        }
    }
}
