using System.Collections.Generic;
using System.IO;

using SaturnBuildTool.Tools;

namespace SaturnBuildTool
{
    public class Target
    {
        // Set by the user, in the .Target.cs file
        public ArchitectureKind[] Architectures { get; set; }

        // Set by the user, in the .Target.cs file
        public ConfigKind[] BuildConfigs { get; set; }

        // The output type when the Target/Module is fully built
        public LinkerOutput OutputType = LinkerOutput.Executable;

        // The name of this Target
        public string Name = null;

        public string OutputPath { get; set; }

        // Include directories
        public List<string> Includes = new List<string>();

        public List<string> PreprocessorDefines = new List<string>();

        // Additional library links (.lib on Windows)
        public List<string> Links = new List<string>();

        public List<string> DynamicBase = new List<string>();

        // Library search paths
        public List<string> LibraryPaths = new List<string>();

        /// <summary>
        /// The name of modules that this target needs in order to compile successfully.
        /// </summary>
        public List<string> Modules = new List<string>();

        public virtual void Init()
        {
            if( Shared.Platform.PlatformType == PlatformType.Windows )
            {
                PreprocessorDefines.AddRange( new string[] { "UNICODE", "_UNICODE", "SAT_PLATFORM_WINDOWS", "_CRT_SECURE_NO_WARNINGS" } );
            }
        }

        public List<string> GetIntermediateFiles()
        {
            List<string> intermediate = new List<string>();
            intermediate.AddRange( DirectoryTools.DirSearch( OutputPath, Shared.Platform.ObjectFileExtension ) );

            /*
            foreach( string file in CompiledInDirectlyObjects )
            {
                intermediate.AddRange( DirectoryTools.DirSearch( file, Shared.Platform.ObjectFileExtension ) );
            }
            */

            return intermediate;
        }

        /// <summary>
        /// A potential return could be:
        /// C:\Projects\MyProject\bin\Debug-windows-x86_64\MyProject
        /// </summary>
        /// <returns>
        /// The absolute binary directory
        /// </returns>
        public string GetBinDir()
        {
            // A potential dir could be:
            // C:\Projects\MyProject\bin\Debug-windows-x86_64\MyProject

            // NOTE: Bin dirs always use the Targets name, not the modules name (if this was a module)
            string BinDir = Shared.ProjectInfo.RootDirectory;
            BinDir = Path.Combine( BinDir, "bin" );
            switch( Shared.ProjectInfo.CurrentConfigKind )
            {
                case ConfigKind.Debug:
                    {
                        BinDir = Path.Combine( BinDir, "Debug-windows-x86_64" );
                    }
                    break;

                case ConfigKind.Release:
                    {
                        BinDir = Path.Combine( BinDir, "Release-windows-x86_64" );
                    }
                    break;

                case ConfigKind.Dist:
                    {
                        BinDir = Path.Combine( BinDir, "Dist-windows-x86_64" );
                    }
                    break;
            }

            BinDir = Path.Combine( BinDir, Shared.ProjectInfo.Name );

            return BinDir;
        }

        /// <summary>
        /// A potential return could be:
        /// C:\Projects\MyProject\bin\{build-config}\MyProject\MyModule.dll
        /// </summary>
        /// <returns>
        /// Returns the full absolute path to this modules/targets output file
        /// </returns>
        public string GetFullBinPath( string customFileName )
        {
            string BinDir = GetBinDir();

            switch( OutputType )
            {
                case LinkerOutput.StaticLibrary:
                    {
                        BinDir = Path.Combine( BinDir, customFileName );
                        BinDir = Path.ChangeExtension( BinDir, Shared.Platform.StaticLibraryExtension );
                    }
                    break;

                case LinkerOutput.SharedLibrary:
                    {
                        BinDir = Path.Combine( BinDir, customFileName );
                        BinDir = Path.ChangeExtension( BinDir, Shared.Platform.SharedLibraryExtension );
                    }
                    break;

                case LinkerOutput.Executable:
                    {
                        BinDir = Path.Combine( BinDir, customFileName );
                        BinDir = Path.ChangeExtension( BinDir, Shared.Platform.ExecutableExtension );
                    }
                    break;
            }

            return BinDir;
        }

        /// <summary>
        /// A potential return could be:
        /// C:\Projects\MyProject\bin\{build-config}\MyProject\MyModule.dll
        /// </summary>
        /// <returns>
        /// Returns the full absolute path to this modules/targets output file (default name)
        /// </returns>
        public virtual string GetFullPDBPath()
        {
            string BinDir = GetBinDir();

            BinDir = Path.Combine( BinDir, Shared.ProjectInfo.Name );
            BinDir = Path.ChangeExtension( BinDir, Shared.Platform.ProgramDebugDatabaseExtension );

            return BinDir;
        }

        /// <returns>
        /// Returns the full absolute path to this modules/targets debug database
        /// </returns>
        public virtual string GetFullBinPath()
        {
            return GetFullBinPath( Shared.ProjectInfo.Name );
        }
 
    }
}
