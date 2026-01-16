using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using SaturnBuildTool.Auxiliary;
using SaturnBuildTool.Cache;
using SaturnBuildTool.Tools;

namespace SaturnBuildTool
{
    public enum ActionType
    {
        Build,
        Rebuild,
        Clean
    }

    internal class Application
    {
        public int ExitCode = 0;
        private readonly List<string> Args;

        private bool HasCompiledAnyFile = false;

        private bool TargetPendingLink = false;

        private uint NumTasksFailed = 0;

        private readonly List<List<string>> FilesPerThread = new List<List<string>>();
        private readonly List<bool> ThreadsCompleted = new List<bool>();

        private ActionType Action = ActionType.Build;

        private List<string> RecipeFiles = new List<string>();

        private Dictionary<string, List<string>> ModuleToFiles = new Dictionary<string, List<string>>();

        private Dictionary<string, ToolchainBase> ModuleToToolchain = new Dictionary<string, ToolchainBase>();

        private Queue<string> SortedModules = new Queue<string>();
        private HashSet<string> _vistedModules = new HashSet<string>();

        // The build tool version is 0.0.5,
        // however, SBT 5.1 means that is the modular, task-cache, dependency tracking version and C# build rules,
        // as there is a version before this that was private and shipped with Saturn 0.2.2 that had initial dependency tracking and task-cache but that version
        // was superseded by this one.
        //
        // To sum it up:
        // 0.0.4 -> created a private "engine test" version with the features listed above, this version shipped in Saturn 0.2.2 although source code was SBT 0.0.4, this of the BuildTool was started before 0.2.2 was due to release.
        // Engine Test -> created buildtool-x0.0.5 was very similar to engine test but it used premake instead of custom C# build rules.
        // buildtool-x0.0.5 -> created sbt-5.1 branch which is this version now.
        //
        private readonly string StartupMessage = "Saturn Build Tool X0.0.5 \"SBT 5.1\" (Engine Version: 0.2.3 8195)";

        public Application( string[] args )
        {
            Args = new List<string>();
            Args.AddRange( args );
        }

        public bool Init()
        {
            CommandLineParser.Instance.Parse( Args );

            // Set Action
            if( CommandLineParser.Instance.FindFlag( "BUILD" ) )
                Action = ActionType.Build;
            else if( CommandLineParser.Instance.FindFlag( "REBUILD" ) )
                Action = ActionType.Rebuild;
            else if( CommandLineParser.Instance.FindFlag( "CLEAN" ) )
                Action = ActionType.Clean;
            else
            {
                Console.WriteLine( "ERROR: No appropriate action command found!, must suggest either (/BUILD or /REBUILD or /CLEAN)" );
                ExitCode = 1;
                return false;
            }

            // Handle command lines args
            bool safeToContinue = false;
            if( CheckArgs() )
            {
                switch( Action )
                {
                    default: Debugger.Break(); break;

                    case ActionType.Build:
                    case ActionType.Rebuild:
                        {
                            safeToContinue = InitForBuilding();
                        } break;
                 
                    case ActionType.Clean:
                        safeToContinue = InitForCleaning();
                        break;
                }
            }

            return safeToContinue;
        }

