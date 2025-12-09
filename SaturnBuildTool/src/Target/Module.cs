using System.Collections.Generic;
using System.IO;

namespace SaturnBuildTool
{
    public class Module
    {
        // Set by the user, in the .Target.cs file
        public ConfigKind[] BuildConfigs { get; set; }

        // The output type when the Target/Module is fully built
        public LinkerOutput OutputType = LinkerOutput.Executable;

        public SourceDirectoryOptions SourceDirectoryOptions { get; set; } = SourceDirectoryOptions.Default;

        public OutputDirectoryOptions OutputDirectoryOptions { get; set; } = OutputDirectoryOptions.Default;

        // The name of this Target
        public string Name = null;

        public List<string> SourcePaths = new List<string>();

        // Should this module be compiled in directly to the Target.
        public bool CompiledInDirectly = false;

        public struct PCHInfo
        {
            public string HeaderFile;
            public string SourceFile;

            public PCHInfo( string headerPath, string sourcePath )
            {
                HeaderFile = headerPath;
                SourceFile = sourcePath;

                if( Shared.Platform.PlatformType == PlatformType.Windows )
                {
                    SourceFile = SourceFile.Replace( "/", "\\" );
                }

                SourceFile = Path.Combine( Shared.ProjectInfo.RootDirectory, SourceFile );
            }

            public bool Valid()
            {
                return HeaderFile != null && SourceFile != null;
            }
        }

        public PCHInfo PCH;

        // Include directories
        public List<string> Includes = new List<string>();

        public List<string> PreprocessorDefines = new List<string>();

        // Additional library links (.lib on Windows)
        public List<string> Links = new List<string>();

        public List<string> DynamicBase = new List<string>();

        // Library search paths
        public List<string> LibraryPaths = new List<string>();

        public List<string> Modules = new List<string>();

        public virtual void Init()
        {
        }
    }
}
