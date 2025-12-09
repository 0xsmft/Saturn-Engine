using System.Collections.Generic;
using System.IO;

namespace SaturnBuildTool
{
    public enum LinkerOutput
    {
        StaticLibrary,
        SharedLibrary,
        Executable
    }

    public class LinkSettings
    {
        /// <summary>
        /// If true: Adds /OPT:REF else /OPT:NOREF
        /// </summary>
        public bool RemoveUnreferncedFunctions = false;

        /// <summary>
        /// Adds /INCREMENTAL
        /// </summary>
        public bool IncrementalLink = false;

        /// <summary>
        /// Adds /DEBUG
        /// </summary>
        public bool DebugLink = false;

        /// <summary>
        /// The absolute output directory.
        /// </summary>
        public string OutputDirectory { get; set; }

        /// <summary>
        /// The absolute intermediate directory.
        /// </summary>
        public string IntermediateDirectory { get; set; }

        /// <summary>
        /// The absolute output path.
        /// </summary>
        public string OutputPath { get; set; }

        /// <summary>
        /// The name that will be outputted as the final result.
        /// Example: MyGame.exe or Saturn-MyGame.dll
        /// </summary>
        public string OutputName { get; set; }

        /// <summary>
        /// The name that will be outputted as the final result.
        /// Example: MyGame even if this link setting is used for a module the name will not include it's parents name
        /// </summary>
        public string Name { get; set; }

        public LinkerOutput OutputType { get; set; }

        /// <summary>
        /// Search paths to libraries
        /// </summary>
        public List<string> LibraryPaths { get; set; }

        /// <summary>
        /// All .lib files
        /// </summary>
        public List<string> Links { get; set; }

        /// <summary>
        /// Object files a.k.a the input files
        /// </summary>
        public List<string> ObjectFiles { get; set; } = new List<string>();

        /// <summary>
        /// See more: https://learn.microsoft.com/en-us/cpp/build/reference/dynamicbase
        /// </summary>
        public List<string> DynamicBases { get; set; }

        public string GetFullDebugDatabasePath()
        {
            string filename = Path.ChangeExtension( OutputName, Shared.Platform.ProgramDebugDatabaseExtension );
            return Path.Combine( OutputDirectory, filename );
        }

        public LinkSettings(
            bool removeUnreferncedFunctions,
            bool incrementalLink,
            bool debugLink,
            string outputPath,
            string outputName,
            string name,
            string intPath,
            LinkerOutput outputType,
            List<string> libraryPaths,
            List<string> links,
            List<string> dynamicBases )
        {
            RemoveUnreferncedFunctions = removeUnreferncedFunctions;
            IncrementalLink = incrementalLink;
            DebugLink = debugLink;
            OutputPath = outputPath;
            OutputName = outputName;
            OutputType = outputType;
            IntermediateDirectory = intPath;
            LibraryPaths = libraryPaths;
            Links = links;
            DynamicBases = dynamicBases;

            Name = name;
            switch( OutputType )
            {
                case LinkerOutput.StaticLibrary:
                    {
                        OutputName += Shared.Platform.StaticLibraryExtension;
                    }
                    break;

                case LinkerOutput.SharedLibrary:
                    {
                        OutputName += Shared.Platform.SharedLibraryExtension;
                    }
                    break;

                case LinkerOutput.Executable:
                    {
                        OutputName += Shared.Platform.ExecutableExtension;
                    }
                    break;
            }

            OutputDirectory = outputPath;
            OutputPath = Path.Combine( OutputDirectory, OutputName );
        }
    }
}
