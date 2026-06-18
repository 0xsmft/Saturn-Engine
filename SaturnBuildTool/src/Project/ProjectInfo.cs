using System;
using System.IO;

using SaturnBuildTool.Auxiliary;

namespace SaturnBuildTool
{
    public enum ArchitectureKind
    {
        x86_64,
        AArch64,
        Unknown
    }

    public enum ConfigKind
    {
        Debug,
        Release,
        Dist
    }

    // This class holds all of our needed information about the current project that is being built.
    // There should only ever be one of these created in the Build Tool.
    public class ProjectInfo
    {
        public string Name { get; set; }

        public string RootDirectory { get; set; }

        public string SourceDir { get; set; }
        
        public string SourceRootDir { get; set; }

        // The directory where the "Build" folder are located.
        public string BuildDir { get; set; }

        // The directory where the ".Gen.cpp/.h" files are located, created by the HeaderTool
        public string HeaderToolGeneratedRootPath { get; set; }

        // The directory where the ".Build.cs" files are located.
        public string TargetDir { get; set; }

        public ArchitectureKind TargetArchitectureKind = ArchitectureKind.Unknown;

        public ConfigKind CurrentConfigKind;

        public ToolchainType ToolchainTypeToUse = ToolchainType.Unknown;

        public string FileCacheLocation { get; set; }

        public string LinkCacheLocation { get; set; }

        // The current "*.{config}.cs" file path.
        public string BuildRuleFile { get; set; }

        // Path to header tool exe
        public string HeaderToolExePath { get; set; }

        public string SaturnDir { get; set; }

        // Setup the ProjectInfo
        public bool Setup()
        {
            if( !CheckAllArgs() ) return false;

            RootDirectory = CommandLineParser.Instance.FindValueFromKey( "PROJECT" );

            // Name
            Name = CommandLineParser.Instance.FindValueFromKey( "NAME" );

            // Source
            // Default to "Source" unless specified, /SRC={...}
            if( CommandLineParser.Instance.HasArgument( "SRC" ) )
            {
                // Source dir passed in from the CLI is relative to the .sln file / .sproject file
                SourceRootDir = SourceDir = Path.Combine( RootDirectory, CommandLineParser.Instance.FindValueFromKey( "SRC" ) );

                if( Shared.Platform.PlatformType == PlatformType.Windows ) 
                {
                    SourceDir = SourceDir.Replace( "/", "\\" );
                }
            }
            else
            {
                SourceRootDir = SourceDir = Path.Combine( RootDirectory, "Source" );
                SourceDir = Path.Combine( SourceDir, Name );
            }

            if( !Directory.Exists( SourceDir ) )
            {
                Console.WriteLine( $"Source directory \"{SourceDir}\" does not exist!" );
                return false;
            }

            // Target config file
            // So if we had a project it would be:
            // RootDirectory = C:\Projects\Example
            // TargetDir = C:\Projects\Example\Source
            // As the source folder contains a folder with all of the source of the project
            TargetDir = RootDirectory;

            // Build folder
            BuildDir = Path.Combine( RootDirectory, "Build" );
            HeaderToolGeneratedRootPath = Path.Combine( BuildDir, "Generated" );

            if( !Directory.Exists( BuildDir ) )
            {
                Console.WriteLine( $"WARN: Build directory \"{BuildDir}\" does not exist!" );
                Directory.CreateDirectory( BuildDir );
            }

            GetTargetPlatform();
            GetConfigKind();

            if( !GetToolchainType() ) return false;

            // Filecache
            FileCacheLocation = Path.Combine( BuildDir, $"Filecache-{CurrentConfigKind}.fc" );
            if( !File.Exists( FileCacheLocation ) )
            {
                Console.WriteLine( $"File cache does not exist looking for \"{SourceDir}\" Resulting in a new FileCache being used." );
            }

            // Link cache
            LinkCacheLocation = Path.Combine( BuildDir, $"Linkcache-{CurrentConfigKind}.fc" );
            if( !File.Exists( LinkCacheLocation ) )
            {
                Console.WriteLine( $"Link cache does not exist looking for \"{SourceDir}\" Resulting in a new FileCache being used." );
            }

            FindBuildRuleFile();

            // Find Header tool location
            SaturnDir = CommandLineParser.Instance.FindValueFromKey( "SATURNDIR" );

            if( SaturnDir == null || SaturnDir == string.Empty )
            {
                Console.WriteLine( "No override Saturn directory was suggested, using location from \"SATURN_DIR\" Environment Variable." );
                SaturnDir = Environment.GetEnvironmentVariable( "SATURN_DIR" );
            }

            switch( CurrentConfigKind )
            {
                case ConfigKind.Debug:
                    {
                        HeaderToolExePath = Path.Combine( SaturnDir, "bin", "Debug-windows-x86_64", "SaturnHeaderTool", "SaturnHeaderTool.exe" );
                    }
                    break;

                case ConfigKind.Dist:
                case ConfigKind.Release:
                    {
                        // Always use the release build on Dist
                        HeaderToolExePath = Path.Combine( SaturnDir, "bin", "Release-windows-x86_64", "SaturnHeaderTool", "SaturnHeaderTool.exe" );
                    }
                    break;
            }

            return true;
        }

