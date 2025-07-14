using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;
using System.Threading;

using SaturnBuildTool.Cache;
using SaturnBuildTool.Tools;
using SaturnBuildTool.Auxiliary;

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

        private UserTarget TargetToBuild = null;

        private ToolchainBase Toolchain;

        private FileCache FileCache = null;

        private bool HasCompiledAnyFile = false;

        private int NumTasksFailed = 0;

        private readonly List<List<string>> FilesPerThread = new List<List<string>>();
        private readonly List<bool> ThreadsCompleted = new List<bool>();

        private ActionType Action = ActionType.Build;

        // Includes every file in the Source dir (inc, .hpp .cpp)
        private List<string> SourceFiles = null;

        public Application( string[] args )
        {
            Args = new List<string>();
            Args.AddRange( args );
        }

        public bool Init()
        {
            bool safeToContinue = false;
            CommandLineParser.Instance.Parse( Args );

            // Handle command lines args
            if( CheckArgs() ) 
            {
                safeToContinue = InitForBuilding();
            }

            return safeToContinue;
        }

        private bool CheckArgs() 
        {
            if( CommandLineParser.Instance.FindFlag( "HELP" ) )
            {
                Console.WriteLine( "Help for Saturn Build Tool X0.0.4 (Engine Version: 0.2.1 8193)" );
                Console.WriteLine( "Options:" );
                Console.WriteLine( " Action Options:" );
                Console.WriteLine( "  /BUILD* -- build the project" );
                Console.WriteLine( "  /REBUILD* -- rebuild the project, ignoring the filecache" );
                Console.WriteLine( "  /CLEAN* -- clean the project" );
                Console.WriteLine( " Project Options:" );
                Console.WriteLine( "  /PROJECT* -- path to the project root dir (same place where the .sproject file is located)" );
                Console.WriteLine( "  /NAME* -- project name MUST match with the .sproject file name!" );
                Console.WriteLine( "  /SATURNDIR -- override the Saturn Root Directory by default the build tool will use the \"SATURN_DIR\" environment variable" );
                Console.WriteLine( " Compile Options:" );
                Console.WriteLine( "  /WIN64* -- build for Windows x64" );
                Console.WriteLine( "  /HOTRELOAD -- this is an internal command and is used for hot reloading, when this command is suggested the build tool will create a special timestamp file and output files with the timestamp suffix" );
                Console.WriteLine( "  /DISTASDBG -- Build for Dist but compile with debug symbols and full \"/DIST\" must be suggested" );
                Console.WriteLine( "  Configuration Options:" );
                Console.WriteLine( "   /DEBUG* -- build the project for Debug configuration with full symbols" );
                Console.WriteLine( "   /RELEASE* -- build the project for Release configuration with symbols on for this project but symbols off for third party projects" );
                Console.WriteLine( "   /DIST* -- build the project for the Dist configuration" );
                Console.WriteLine( "  Warning Options:" );
                Console.WriteLine( "   /XW+ -- treat warnings as errors" );
                Console.WriteLine( "   /XW- -- no warnings" );
                Console.WriteLine( "   /XW1 -- warnings level one" );
                Console.WriteLine( "   /XW2 -- warnings level two" );
                Console.WriteLine( "   /XW3 -- warnings level three (default)" );
                Console.WriteLine( "   /XW4 -- warnings level four" );
                Console.WriteLine( "   /XWAx -- all warnings" );
                Console.WriteLine( " Auxiliary Options:" );
                Console.WriteLine( "  /HELP -- this command that displays the help message" );
                Console.WriteLine( "  /INCLUDESTREE -- displays and create an include tree" );
                Console.WriteLine( "  /VERISON -- displays the version for the build tool" );
                Console.WriteLine( "  /ARGS+ -- displays the compiler and linker command line arguments" );
                Console.WriteLine( " * indicates required argument" );

                return false;
            }

            if( CommandLineParser.Instance.FindFlag( "VERSION" ) )
            {
                Console.WriteLine( "Saturn Build Tool X0.0.4 (Engine Version: 0.2.1 8193)" );
                return false;
            }

            if( CommandLineParser.Instance.GetComamndCount() == 0 ) 
            {
                Console.WriteLine( "You must provide more than one argument! Try running /HELP for more" );

                // treat as error
                ExitCode = 1;
                return false;
            }

            return true;
        }

        private bool InitForBuilding() 
        {
            Console.WriteLine( "==== Saturn Build Tool X0.0.4 (Engine Version: 0.2.1 8193) ====" );

            // Setup project info from args.
            if( !ProjectInfo.Instance.Setup() )
            {
                ExitCode = 1;
                return false;
            }

            TargetToBuild = UserTarget.SetupUserTarget();

            if( TargetToBuild == null )
            {
                Console.WriteLine( "ERROR: Could not find a user target!, looking for {0} Please regenerate it in the engine!", ProjectInfo.Instance.BuildRuleFile );

                ExitCode = 1;
                return false;
            }

            // TODO: Move this into CommandLineParser
            // Maybe CommandLineParser.VerifyAll
            if( CommandLineParser.Instance.FindFlag( "DISTASDBG" ) && TargetToBuild.CurrentConfig != ConfigKind.Dist ) 
            {
                Console.WriteLine( "ERROR: \"/DISTASDBG\" was suggested however, you aren't building for Dist! \"/DISTASDBG\" is only available when \"/DIST\" is suggested" );

                ExitCode = 1;
                return false;
            }

            if( CommandLineParser.Instance.FindFlag( "showconsole" ) && TargetToBuild.CurrentConfig != ConfigKind.Dist )
            {
                Console.WriteLine( "ERROR: \"/SHOWCONSOLE\" was suggested however, you aren't building for Dist! \"/showconsole\" is only available when \"/DIST\" is suggested" );

                ExitCode = 1;
                return false;
            }

            switch( ProjectInfo.Instance.TargetPlatformKind )
            {
                case ArchitectureKind.x86:
                case ArchitectureKind.x64:
                    {
                        Toolchain = new MSVCToolchain( TargetToBuild );
                    }
                    break;

                default:
                    break;
            }

            // Set Action
            if( CommandLineParser.Instance.FindFlag( "BUILD" ) )
                Action = ActionType.Build;
            else if( CommandLineParser.Instance.FindFlag( "REBUILD" ) )
                Action = ActionType.Rebuild;
            else
                Action = ActionType.Clean;

            FileCache = FileCache.Load();

            return true;
        }

        private void CompileFiles_ForThread( object index )
        {
            int ThreadIndex = ( int ) index;
            List<string> Files;

            lock( new object() )
            {
                Files = FilesPerThread[ ThreadIndex ];
            }

            foreach( string file in Files )
            {
                // We are only building C++ source files.
                if( !FileCache.IsCppFile( file ) && !FileCache.IsSourceFile( file ) )
                {
                    continue;
                }

                if( Action == ActionType.Rebuild )
                {
                    int exitCode = Toolchain.Compile( file );

                    if( exitCode == 0 )
                    {
                        HasCompiledAnyFile = true;

                        if( !FileCache.IsFileInCache( file ) )
                        {
                            FileCache.CacheFile( file );
                        }
                    }
                    else
                        NumTasksFailed++;
                }
                else if( FileCache.HasSourceFileBeenModified( file, true ) )
                {
                    int exitCode = Toolchain.Compile( file );

                    if( exitCode == 0 )
                    {
                        HasCompiledAnyFile = true;

                        if( !FileCache.IsFileInCache( file ) )
                        {
                            FileCache.CacheFile( file );
                        }
                    }
                    else
                        NumTasksFailed++;
                }
            }

            ThreadsCompleted[ ThreadIndex ] = true;
        }

        private void CompileSourceFiles()
        {
            int threadCount = 0;
            threadCount = ( int ) Math.Ceiling( ( double ) SourceFiles.Count / ( Environment.ProcessorCount / 2 ) );

            if( threadCount > 1 )
            {
                Console.WriteLine( String.Format( "Building with {0} threads", threadCount ) );

                ThreadPool.SetMaxThreads( threadCount, threadCount );

                // Divide the files into separate lists for each thread.
                for( int i = 0; i < threadCount; i++ )
                {
                    int start = i * ( SourceFiles.Count / threadCount );
                    int end = ( i == threadCount - 1 ) ? SourceFiles.Count : i + 1 * ( SourceFiles.Count / threadCount );
                    int count = end - start;

                    if( count < 0 )
                    {
                        count = 0;
                    }

                    List<string> filesForThread = new List<string>( SourceFiles.GetRange( start, count ) );
                    FilesPerThread.Add( filesForThread );
                    ThreadsCompleted.Add( false );

                    ThreadPool.QueueUserWorkItem( new WaitCallback( CompileFiles_ForThread ), i );
                }

                while( ThreadsCompleted.Contains( false ) )
                {
                    // Wait
                    Thread.Sleep( 1 );
                }
            }
            else
            {
                Console.WriteLine( "Compiling single threaded." );

                // Pass all the files for the one thread.
                FilesPerThread.Add( SourceFiles );
                ThreadsCompleted.Add( false );

                CompileFiles_ForThread( 0 );
            }
        }

        private void SearchForFiles()
        {
            SourceFiles = DirectoryTools.SourceSearch( ProjectInfo.Instance.SourceDir, true );
            SourceFiles.AddRange( DirectoryTools.SourceSearch( ProjectInfo.Instance.BuildDir, true ) );

            // Remove the entry file if we are not an exe.
            if( ProjectInfo.Instance.CurrentConfigKind != ConfigKind.Dist )
            {
                string EntryFilepath = Path.Combine( ProjectInfo.Instance.BuildDir, string.Format( "{0}.Entry.cpp", ProjectInfo.Instance.Name ) );

                SourceFiles.Remove( EntryFilepath );
            }
        }

        private bool ExecuteHeaderTool()
        {
            return HeaderToolExt.RunHeaderTool();
        }

        // Create the timestamp file for hot reloading.
        private void CreateTimestampFile()
        {
            string timetampFilename = CommandLineParser.Instance.FindFlag( "HOTRELOAD" ) ? "Timestamp.hot" : "Timestamp";

            string outputPath = TargetToBuild.GetBinDir();
            outputPath = Path.Combine( outputPath, timetampFilename );

            FileStream fs = File.Create( outputPath );
            StreamWriter sw = new StreamWriter( fs );

            sw.Write( TargetToBuild.Timestamp );

            sw.Close();
            fs.Close();

            CleanupFromLastHotReload();
        }

        private void CleanupFromLastHotReload() 
        {
            // Now, delete any left over files that match the pattern: {PrjName}_{Timestamp}.{dll/lib/pdb}
            string pattern = @"^[a-zA-Z0-9]+_[a-zA-Z0-9]+\.(dll|lib|pdb|exp)";
            Regex regex = new Regex( pattern, RegexOptions.IgnoreCase );

            string[] files = Directory.GetFiles( TargetToBuild.GetBinDir() );

            foreach( string file in files )
            {
                string stem = Path.GetFileName( file );

                if( regex.IsMatch( stem ) && !stem.Contains( TargetToBuild.Timestamp ) && stem.Contains( TargetToBuild.ProjectName ) )
                {
                    try
                    {
                        Console.WriteLine( $"Cleaning hot reloaded dll file: {file}" );

                        File.Delete( file );
                    }
                    catch( System.IO.IOException ex )
                    {
                        Console.WriteLine( $"Skipping hot reload build file... (maybe in use, or unable to delete it.) {ex.Message}" );
                    }
                }
            }
        }

        private bool CheckIfLastRunWasHT() 
        {
            bool result = false;

            string filepath = Path.Combine( TargetToBuild.GetBinDir(), "Timestamp.hot" );

            if( File.Exists( filepath ) ) 
            {
                try
                {
                    File.Delete( filepath );
                    return true;
                }
                catch( System.IO.IOException ex ) 
                {
                    Console.WriteLine( $"Failed to delete old Hot Reloaded Timestamp file: {filepath}. Error was {ex.ToString()}" );
                }
            }

            return result;
        }

        private void ActionBuild()
        {
            Stopwatch time = Stopwatch.StartNew();

            bool lastInstanceWasHotReload = CheckIfLastRunWasHT();

            SearchForFiles();

            // Compile all source files.
            CompileSourceFiles();

            Console.WriteLine( string.Format( "{0} task(s) failed.", NumTasksFailed ) );

            if( HasCompiledAnyFile && NumTasksFailed == 0 || lastInstanceWasHotReload )
            {
                if( Toolchain.Link() == 0 )
                {
                    CreateTimestampFile();
                }
            }

            if( HasCompiledAnyFile )
            {
                FileCache.RT_WriteCache( FileCache );
            }

            CleanupFromLastHotReload();

            Console.WriteLine( "Done building in {0}", time.Elapsed );
        }

        private void CleanBinaryFolder()
        {
            // Binary folder.
            try
            {
                Directory.Delete( TargetToBuild.GetBinDir(), true );
            }
            catch( Exception e )
            {
                Console.WriteLine( string.Format( "Could not delete dir/file: {0}", e.Message ) );
            }

            FileCache.Clean();
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

            foreach( string file in DirectoryTools.DirSearch( TargetToBuild.OutputPath, fileExtensionsForClean ) )
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

            FileCache.RT_WriteCache( FileCache );

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
                        if( ExecuteHeaderTool() )
                        {
                            ActionBuild();
                        }
                        else
                        {
                            Console.WriteLine( "ERROR: Stopping compilation, header tool failed -- FAILED" );
                            ExitCode = 1;
                        }
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