        private bool CheckArgs()
        {
            if( CommandLineParser.Instance.FindFlag( "HELP" ) )
            {
                Console.WriteLine( $"Help for {StartupMessage}" );
                Console.WriteLine( "Options:" );
                Console.WriteLine( " Action Options:" );
                Console.WriteLine( "  /BUILD*          -- build the project" );
                Console.WriteLine( "  /REBUILD*        -- rebuild the project, ignoring the FileCache and TaskCache" );
                Console.WriteLine( "  /CLEAN*          -- clean the project" );
                Console.WriteLine( " Project Options:" );
                Console.WriteLine( "  /PROJECT*        -- path to the project root dir (same place where the .sproject file is located)" );
                Console.WriteLine( "  /NAME*           -- project name MUST match with the .sproject file name!" );
                Console.WriteLine( "  /SATURNDIR       -- override the Saturn Root Directory by default the build tool will use the \"SATURN_DIR\" environment variable" );
                Console.WriteLine( "  /SRC             -- override the Source Dir, by default its \"Source/{prj.name}\", when overriding make sure the path is relative to the .sproject path" );
                Console.WriteLine( " Compile Options:" );
                Console.WriteLine( "  /WIN64*          -- build for Windows x64" );
                Console.WriteLine( "  /HOTRELOAD       -- this is an internal command and is used for hot reloading, when this command is suggested the build tool will create a special timestamp file and output files with the timestamp suffix" );
                Console.WriteLine( "  /DISTASDBG       -- Build for Dist but compile with debug symbols and full \"/DIST\" must be suggested" );
                Console.WriteLine( "  Configuration Options:" );
                Console.WriteLine( "   /DEBUG*         -- build the project for Debug configuration with full symbols" );
                Console.WriteLine( "   /RELEASE*       -- build the project for Release configuration with symbols on for this project but symbols off for third party projects" );
                Console.WriteLine( "   /DIST*          -- build the project for the Dist configuration" );
                Console.WriteLine( "  Warning Options:" );
                Console.WriteLine( "   /XW+            -- treat warnings as errors" );
                Console.WriteLine( "   /XW-            -- no warnings" );
                Console.WriteLine( "   /XW1            -- warnings level one" );
                Console.WriteLine( "   /XW2            -- warnings level two" );
                Console.WriteLine( "   /XW3            -- warnings level three (default)" );
                Console.WriteLine( "   /XW4            -- warnings level four" );
                Console.WriteLine( "   /XWAx           -- all warnings" );
                Console.WriteLine( " Auxiliary Options:" );
                Console.WriteLine( "  /HELP            -- this command that displays the help message" );
                Console.WriteLine( "  /INCLUDESTREE    -- displays and create an include tree" );
                Console.WriteLine( "  /VERISON         -- displays the version for the build tool" );
                Console.WriteLine( "  /ARGS+           -- displays the compiler and linker command line arguments" );
                Console.WriteLine( "  /EXPORTFILECACHE -- exports the FileCache into a human readable format" );
                Console.WriteLine( "  /EXPORTTASKCACHE -- exports the TaskCache into a human readable format" );
                Console.WriteLine( "  /SHOWARGS        -- show the command line arguments for the BuildTool" );
                Console.WriteLine( " * indicates required argument" );
                Console.WriteLine( " Example:" );
                Console.WriteLine( "  SaturnBuildTool /BUILD /WIN64 /PROJECT:{path_to_prj_root} /NAME:MyProject /DEBUG" );

                return false;
            }

            if( CommandLineParser.Instance.FindFlag( "VERSION" ) )
            {
                Console.WriteLine( $"{StartupMessage}" );
                return false;
            }

            if( CommandLineParser.Instance.GetComamndCount() == 0 )
            {
                Console.WriteLine( "You must provide more than one argument! Try running /HELP for more" );

                // treat as error
                ExitCode = 1;
                return false;
            }

            if( CommandLineParser.Instance.FindFlag( "SHOWARGS" ) )
            {
                CommandLineParser.Instance.PrintAllArgs();
            }

            return true;
        }

        private bool InitForBuilding()
        {
            Console.WriteLine( $"==== {StartupMessage} ====" );

            Shared.Platform = new Platform( CommandLineParser.Instance.FindPlatformCmd() );

            // Setup project info from args.
            Shared.ProjectInfo = new ProjectInfo();
            if( !Shared.ProjectInfo.Setup() )
            {
                Console.WriteLine( "ERROR: Project initialisation failed." );

                ExitCode = 1;
                return false;
            }

            // Find and compile rules
            Shared.RulesAssembly = RulesAssembly.CompileRules();

            if( Shared.TargetToBuild == null )
            {
                Console.WriteLine( $"ERROR: The target file: {Shared.ProjectInfo.BuildRuleFile}, failed to compile or it doesn't exist!" );

                ExitCode = 1;
                return false;
            }

            // Convert rules into proper buildable items.
            Shared.CurrentBuildTarget = BuildTarget.Create( Shared.TargetToBuild );

            // TODO: Move this into CommandLineParser
            // Maybe CommandLineParser.VerifyAll
            if( CommandLineParser.Instance.FindFlag( "DISTASDBG" ) && Shared.ProjectInfo.CurrentConfigKind != ConfigKind.Dist )
            {
                Console.WriteLine( "ERROR: \"/DISTASDBG\" was suggested however, you aren't building for Dist! \"/DISTASDBG\" is only available when \"/DIST\" is suggested" );

                ExitCode = 1;
                return false;
            }

            if( CommandLineParser.Instance.FindFlag( "showconsole" ) && Shared.ProjectInfo.CurrentConfigKind != ConfigKind.Dist )
            {
                Console.WriteLine( "ERROR: \"/SHOWCONSOLE\" was suggested however, you aren't building for Dist! \"/showconsole\" is only available when \"/DIST\" is suggested" );

                ExitCode = 1;
                return false;
            }

            // TODO: We only support building for Windows
            switch( Shared.ProjectInfo.TargetPlatformKind )
            {
                case ArchitectureKind.x86:
                case ArchitectureKind.x64:
                    {
                        Shared.Toolchain = new MSVCToolchain();
                    }
                    break;

                default:
                    break;
            }

            Shared.FileCache = FileCache.Load();
            Shared.TaskCache = TaskCache.Load();

            return true;
        }

