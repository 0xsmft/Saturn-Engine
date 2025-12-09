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
                PreprocessorDefines.AddRange( new string[] { "UNICODE", "_UNICODE", "SAT_PLATFORM_WINDOWS" } );
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

        private static Module CompileAndLoadModule( string path )
        {
            return null;

            /*
            // Compile the *.Build.cs file
            Assembly asm = CSharpCompiler.CompileCSharpFiles( path );

            Module module = null;
            Type[] types = asm.GetTypes();
            for( var i = 0; i < types.Length; i++ )
            {
                var type = types[ i ];

                if( !type.IsClass || type.IsAbstract )
                {
                    continue;
                }

                if( type.IsSubclassOf( typeof( Module ) ) )
                {
                    module = ( Module ) Activator.CreateInstance( type );
                    module.Init();

                    string outDir = Shared.ProjectInfo.RootDirectory;
                    switch( Shared.ProjectInfo.CurrentConfigKind )
                    {
                        case ConfigKind.Debug:
                            {
                                outDir = Path.Combine( outDir, "bin-int" );
                                outDir = Path.Combine( outDir, "Debug-windows-x86_64" );
                            }
                            break;

                        case ConfigKind.Release:
                            {
                                outDir = Path.Combine( outDir, "bin-int" );
                                outDir = Path.Combine( outDir, "Release-windows-x86_64" );
                            }
                            break;

                        case ConfigKind.Dist:
                            {
                                outDir = Path.Combine( outDir, "bin-int" );
                                outDir = Path.Combine( outDir, "Dist-windows-x86_64" );
                            }
                            break;
                        default:
                            break;
                    }

                    outDir = Path.Combine( outDir, Shared.ProjectInfo.Name );
                    outDir = Path.Combine( outDir, module.Name );
                    module.OutputPath = outDir;

                    if( !Directory.Exists( outDir ) )
                        Directory.CreateDirectory( outDir );
                }
            }

            return module;
            */
        }

        public static Target SetupUserTarget()
        {
            /*
            string BuildFile = Shared.ProjectInfo.BuildRuleFile;

            Assembly asm = CSharpCompiler.CompileCSharpFile( BuildFile );
            if( asm == null )
                return null;

            BuildTarget target = null;
            Type[] types = asm.GetTypes();
            for( var i = 0; i < types.Length; ++i )
            {
                var type = types[ i ];
                if( !type.IsClass || type.IsAbstract )
                {
                    continue;
                }

                if( type.IsSubclassOf( typeof( BuildTarget ) ) )
                {
                    // Create a user target.
                    target = ( BuildTarget ) Activator.CreateInstance( type );

                    string outDir = Shared.ProjectInfo.RootDirectory;
                    switch( Shared.ProjectInfo.CurrentConfigKind )
                    {
                        case ConfigKind.Debug:
                            {
                                outDir = Path.Combine( outDir, "bin-int" );
                                outDir = Path.Combine( outDir, "Debug-windows-x86_64" );
                            }
                            break;

                        case ConfigKind.Release:
                            {
                                outDir = Path.Combine( outDir, "bin-int" );
                                outDir = Path.Combine( outDir, "Release-windows-x86_64" );
                            }
                            break;

                        case ConfigKind.Dist:
                            {
                                outDir = Path.Combine( outDir, "bin-int" );
                                outDir = Path.Combine( outDir, "Dist-windows-x86_64" );
                            }
                            break;
                        default:
                            break;
                    }
                    outDir = Path.Combine( outDir, Path.GetFileNameWithoutExtension( BuildFile ) );

                    // Remove .Build
                    int index = outDir.LastIndexOf( '.' );
                    outDir = outDir.Substring( 0, index );

                    target.OutputPath = outDir;

                    // Init the target.
                    target.Init();

                    // Dependencies
                    var moduleCSFiles = DirectoryTools.CsSourceSearch( Shared.ProjectInfo.SourceDir, DirectoryTools.CSharpSearchOptions.Modules );
                    foreach( string moduleFile in moduleCSFiles )
                    {
                        var compiledModule = CompileAndLoadModule( moduleFile );
                        var binDir = compiledModule.GetBinDir();

                        if( target.Modules.Contains( compiledModule.Name ) ) 
                        {
                            target.RealModules.Add( compiledModule );
                            if( compiledModule.CompiledInDirectly )
                            {
                                target.CompiledInDirectlyObjects.Add( compiledModule.OutputPath );
                            }
                            else
                            {
                                target.LibraryPaths.Add( binDir );
                            }
                        }
                        else 
                        {
                            Console.WriteLine( $"Module file: {moduleFile} was found but not referenced in the target, skipping..." );
                        }
                    }

                    foreach( var dep in target.RealModules ) 
                    {
                        // Now try to init their mods, if needed
                        foreach( var depMod in dep.Modules )
                        {
                            // If this module is already loaded then all we need to do is find it
                            if( target.Modules.Contains( depMod ) ) 
                            {
                                dep.Modules.Add( target.RealModules.GetRange() );
                            }
                        }
                    }

                        target.SortModules();

                    target.Modules.Clear();
                }
            }

            return target;
            */

            return null;
        }
    }
}
