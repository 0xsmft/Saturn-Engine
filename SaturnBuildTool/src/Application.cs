using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
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

    public enum ApplicationExitStatus 
    {
        Success = 0,
        Failure = 1,
        NothingTodo = 2,
    }

    internal class Application
    {
        public ApplicationExitStatus ExitCode = ApplicationExitStatus.Success;
        private readonly List<string> Args;

        private int NumTasksFailed = 0;
        private int AttemptedTasks = 0;

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
        private readonly string StartupMessage = "Saturn Build Tool X0.0.5 \"SBT 5.1\"";

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
                ExitCode = ApplicationExitStatus.Failure;
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
                Console.WriteLine( "  Platform Options:" );
                Console.WriteLine( "   /WIN64*          -- build for Windows x64" );
                Console.WriteLine( "   /LINUX64*        -- build for Linux x64" );
                Console.WriteLine( "   /APPLE*          -- build for macOS AArch64 (Apple Silicon)" );
                Console.WriteLine( "  /CC              -- specify toolchain to use. On Windows by default this is set to MSVC, on linux and macOS this is set to CLANG. Options are /CC={MSVC|GCC|CLANG}" );
                Console.WriteLine( "  /HOTRELOAD       -- this is an internal command and is used for hot reloading, when this command is suggested the build tool will create a special timestamp file and output files with the timestamp suffix" );
                Console.WriteLine( "  /DISTASDBG       -- Build for Dist but compile with debug symbols and no optimisation. \"/DIST\" must be suggested" );
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
                ExitCode = ApplicationExitStatus.Failure;
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

                ExitCode = ApplicationExitStatus.Failure;
                return false;
            }

            // Find and compile rules
            Shared.RulesAssembly = RulesAssembly.CompileRules();

            if( Shared.TargetToBuild == null )
            {
                Console.WriteLine( $"ERROR: The target file: {Shared.ProjectInfo.BuildRuleFile}, failed to compile or it doesn't exist!" );

                ExitCode = ApplicationExitStatus.Failure;
                return false;
            }

            // Convert rules into proper buildable items.
            Shared.CurrentBuildTarget = BuildTarget.Create( Shared.TargetToBuild );
            Shared.CurrentBuildTarget.Init();

            // TODO: Move this into CommandLineParser
            // Maybe CommandLineParser.VerifyAll
            if( CommandLineParser.Instance.FindFlag( "DISTASDBG" ) && Shared.ProjectInfo.CurrentConfigKind != ConfigKind.Dist )
            {
                Console.WriteLine( "ERROR: \"/DISTASDBG\" was suggested however, you aren't building for Dist! \"/DISTASDBG\" is only available when \"/DIST\" is suggested" );

                ExitCode = ApplicationExitStatus.Failure;
                return false;
            }

            if( CommandLineParser.Instance.FindFlag( "showconsole" ) && Shared.ProjectInfo.CurrentConfigKind != ConfigKind.Dist )
            {
                Console.WriteLine( "ERROR: \"/SHOWCONSOLE\" was suggested however, you aren't building for Dist! \"/showconsole\" is only available when \"/DIST\" is suggested" );

                ExitCode = ApplicationExitStatus.Failure;
                return false;
            }

            switch( Shared.ProjectInfo.ToolchainTypeToUse )
            {
                case ToolchainType.MSVC:
                    {
                        Shared.Toolchain = new MSVCToolchain();
                    }
                    break;

                case ToolchainType.Clang:
                    {
                        Shared.Toolchain = new ClangToolchain();
                    }
                    break;

                default:
                    return false;
            }

            Shared.FileCache = FileCache.Load();
            Shared.TaskCache = TaskCache.Load();
            Shared.LinkCache = LinkCache.Load();

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

                ExitCode = ApplicationExitStatus.Failure;
                return false;
            }

            Shared.FileCache = FileCache.Load();
            Shared.TaskCache = TaskCache.Load();

            return true;
        }

        private void CompileSingeFileUnchecked( CompileSettings compileSettings, BuildModule buildModule, ToolchainBase toolchain, string file )
        {
            int exitCode = toolchain.Compile( file, compileSettings );
            Shared.FileCache.CacheFile( file );

            if( exitCode != 0 )
            {
                Interlocked.Increment( ref NumTasksFailed );
                Console.WriteLine( $"SBT: ERR: UNABLE TO COMPILE FILE: CL {file}" );
            }
            else
            {
                buildModule.ShouldLink = true;
            }
        }

        private bool SearchForFiles()
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

            if( !File.Exists( loadFilePath )  )
            {
                Console.WriteLine( $"ERROR: Required file {loadFilePath} does not exist! Please regenerate it in the Engine." );

                ExitCode = ApplicationExitStatus.Failure;
                return false;
            }

            bool modified = Shared.FileCache.HasFileBeenModified( loadFilePath );
            if( File.Exists( loadFilePath ) && ( modified || Action == ActionType.Rebuild ) )
            {
                // Add it to the first module, a bit screwy!
                var first = ModuleToFiles.Keys.First();
                ModuleToFiles[ first ].Add( loadFilePath );

                Shared.FileCache.CacheFile( loadFilePath );
            }

            if( Shared.ProjectInfo.CurrentConfigKind == ConfigKind.Dist )
            {
                // Add {project-name}.Entry.cpp file
                string entryFilePath = Path.Combine( Shared.ProjectInfo.BuildDir, $"{Shared.ProjectInfo.Name}.Entry.cpp" );

                if( !File.Exists( entryFilePath ) ) 
                {
                    Console.WriteLine( $"ERROR: Required file {loadFilePath} does not exist! Please regenerate it in the Engine." );

                    ExitCode = ApplicationExitStatus.Failure;
                    return false;
                }

                bool entryModified = Shared.FileCache.HasFileBeenModified( entryFilePath );
                if( File.Exists( entryFilePath ) && ( entryModified || Action == ActionType.Rebuild ) )
                {
                    // Add it to the first module, a bit screwy!
                    var first = ModuleToFiles.Keys.First();
                    ModuleToFiles[ first ].Add( entryFilePath );

                    Shared.FileCache.CacheFile( entryFilePath );
                }
            }

            /*
            List<string> sourceFiles = DirectoryTools.SourceSearch( Shared.ProjectInfo.SourceDir, true );

            // Remove the entry file if we are not an exe.
            if( Shared.ProjectInfo.CurrentConfigKind == ConfigKind.Dist )
            {
                string EntryFilepath = Path.Combine( Shared.ProjectInfo.BuildDir, $"{Shared.ProjectInfo.Name}.Entry.cpp" );

                sourceFiles.Add( EntryFilepath );
            }

            sourceFiles.AddRange( DirectoryTools.SourceSearch( Shared.ProjectInfo.HeaderToolGeneratedPath, true ) );
            */

            WriteRecipe();

            return true;
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

            sw.Write( Shared.CurrentBuildTarget.Timestamp );

            sw.Close();
            fs.Close();

            CleanupFromLastHotReload();
        }

        private void CleanupFromLastHotReload()
        {
            string filepath = Path.Combine( Shared.TargetToBuild.GetBinDir(), "Timestamp.hot" );

            if( File.Exists( filepath ) )
            {
                StreamReader streamReader = new StreamReader( filepath );

                string text = null;
                try
                {
                    text = streamReader.ReadLine();
                }
                catch( Exception ex )
                {
                    Console.WriteLine( $"Warning unable to read Timestamp.hot file! Error: {ex.Message}" );
                    return;
                }
                streamReader.Close();

                string fileStem = Shared.TargetToBuild.Name + $"_{Shared.CurrentBuildTarget.Timestamp}";

                string[] files = Directory.GetFiles( Shared.TargetToBuild.GetBinDir() );
                foreach( string file in files )
                {
                    string stem = Path.GetFileNameWithoutExtension( file );

                    if( stem.Contains( fileStem ) )
                    {
                        Console.WriteLine( $"Cleaned up file {Path.GetFileName( file )} from last hot-reload build." );
                        File.Delete( file );
                    }
                }

                // Now, we delete the Timestamp.hot file
                File.Delete( filepath );
            }
        }

        private void CompileModule()
        {
            foreach( var name in SortedModules )
            {
                if( ModuleToFiles.TryGetValue( name, out var files ) )
                {
                    if( files.Count == 0 )
                        continue;

                    ModuleToToolchain.TryGetValue( name, out var toolchain );
                    if( Shared.CurrentBuildTarget.Modules.TryGetValue( name, out var buildModule ) )
                    {
                        // Compile module PCH first
                        if( buildModule.ModuleRules.PCH.Valid() ) 
                        {
                            bool modified = Shared.FileCache.HasFileBeenModified( buildModule.ModuleRules.PCH.SourceFile );
                            if( modified || Action == ActionType.Rebuild )
                            {
                                // Compile PCH with PCH settings.
                                CompileSingeFileUnchecked( buildModule.PCHCompileSettings, buildModule, toolchain, buildModule.ModuleRules.PCH.SourceFile );

                                files.Remove( buildModule.ModuleRules.PCH.SourceFile );

                                Shared.FileCache.CacheFile( buildModule.ModuleRules.PCH.SourceFile );
                            }
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

                                Interlocked.Increment( ref AttemptedTasks );

                                if( buildModule.ModuleRules.CompiledInDirectly )
                                {
                                    CompileSingeFileUnchecked( buildModule.ModuleCompileSettings, buildModule, Shared.Toolchain, files[ j ] );
                                }
                                else
                                {
                                    CompileSingeFileUnchecked( buildModule.ModuleCompileSettings, buildModule, toolchain, files[ j ] );
                                }
                            }
                        } );
                    }
                }
            }
        }

        private void BuildLinkCacheForModules() 
        {
            foreach( var name in SortedModules )
            {
                if( Shared.CurrentBuildTarget.Modules.TryGetValue( name, out var buildModule ) )
                {
                    // Append outputs
                    buildModule.AppendOutputs();

                    foreach( var link in buildModule.ModuleLinkSettings.Links )
                    {
                        string path = link;

                        // If there is not parent i.e. "MyLink.lib" then we need to resolve the link path to this lib...
                        if( !Path.IsPathRooted( link ) )
                        {
                            // Only if it does not exist.
                            if( !File.Exists( link ) )
                            {
                                foreach( var linkPath in buildModule.ModuleLinkSettings.LibraryPaths )
                                {
                                    string resolvedPath = Path.Combine( linkPath, link );
                                    if( File.Exists( resolvedPath ) )
                                    {
                                        path = resolvedPath;
                                        break;
                                    }
                                    else
                                    {
                                        path = null;
                                    }
                                }
                            }
                        }

                        if( path != null )
                            Shared.LinkCache.CacheFile( path );
                    }
                }
            }
        }

        private void LinkModules()
        {
            if( NumTasksFailed != 0 )
                return;

            foreach( var name in SortedModules )
            {
                ModuleToToolchain.TryGetValue( name, out var toolchain );
                if( Shared.CurrentBuildTarget.Modules.TryGetValue( name, out var buildModule ) )
                {
                    if( buildModule.ModuleRules.CompiledInDirectly )
                        continue;

                    string fullPath = buildModule.GetFullBinaryPathWithFilename();
                    bool shouldLink = !Shared.TaskCache.LnkFinalOutputExists( fullPath );

                    // If any .lib file has changed and we need it, then we must link as well
                    foreach( var link in buildModule.ModuleLinkSettings.Links )
                    {
                        string searchPath = link;

                        // If there is no parent path i.e. "MyLink.lib" then we need to resolve the link path to this lib...
                        if( !Path.IsPathRooted( link ) ) 
                        {
                            // Only if it does not exist...
                            if( !File.Exists( link ) )
                            {
                                foreach( var linkPath in buildModule.ModuleLinkSettings.LibraryPaths )
                                {
                                    string path = Path.Combine( linkPath, link );
                                    if( File.Exists( path ) )
                                    {
                                        searchPath = path;
                                        break;
                                    }
                                }
                            }
                        }

                        shouldLink |= Shared.LinkCache.HasFileBeenModified( searchPath );
                    }

                    shouldLink |= buildModule.ShouldLink;

                    if( shouldLink ) 
                    {
                        ++AttemptedTasks;
                        toolchain.Link( buildModule.ModuleLinkSettings );
                    }
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
                if( Action == ActionType.Build )
                    filePath = Shared.FileCache.Analyse( filePath );
                
                string[] sourceFileExts = { ".cpp", ".cc", ".cxx", ".c" };
                List<string> sourceFilesModule = filePath.Where( f => sourceFileExts.Any( ext => f.EndsWith( ext, StringComparison.OrdinalIgnoreCase ) ) ).ToList();

                ModuleToFiles[ kv.Key ].AddRange( sourceFilesModule );
            }
        }

        private void ActionBuild()
        {
            Stopwatch time = Stopwatch.StartNew();

            if( !SearchForFiles() )
            {
                return;
            }

            if( !ExecuteHeaderTool() )
            {
                Console.WriteLine( "ERROR: Stopping compilation, header tool failed -- FAILED" );
                ExitCode = ApplicationExitStatus.Failure;
                return;
            }

            // ~Pre build.
            AppendGeneratedFiles();
            SortModules();

            // ~Build
            CompileModule();
            BuildLinkCacheForModules();
            LinkModules();

            // ~Post build.
            Console.WriteLine( $"{NumTasksFailed} task(s) failed." );

            CreateTimestampFile();

            Shared.TaskCache.RT_WriteCache();
            Shared.LinkCache.RT_WriteCache();

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

                        // Determine exit code.
                        // If NumTasksFailed is non-zero then we have a task that is failed.
                        // If no tasks have failed but we haven't actually done anything then we report NothingTodo.
                        // and by default the value is Success.
                        if( NumTasksFailed != 0 )
                            ExitCode = ApplicationExitStatus.Failure;
                        else if( AttemptedTasks == 0 )
                            ExitCode = ApplicationExitStatus.NothingTodo;

                        Console.WriteLine( ExitCode.ToString() );
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