        private bool InitForCleaning() 
        {
            Console.WriteLine( $"==== {StartupMessage} ====" );

            Shared.Platform = new Platform( CommandLineParser.Instance.FindPlatformCmd() );

            // Setup project info from args.
            Shared.ProjectInfo = new ProjectInfo();
            if( !Shared.ProjectInfo.Setup() )
            {
                Console.WriteLine( "ERROR: Project initialisation failed." );

                ExitCode = 1;
                return false;
            }

            Shared.FileCache = FileCache.Load();
            Shared.TaskCache = TaskCache.Load();

            return true;
        }

        private void CompileSingeFileUnchecked( CompileSettings compileSettings, ToolchainBase toolchain, string file )
        {
            int exitCode = toolchain.Compile( file, compileSettings );
            Shared.FileCache.CacheFile( file );

            if( exitCode == 0 )
            {
                HasCompiledAnyFile = true;
            }
            else
            { 
                ++NumTasksFailed;
                Console.WriteLine( $"SBT: ERR: UNABLE TO COMPILE FILE: CL {file}" );
            }
        }

        private void SearchForFiles()
        {
            string targetDir = Path.GetDirectoryName( Shared.ProjectInfo.BuildRuleFile );
            foreach( var kv in Shared.CurrentBuildTarget.Modules )
            {
                List<string> searchPaths = new List<string>();
                switch( kv.Value.ModuleRules.SourceDirectoryOptions )
                {
                    default:
                    case SourceDirectoryOptions.Default:
                        {
                            // No source path provided, default to target source path.
                            searchPaths.Add( Path.Combine( targetDir, Shared.ProjectInfo.Name, kv.Value.ModuleRules.Name ) );
                        }
                        break;

                    case SourceDirectoryOptions.Custom:
                        {
                            searchPaths.AddRange( kv.Value.ModuleRules.SourcePaths );
                        } break;

                    case SourceDirectoryOptions.UseTargetDirectory:
                        {
                            searchPaths.Add( targetDir );
                        } break;
                }

                List<string> allCppFilesModule = new List<string>();
                foreach( var path in searchPaths )
                {
                    allCppFilesModule.AddRange( DirectoryTools.CppSourceSearch( Path.Combine( Shared.ProjectInfo.RootDirectory, path ), true ) );
                }

                ModuleToToolchain.Add( kv.Key, new MSVCToolchain() );

                string[] sourceFileExts = { ".cpp", ".cc", ".cxx", ".c" };
                string[] headerFileExts = { ".h", ".hpp" };

                // Now filter the files
                List<string> sourceFilesModule = allCppFilesModule.Where( f => sourceFileExts.Any( ext => f.EndsWith( ext, StringComparison.OrdinalIgnoreCase ) ) ).ToList();

                List<string> headerFiles = allCppFilesModule.Where( f => headerFileExts.Any( ext => f.EndsWith( ext, StringComparison.OrdinalIgnoreCase ) ) ).ToList();

                if( Action == ActionType.Build )
                    sourceFilesModule = Shared.FileCache.Analyse( sourceFilesModule );

                ModuleToFiles.Add( kv.Key, sourceFilesModule );
                RecipeFiles.AddRange( headerFiles );
            }

            // Add {project-name}.Load.cpp file
            string loadFilePath = Path.Combine( Shared.ProjectInfo.BuildDir, $"{Shared.ProjectInfo.Name}.Load.cpp" );
            if( File.Exists( loadFilePath ) )
            {
                // Add it to the first module, a bit screwy!
                var first = ModuleToFiles.Keys.First();
                ModuleToFiles[ first ].Add( loadFilePath );
            }

            /*
            List<string> sourceFiles = DirectoryTools.SourceSearch( Shared.ProjectInfo.SourceDir, true );

            // Remove the entry file if we are not an exe.
            if( Shared.ProjectInfo.CurrentConfigKind == ConfigKind.Dist )
            {
                string EntryFilepath = Path.Combine( Shared.ProjectInfo.BuildDir, $"{Shared.ProjectInfo.Name}.Entry.cpp" );

                sourceFiles.Add( EntryFilepath );
            }

            // Add {project-name}.Load.cpp file
            string path = Path.Combine( Shared.ProjectInfo.BuildDir, $"{Shared.ProjectInfo.Name}.Load.cpp" );
            if( File.Exists( path ) )
            {
                sourceFiles.Add( path );
            }

            sourceFiles.AddRange( DirectoryTools.SourceSearch( Shared.ProjectInfo.HeaderToolGeneratedPath, true ) );

            if( Action == ActionType.Build )
                SourceFiles = Shared.FileCache.Analyse( sourceFiles );
            else
                SourceFiles = sourceFiles;
            */

            WriteRecipe();
        }

