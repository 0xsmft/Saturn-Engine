using System;
using System.IO;

using SaturnBuildTool.Auxiliary;

namespace SaturnBuildTool
{
    public enum ArchitectureKind
    {
        x64,
        x86,
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

        // The directory where the "Build" folder are located.
        public string BuildDir { get; set; }

        // The directory where the ".Gen.cpp/.h" files are located, created by the HeaderTool
        public string HeaderToolGeneratedRootPath { get; set; }

        // The directory where the ".Build.cs" files are located.
        public string TargetDir { get; set; }

        public ArchitectureKind TargetPlatformKind = ArchitectureKind.Unknown;

        public ConfigKind CurrentConfigKind;

        public string FileCacheLocation { get; set; }

        // The current "*.{config}.cs" file path.
        public string BuildRuleFile { get; set; }

        // Path to header tool exe
        public string HeaderToolExePath { get; set; }

        public string SaturnDir { get; set; }

        // Setup the ProjectInfo
        // Args:
        // 0: The Action, BUILD, REBULD, CLEAN. TODO
        // 1: The project name
        // 2: The target platform, Win64
        // 3: The configuration, Debug, Release, Dist
        // 4: The project location
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
                SourceDir = Path.Combine( RootDirectory, CommandLineParser.Instance.FindValueFromKey( "SRC" ) );

                if( Shared.Platform.PlatformType == PlatformType.Windows ) 
                {
                    SourceDir = SourceDir.Replace( "/", "\\" );
                }
            }
            else
            {
                SourceDir = Path.Combine( RootDirectory, "Source" );
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

            // Filecache
            FileCacheLocation = Path.Combine( BuildDir, $"Filecache-{CurrentConfigKind}.fc" );

            if( !File.Exists( FileCacheLocation ) )
            {
                Console.WriteLine( $"File cache does not exist looking for \"{SourceDir}\" Resulting in a new FileCache being used." );
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
                Console.WriteLine( "Please suggested the help command (/HELP) for more information on the command line arguments." );
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

        private void GetTargetPlatform()
        {
            if( CommandLineParser.Instance.FindFlag( "Win64" ) )
            {
                TargetPlatformKind = ArchitectureKind.x64;
            }
            else if( CommandLineParser.Instance.FindFlag( "Win86" ) )
            {
                TargetPlatformKind = ArchitectureKind.x86;
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
