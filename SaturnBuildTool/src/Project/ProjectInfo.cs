using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

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

        // Setup the ProjectInfo
        // Args:
        // 0: The Action, BUILD, REBULD, CLEAN. TODO
        // 1: The project name
        // 2: The target platform, Win64
        // 3: The configuration, Debug, Release, Dist
        // 4: The project location
        public void Setup( string[] Args ) 
        {
            string RootDir = Args[4];

            // Get the project directory.
            int index = RootDir.IndexOf( "/" );
            RootDirectory = RootDir.Substring( index + 1 );
            RootDirectory = RootDirectory.Replace( "/", "\\" );

            // Name
            Name = Args[1];
            index = Name.IndexOf("/");
            Name = Name.Substring( index + 1);

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
            TargetDir = TargetDir.Replace("/", "\\");

            // Build folder
            BuildDir = Path.Combine( RootDirectory, "Build" );
            BuildDir = BuildDir.Replace( "/", "\\" );

            // Filecache
            FileCacheLocation = Path.Combine( RootDirectory, "Filecache.fc" );

            TargetPlatformName = Args[2];
            index = TargetPlatformName.IndexOf("/");
            TargetPlatformName = TargetPlatformName.Substring(index + 1);

            StringToTargetPlatform();

            ConfigName = Args[3];
            index = ConfigName.IndexOf("/");
            ConfigName = ConfigName.Substring(index + 1);

            StringToConfigKind();

            FindBuildRuleFile();

            // Find Header tool location
            string saturnLocation = Environment.GetEnvironmentVariable( "SATURN_DIR" );
            switch( CurrentConfigKind ) 
            {
                case ConfigKind.Debug:
                    {
                       HeaderToolExePath = saturnLocation + "\\bin\\Debug-windows-x86_64\\SaturnHeaderTool\\SaturnHeaderTool.exe";
                    } break;

                case ConfigKind.Release:
                    {
                        HeaderToolExePath = saturnLocation + "\\bin\\Release-windows-x86_64\\SaturnHeaderTool\\SaturnHeaderTool.exe";
                    }
                    break;

                case ConfigKind.Dist:
                    {
                        HeaderToolExePath = saturnLocation + "\\bin\\Dist-windows-x86_64\\SaturnHeaderTool\\SaturnHeaderTool.exe";
                    }
                    break;
            }
        }

        private void StringToConfigKind() 
        {
            if (ConfigName == "Debug")
            {
                CurrentConfigKind = ConfigKind.Debug;
            }
            else if (ConfigName == "Release")
            {
                CurrentConfigKind = ConfigKind.Release;
            }
            else if (ConfigName == "Dist")
            {
                CurrentConfigKind = ConfigKind.Dist;
            }
        }

        private void StringToTargetPlatform() 
        {
            if (TargetPlatformName == "Win64")
                TargetPlatformKind = ArchitectureKind.x64;
            else if (TargetPlatformName == "Win86")
                TargetPlatformKind = ArchitectureKind.x86;
        }

        private void FindBuildRuleFile() 
        {
            switch (CurrentConfigKind)
            {
                case ConfigKind.Debug:
                case ConfigKind.Release:
                    {
                        BuildRuleFile = TargetDir;
                        BuildRuleFile += string.Format( "\\{0}.Build.cs", Name );

                        BuildRuleFile = BuildRuleFile.Replace( "/", "\\" );
                    } break;

                case ConfigKind.Dist:
                    {
                        BuildRuleFile = TargetDir;
                        BuildRuleFile += string.Format("\\{0}.RT_Build.cs", Name);

                        BuildRuleFile = BuildRuleFile.Replace("/", "\\");
                    }
                    break;
            }
        }
    }
}