        private void WriteRecipe()
        {
            string recipeFilepath = Path.Combine( Shared.ProjectInfo.BuildDir, $"{Shared.ProjectInfo.Name}-{Shared.ProjectInfo.CurrentConfigKind}.recipe" );

            if( !File.Exists( recipeFilepath ) )
                File.Create( recipeFilepath ).Close();

            // --- Begin write 

            FileStream fs = new FileStream( recipeFilepath, FileMode.Truncate, FileAccess.Write, FileShare.ReadWrite );
            BinaryWriter writer = new BinaryWriter( fs, Encoding.UTF8 );

            writer.Write( (ulong) RecipeFiles.Count );
            foreach( var kv in RecipeFiles )
            {
                // Key string buffer.
                // C++: RawSerialisation::WriteString
                byte[] keyStrBuffer = Encoding.UTF8.GetBytes( kv );
                writer.Write( ( ulong ) keyStrBuffer.Length );
                writer.Write( keyStrBuffer );

                /*
                foreach( var item in kv.Value )
                {
                    if( item.Contains( ".Load.cpp" ) || item.Contains( ".Entry.cpp" ) )
                        continue;

                    // C++: RawSerialisation::WriteString
                    byte[] strBuffer = Encoding.UTF8.GetBytes( item );
                    writer.Write( ( ulong ) item.Length );
                    writer.Write( strBuffer );
                }
                */
            }

            writer.Close();
            fs.Close();
        }

        private bool ExecuteHeaderTool()
        {
            return HeaderToolExt.RunHeaderTool();
        }

        // Create the timestamp file for hot reloading.
        private void CreateTimestampFile()
        {
            string timetampFilename = CommandLineParser.Instance.FindFlag( "HOTRELOAD" ) ? "Timestamp.hot" : "Timestamp";

            string outputPath = Shared.TargetToBuild.GetBinDir();
            outputPath = Path.Combine( outputPath, timetampFilename );

            FileStream fs = File.Create( outputPath );
            StreamWriter sw = new StreamWriter( fs );

//            sw.Write( Shared.TargetToBuild.Timestamp );

            sw.Close();
            fs.Close();

            CleanupFromLastHotReload();
        }

        private void CleanupFromLastHotReload()
        {
            // Now, delete any left over files that match the pattern: {PrjName}_{Timestamp}.{dll/lib/pdb}
            string pattern = @"^[a-zA-Z0-9]+_[a-zA-Z0-9]+\.(dll|lib|pdb|exp)";
            Regex regex = new Regex( pattern, RegexOptions.IgnoreCase );

            string[] files = Directory.GetFiles( Shared.TargetToBuild.GetBinDir() );

            foreach( string file in files )
            {
                string stem = Path.GetFileName( file );

                /*
                if( regex.IsMatch( stem ) && !stem.Contains( Shared.TargetToBuild.Timestamp ) && stem.Contains( Shared.ProjectInfo.Name ) )
                {
                    try
                    {
                        Console.WriteLine( $"Cleaning hot reloaded dll file: {file}" );

                        File.Delete( file );
                    }
                    catch( System.IO.IOException ex )
                    {
                        Console.WriteLine( $"Skipping hot reload build file... (maybe in use or unable to delete it.) {ex.Message}" );
                    }
                }
                */
            }
        }

        private bool CheckIfLastRunWasHotReload()
        {
            bool result = false;

            string filepath = Path.Combine( Shared.TargetToBuild.GetBinDir(), "Timestamp.hot" );

            if( File.Exists( filepath ) )
            {
                try
                {
                    File.Delete( filepath );
                    return true;
                }
                catch( System.IO.IOException ex )
                {
                    Console.WriteLine( $"Failed to delete old Hot Reloaded Timestamp file: {filepath}. Error was {ex}" );
                }
            }

            return result;
        }