        private bool CheckAllArgs()
        {
            bool result = true;

            if(
                ( CommandLineParser.Instance.HasArgument( "BUILD" ) ? 1 : 0 ) +
                ( CommandLineParser.Instance.HasArgument( "REBUILD" ) ? 1 : 0 ) +
                ( CommandLineParser.Instance.HasArgument( "CLEAN" ) ? 1 : 0 ) != 1
            )
            {
                Console.WriteLine( "Exactly one action command must be suggested! (/BUILD, /REBUILD, /CLEAN)" );
                result = false;
            }

            if(
                ( CommandLineParser.Instance.HasArgument( "DEBUG" ) ? 1 : 0 ) +
                ( CommandLineParser.Instance.HasArgument( "RELEASE" ) ? 1 : 0 ) +
                ( CommandLineParser.Instance.HasArgument( "DIST" ) ? 1 : 0 ) != 1
            )
            {
                Console.WriteLine( "Exactly one config command must be suggested! (/DEBUG, /RELEASE, /DIST)" );
                result = false;
            }

            if(
                ( CommandLineParser.Instance.HasArgument( "WIN64" ) ? 1 : 0 ) +
                ( CommandLineParser.Instance.HasArgument( "LINUX64" ) ? 1 : 0 ) +
                ( CommandLineParser.Instance.HasArgument( "APPLE" ) ? 1 : 0 ) != 1
            )
            {
                Console.WriteLine( "Exactly one operating system command must be suggested! (/WIN64, /LINUX64, /APPLE)" );
                result = false;
            }

            if( !CommandLineParser.Instance.HasArgument( "NAME" ) )
            {
                Console.WriteLine( "Missing project name command! (/NAME)" );
                result = false;
            }

            if( !CommandLineParser.Instance.HasArgument( "WIN64" ) )
            {
                Console.WriteLine( "Missing target platform command! (/Win64)" );
                result = false;
            }

            if( !CommandLineParser.Instance.HasArgument( "PROJECT" ) )
            {
                Console.WriteLine( "Missing project path command! (/PROJECT)" );
                result = false;
            }

            if( !result )
            {
                Console.WriteLine( "Please suggest the help command (/HELP) for more information on the command line arguments." );
            }

            return result;
        }

        private void GetConfigKind()
        {
            if( CommandLineParser.Instance.FindFlag( "Debug" ) )
            {
                CurrentConfigKind = ConfigKind.Debug;
            }
            else if( CommandLineParser.Instance.FindFlag( "Release" ) )
            {
                CurrentConfigKind = ConfigKind.Release;
            }
            else if( CommandLineParser.Instance.FindFlag( "Dist" ) )
            {
                CurrentConfigKind = ConfigKind.Dist;
            }
        }

        private bool GetToolchainType() 
        {
            string toolchainSpecifiedInCmd = CommandLineParser.Instance.FindValueFromKey( "CC" );

            // If its null, then /CC isn't suggested so we pick the best depending on the target OS.
            if( toolchainSpecifiedInCmd == null ) 
            {
                switch( Shared.Platform.PlatformType ) 
                {
                    case PlatformType.Unknown:
                        {
                            Console.WriteLine( "ERROR: Trying to get tool-chain via the OS but Shared.TargetOperatingSystem is unknown!" );
                        } return false;

                    case PlatformType.Windows:
                        {
                            ToolchainTypeToUse = ToolchainType.MSVC;
                        } break;

                    case PlatformType.Linux:
                    case PlatformType.MacApple:
                        {
                            ToolchainTypeToUse = ToolchainType.Clang;
                        } break;
                }
            }
            else
            {
                if( toolchainSpecifiedInCmd == "MSVC" )
                {
                    ToolchainTypeToUse = ToolchainType.MSVC;
                }
                else if( toolchainSpecifiedInCmd == "CLANG" )
                {
                    ToolchainTypeToUse = ToolchainType.Clang;
                }
                else if( toolchainSpecifiedInCmd == "GCC" )
                {
                    ToolchainTypeToUse = ToolchainType.GCC;
                }
                else 
                {
                    Console.WriteLine( $"ERROR: '{toolchainSpecifiedInCmd}' is not a valid toolchain type!" );
                    return false;
                }
            }

            return true;
        }

        private void GetTargetPlatform()
        {
            if( CommandLineParser.Instance.FindFlag( "Win64" ) )
            {
                TargetArchitectureKind = ArchitectureKind.x86_64;
            }
            else if( CommandLineParser.Instance.FindFlag( "LINUX64" ) )
            {
                TargetArchitectureKind = ArchitectureKind.AArch64;
            }
            else if( CommandLineParser.Instance.FindFlag( "APPLE" ) )
            {
                TargetArchitectureKind = ArchitectureKind.AArch64;
            }
        }

        private void FindBuildRuleFile()
        {
            switch( CurrentConfigKind )
            {
                case ConfigKind.Debug:
                case ConfigKind.Release:
                    {
                        BuildRuleFile = Path.Combine( SourceDir, $"{Name}.Development.cs" );
                    }
                    break;
                case ConfigKind.Dist:
                    {
                        BuildRuleFile = Path.Combine( SourceDir, $"{Name}.Dist.cs" );
                    }
                    break;
            }
        }
    }
}
