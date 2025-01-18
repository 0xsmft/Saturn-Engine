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
    internal class ProjectInfo
    {
        public static readonly ProjectInfo Instance = new ProjectInfo();

        public string Name { get; set; }

        public string RootDirectory { get; set; }

        public string SourceDir { get; set; }

        // The directory where the "Build" folder are located.
        public string BuildDir { get; set; }

        // The directory where the ".Build.cs" files are located.
        public string TargetDir { get; set; }

        private string TargetPlatformName { get; set; }
        public ArchitectureKind TargetPlatformKind = ArchitectureKind.Unknown;

        private string ConfigName { get; set; }
        public ConfigKind CurrentConfigKind;

        public string FileCacheLocation { get; set; }

        // The current "*.Build.cs" file path.
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
            SourceDir = Path.Combine( RootDirectory, "Source" );
            SourceDir = Path.Combine( SourceDir, Name );
            SourceDir = SourceDir.Replace( "/", "\\" );

            // Target config file
            // So if we had a project it would be:
            // RootDirectory = C:\Projects\Example\
            // TargetDir = C:\Projects\Example\Source
            // As the source folder contains a folder with all of the source of the project
            // And then a file with the build rules.
            TargetDir = Path.Combine( RootDirectory, "Source" );
            TargetDir = TargetDir.Replace( "/", "\\" );

            // Build folder
            BuildDir = Path.Combine( RootDirectory, "Build" );
            BuildDir = BuildDir.Replace( "/", "\\" );

            // Filecache
            FileCacheLocation = Path.Combine( RootDirectory, "Filecache.fc" );

            GetTargetPlatform();
            GetConfigKind();

            FindBuildRuleFile();

            // Find Header tool location
            SaturnDir = CommandLineParser.Instance.FindValueFromKey( "SATURNDIR" );

            if( SaturnDir == string.Empty ) 
            {
                Console.WriteLine( "No override Saturn directory was set using location from SATURN_DIR Environment Variable." );
                SaturnDir = Environment.GetEnvironmentVariable( "SATURN_DIR" );
            }
            
            switch( CurrentConfigKind )
            {
                case ConfigKind.Debug:
                    {
                        HeaderToolExePath = SaturnDir + "\\bin\\Debug-windows-x86_64\\SaturnHeaderTool\\SaturnHeaderTool.exe";
                    }
                    break;

                case ConfigKind.Dist:
                case ConfigKind.Release:
                    {
                        HeaderToolExePath = SaturnDir + "\\bin\\Release-windows-x86_64\\SaturnHeaderTool\\SaturnHeaderTool.exe";
                    }
                    break;
            }

            return true;
        }

        private bool CheckAllArgs() 
        {
            bool result = true;

            if( 
                !CommandLineParser.Instance.HasArgument( "BUILD" ) || 
                !CommandLineParser.Instance.HasArgument( "REBUILD" ) || 
                !CommandLineParser.Instance.HasArgument( "CLEAN" ) ) 
            {
                Console.WriteLine( "Missing action command! (/BUILD /REBUILD /CLEAN)" );
                result = false;
            }

            if(
                !CommandLineParser.Instance.HasArgument( "DEBUG" ) ||
                !CommandLineParser.Instance.HasArgument( "RELEASE" ) ||
                !CommandLineParser.Instance.HasArgument( "DIST" ) )
            {
                Console.WriteLine( "Missing config command! (/DEBUG /RELEASE /DIST)" );
                result = false;
            }

            if( !CommandLineParser.Instance.HasArgument( "NAME" ) )
            {
                Console.WriteLine( "Missing project name command! (/NAME)" );
                result = false;
            }

            if( !CommandLineParser.Instance.HasArgument( "Win64" ) )
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
                Console.WriteLine( "Please use the help command (/HELP) for more information on the command line arguments." );
            }

            return result;
        }

        private void GetConfigKind()
        {
            if( CommandLineParser.Instance.FindFlag( "Debug" ) )
            {
                ConfigName = "Debug";
                CurrentConfigKind = ConfigKind.Debug;
            }
            else if( CommandLineParser.Instance.FindFlag( "Release" ) )
            {
                ConfigName = "Release";
                CurrentConfigKind = ConfigKind.Release;
            }
            else if( CommandLineParser.Instance.FindFlag( "Dist" ) )
            {
                ConfigName = "Dist";
                CurrentConfigKind = ConfigKind.Dist;
            }
        }

        private void GetTargetPlatform()
        {
            if( CommandLineParser.Instance.FindFlag( "Win64" ) )
            {
                TargetPlatformName = "Win64";
                TargetPlatformKind = ArchitectureKind.x64;
            }
            else if( CommandLineParser.Instance.FindFlag( "Win86" ) ) 
            {
                TargetPlatformName = "Win86";
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
                        BuildRuleFile = TargetDir;
                        BuildRuleFile += string.Format( "\\{0}.Build.cs", Name );

                        BuildRuleFile = BuildRuleFile.Replace( "/", "\\" );
                    }
                    break;

                case ConfigKind.Dist:
                    {
                        BuildRuleFile = TargetDir;
                        BuildRuleFile += string.Format( "\\{0}.RT_Build.cs", Name );

                        BuildRuleFile = BuildRuleFile.Replace( "/", "\\" );
                    }
                    break;
            }
        }
    }
}