        private bool LinkFinal()
        {
            bool itemExisted = Shared.TaskCache.LnkFinalOutputExists( Shared.CurrentBuildTarget.TargetLinkSettings.OutputPath );
            if( TargetPendingLink || CheckIfLastRunWasHotReload() )
            {
                return Shared.Toolchain.Link( Shared.CurrentBuildTarget.TargetLinkSettings ) == 0;
            }
            else if( !itemExisted && !HasCompiledAnyFile )
            {
                Console.WriteLine( "Linking, as it does not exist in the TaskCache" );
                return Shared.Toolchain.Link( Shared.CurrentBuildTarget.TargetLinkSettings ) == 0;
            }

            return false;
        }

        private void CompileModule()
        {
            foreach( var name in SortedModules )
            {
                if( ModuleToFiles.TryGetValue( name, out var files ) )
                {
                    ModuleToToolchain.TryGetValue( name, out var toolchain );
                    if( Shared.CurrentBuildTarget.Modules.TryGetValue( name, out var buildModule ) )
                    {
                        // Compile module PCH first
                        if( buildModule.ModuleRules.PCH.Valid() ) 
                        {
                            CompileSingeFileUnchecked( buildModule.PCHCompileSettings, toolchain, buildModule.ModuleRules.PCH.SourceFile );

                            files.Remove( buildModule.ModuleRules.PCH.SourceFile );
                        }

                        int threadCount = Math.Max( 1, Math.Min( files.Count, Environment.ProcessorCount ) );
                        Parallel.For( 0, threadCount, i => 
                        {
                            int cc = files.Count / threadCount;
                            int start = i * cc;
                            int end = (i == threadCount - 1) ? files.Count : start + cc;

                            for( int j = start; j < end; j++ )
                            {
                                if( Thread.CurrentThread.Name == null )
                                {
                                    Thread.CurrentThread.Name = $"Worker-{Thread.CurrentThread.ManagedThreadId}";
                                }

                                Console.WriteLine( $"Running on thread: {Thread.CurrentThread.Name}" );

                                if( buildModule.ModuleRules.CompiledInDirectly )
                                {
                                    CompileSingeFileUnchecked( buildModule.ModuleCompileSettings, Shared.Toolchain, files[ j ] );
                                    TargetPendingLink = true;
                                }
                                else
                                {
                                    CompileSingeFileUnchecked( buildModule.ModuleCompileSettings, toolchain, files[ j ] );
                                }
                            }
                        } );

                        /*
                        foreach( var file in files )
                        {
                            if( buildModule.ModuleRules.CompiledInDirectly )
                            {
                                CompileSingeFileUnchecked( buildModule.ModuleCompileSettings, Shared.Toolchain, file );
                                TargetPendingLink = true;
                            }
                            else
                            {
                                CompileSingeFileUnchecked( buildModule.ModuleCompileSettings, toolchain, file );
                            }
                        }
                        */

                        buildModule.AppendOutputs();
                    }
                }
            }
        }

        private void LinkModules()
        {
            foreach( var name in SortedModules )
            {
                ModuleToToolchain.TryGetValue( name, out var toolchain );
                if( Shared.CurrentBuildTarget.Modules.TryGetValue( name, out var buildModule ) )
                {
                    if( buildModule.ModuleRules.CompiledInDirectly )
                        continue;

                    toolchain.Link( buildModule.ModuleLinkSettings );
                }
            }
        }

        void Visit( string moduleName )
        {
            if( _vistedModules.Contains( moduleName ) )
                return;

            _vistedModules.Add( moduleName );

            if( Shared.CurrentBuildTarget.Modules.TryGetValue( moduleName, out var buildModule ) )
            {
                // Visit dependencies first
                foreach( var dep in buildModule.ModuleRules.Modules )
                {
                    if( Shared.CurrentBuildTarget.Modules.ContainsKey( dep ) )
                        Visit( dep );
                }
            }

            // Enqueue after all dependencies are handled
            SortedModules.Enqueue( moduleName );
        }

        private void SortModules()
        {
            foreach( var kv in ModuleToFiles )
            {
                Visit( kv.Key );
            }
        }

