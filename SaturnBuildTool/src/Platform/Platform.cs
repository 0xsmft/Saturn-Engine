using System;

namespace SaturnBuildTool
{
    public enum PlatformType
    {
        // Windows x64
        Windows,

        // Linux x64
        Linux,

        // Mac Apple (AArch64)
        MacApple,

        Unknown,
    }

    public class Platform
    {
        public string ObjectFileExtension { get; private set; }

        public string SharedLibraryExtension { get; private set; }

        public string StaticLibraryExtension { get; private set; }

        public string ExecutableExtension { get; private set; }

        public string ProgramDebugDatabaseExtension { get; private set; }

        public PlatformType PlatformType { get; private set; }

        public string PlatformName { get; private set; }

        public Platform( string rawPlatformStr )
        {
            if( rawPlatformStr == "WIN64" )
            {
                InitForWindows();
            }
            else if( rawPlatformStr == "LINUX64" )
            {
                InitForLinux();
            }
            else if( rawPlatformStr == "APPLE" )
            {
                InitForMacOS();
            }
            else
            {
                Console.WriteLine( "ERROR: Unknown platform!" );
            }
        }

        private void InitForWindows()
        {
            ObjectFileExtension = ".obj";
            StaticLibraryExtension = ".lib";
            SharedLibraryExtension = ".dll";
            ExecutableExtension = ".exe";
            ProgramDebugDatabaseExtension = ".pdb";
            PlatformType = PlatformType.Windows;
            PlatformName = "Windows";
        }

        private void InitForLinux()
        {
            // Although this may vary, we will enforce theses extensions
            ObjectFileExtension = ".o";
            StaticLibraryExtension = ".a";
            SharedLibraryExtension = ".so";
            ExecutableExtension = string.Empty;
            // Most debug applications on Linux have embedded pdbs...
            ProgramDebugDatabaseExtension = string.Empty;
            PlatformType = PlatformType.Linux;
            PlatformName = "Linux";
        }

        private void InitForMacOS() 
        {
            ObjectFileExtension = ".o";
            StaticLibraryExtension = ".a";
            SharedLibraryExtension = ".dylib";

            ExecutableExtension = ".app";
            ProgramDebugDatabaseExtension = ".dsym";
            PlatformType = PlatformType.MacApple;
            PlatformName = "macosx";
        }

        public string GetOutputFolderName( ConfigKind configKind )
        {
            // TODO: #Archs
            return string.Format( "{0}-{1}-x86_64", configKind.ToString(), PlatformName.ToLowerInvariant() );
        }
    }
}