        private void AppendGeneratedFiles() 
        {
            foreach( var kv in Shared.CurrentBuildTarget.Modules )
            {
                List<string> filePath = new List<string>();
                filePath.AddRange( DirectoryTools.CppSourceSearch( Shared.ProjectInfo.HeaderToolGeneratedRootPath, true ) );

                // Now filter the files
                filePath = Shared.FileCache.Analyse( filePath );
                
                string[] sourceFileExts = { ".cpp", ".cc", ".cxx", ".c" };
                List<string> sourceFilesModule = filePath.Where( f => sourceFileExts.Any( ext => f.EndsWith( ext, StringComparison.OrdinalIgnoreCase ) ) ).ToList();

                ModuleToFiles[ kv.Key ].AddRange( sourceFilesModule );
            }
        }

        private void ActionBuild()
        {
            Stopwatch time = Stopwatch.StartNew();

            SearchForFiles();

            if( !ExecuteHeaderTool() )
            {
                Console.WriteLine( "ERROR: Stopping compilation, header tool failed -- FAILED" );
                ExitCode = 1;
                return;
            }

            AppendGeneratedFiles();

            // Compile all source files.
            //CompileSourceFiles();

            // Compile PCH if needed
            /*
            if( Shared.TargetToBuild.PCH.SourceFile != null )
            {
                if( Shared.FileCache.HasSourceFileBeenModified( Shared.TargetToBuild.PCH.SourceFile ) || !Shared.TaskCache.TaskOutputExists( Shared.TargetToBuild.PCH.SourceFile ) )
                {
                    CompileSingeFileUnchecked( Shared.CurrentBuildTarget.TargetCompileSettings, Shared.Toolchain, Shared.TargetToBuild.PCH.SourceFile );

                    if( !HasCompiledAnyFile )
                    {
                        Console.WriteLine( "ERROR: Unable to compile PCH source!" );

                        ExitCode = 1;
                        return;
                    }
                }
            }
            */

            SortModules();
            CompileModule();
            LinkModules();

            Console.WriteLine( $"{NumTasksFailed} task(s) failed." );

//            if( LinkFinal() )
            {
                CreateTimestampFile();
            }

            Shared.TaskCache.RT_WriteCache();

            FileCache.RT_WriteCache( Shared.FileCache );

            if( CommandLineParser.Instance.FindFlag( "EXPORTFILECACHE" ) )
            {
                FileCache.RT_WriteCacheHumanReadable( Shared.FileCache );
            }

            CleanupFromLastHotReload();

            Console.WriteLine( $"Done building in {time.Elapsed}" );
        }

        private void CleanBinaryFolder()
        {
            // Binary folder.
            try
            {
                Directory.Delete( Shared.TargetToBuild.GetBinDir(), true );
            }
            catch( Exception e )
            {
                Console.WriteLine( string.Format( "Could not delete dir/file: {0}", e.Message ) );
            }

            Shared.FileCache.Clean();
        }

        private void ActionClean()
        {
            Stopwatch time = Stopwatch.StartNew();

            List<string> fileExtensionsForClean = new List<string>
            {
                ".cdf",
                ".cache",
                ".obj",
                ".obj.enc",
                ".ilk",
                ".ipdb",
                ".iobj",
                ".resources",
                ".tlb",
                ".tli",
                ".tlh",
                ".tmp",
                ".rsp",
                ".pgc",
                ".pgd",
                ".meta",
                ".tlog",
                ".manifest",
                ".res",
                ".pch",
                ".exp",
                ".idb",
                ".rep",
                ".xdc",
                ".pdb",
                "_manifest.rc", // part name : TODO
                ".bsc",
                ".sbr",
                ".xml",
                ".metagen",
                ".bi"
            };

            foreach( string file in DirectoryTools.DirSearch( Shared.TargetToBuild.OutputPath, fileExtensionsForClean ) )
            {
                try
                {
                    File.Delete( file );
                }
                catch( Exception e )
                {
                    Console.WriteLine( $"Could not clean file: {file}. Error was: {e.Message}" );
                }
            }

            CleanBinaryFolder();

            FileCache.RT_WriteCache( Shared.FileCache );

            Console.WriteLine( "Done cleaning in {0}", time.Elapsed );
        }

        public void Run()
        {
            switch( Action )
            {
                default:
                case ActionType.Build:
                case ActionType.Rebuild:
                    {
                        ActionBuild();
                    }
                    break;

                case ActionType.Clean:
                    {
                        ActionClean();
                    }
                    break;
            }
        }
    }
}
