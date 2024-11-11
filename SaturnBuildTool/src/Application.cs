using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;

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
        public string[] Args;

        private UserTarget TargetToBuild = null;

        public Toolchain Toolchain { get; set; }

        private FileCache FileCache = null;

        private bool HasCompiledAnyFile = false;

        private int NumTasksFailed = 0;

        private readonly List<List<string>> FilesPerThread = new List<List<string>>();
        private readonly List<bool> ThreadsCompleted = new List<bool>();

        private ActionType Action = ActionType.Build;

        // Includes every file in the Source dir (inc, .hpp .cpp)
        private List<string> SourceFiles = null;
        private List<string> BuildFiles = null;

        // Source Files (only .cpp) that need to be build if modifed.
        private List<string> SourceFilesToBuild = null;

        // Args:
        // 0: The Action, BUILD, REBULD, CLEAN <--TODO
        // 1: The project name
        // 2: The target platform, Win64
        // 3: The configuration, Debug, Release, Dist
        // 4: The project location
        public Application(string[] args) 
        {
            // Debug Only
            //Thread.Sleep(7000);

            Args = args;
        }

        public bool Init() 
        {
            Console.WriteLine("==== Saturn Build Tool v0.0.2 (Engine Version: 0.1.3) ====");

            // Setup project info from args.
            ProjectInfo.Instance.Setup(Args);

            TargetToBuild = UserTarget.SetupUserTarget();

            if (TargetToBuild == null)
            {
                Console.WriteLine("ERROR: Could not find a user target!, looking for {0} Please regenerate it in the engine!", ProjectInfo.Instance.BuildRuleFile);

                ExitCode = 1;
                return false;
            }

            switch (ProjectInfo.Instance.TargetPlatformKind)
            {
                case ArchitectureKind.x86:
                case ArchitectureKind.x64:
                    {
                        Toolchain = new MSVCToolchain(TargetToBuild);
                    }
                    break;

                default:
                    break;
            }

            // Set Action
            if (Args[0] == "/BUILD")
                Action = ActionType.Build;
            else if (Args[0] == "/REBUILD")
                Action = ActionType.Rebuild;
            else
                Action = ActionType.Clean;

            FileCache = FileCache.Load();

            return true;
        }

        private void CompileFiles_ForThread(object index) 
        {
            int ThreadIndex = (int)index;
            List<string> Files;

            lock (new object())
            {
                Files = FilesPerThread[ThreadIndex];
            }

            foreach (string file in Files)
            {
                // We are only building C++ source files.
                if (!FileCache.IsCppFile(file) && !FileCache.IsSourceFile(file))
                {
                    continue;
                }

                if (Action == ActionType.Rebuild)
                {
                    int exitCode = Toolchain.Compile(file);

                    if (exitCode == 0)
                    {
                        HasCompiledAnyFile = true;

                        if (!FileCache.IsFileInCache(file)) 
                        {
                            FileCache.CacheFile(file);
                        }
                    }
                    else
                        NumTasksFailed++;
                }
                else if (FileCache.HasSourceFileBeenModified(file))
                { 
                    int exitCode = Toolchain.Compile(file);

                    if (exitCode == 0)
                    {
                        HasCompiledAnyFile = true;

                        if (!FileCache.IsFileInCache(file)) 
                        {
                            FileCache.CacheFile(file);
                        }
                    }
                    else
                        NumTasksFailed++;
                }
            }

            ThreadsCompleted[ThreadIndex] = true;
        }

        private void CompileSourceFiles() 
        {
            int threadCount = 0;
            threadCount = (int)Math.Ceiling((double)SourceFiles.Count / (Environment.ProcessorCount / 2));

            if (threadCount > 1)
            {
                Console.WriteLine( String.Format( "Building with {0} threads", threadCount ) );

                ThreadPool.SetMaxThreads(threadCount, threadCount);

                // Divide the files into separate lists for each thread.
                for (int i = 0; i < threadCount; i++)
                {
                    int start = i * (SourceFiles.Count / threadCount);
                    int end = (i == threadCount - 1) ? SourceFiles.Count : i + 1 * (SourceFiles.Count / threadCount);
                    int count = end - start;

                    if (count < 0) 
                    {
                        count = 0;
                    }

                    List<string> filesForThread = new List<string>( SourceFiles.GetRange( start, count ) );
                    FilesPerThread.Add(filesForThread);
                    ThreadsCompleted.Add(false);

                    ThreadPool.QueueUserWorkItem(new WaitCallback(CompileFiles_ForThread), i);
                }

                while (ThreadsCompleted.Contains(false))
                {
                    // Wait
                    Thread.Sleep( 1 );
                }
            }
            else 
            {
                Console.WriteLine("Compiling single threaded.");

                // Pass all the files for the one thread.
                FilesPerThread.Add(SourceFiles);
                ThreadsCompleted.Add(false);

                CompileFiles_ForThread(0);
            }
        }

        private void CompileBuildFolderFiles() 
        {
            FilesPerThread.Clear();
            ThreadsCompleted.Clear();

            int threadCount = 0;
            threadCount = (int)Math.Ceiling((double)BuildFiles.Count / (Environment.ProcessorCount / 2));

            if (threadCount > 1)
            {
                Console.WriteLine(String.Format("Building with {0} threads", threadCount));

                ThreadPool.SetMaxThreads(threadCount, threadCount);

                // Divide the files into separate lists for each thread.
                for (int i = 0; i < threadCount; i++)
                {
                    int start = i * (BuildFiles.Count / threadCount);
                    int end = (i == threadCount - 1) ? BuildFiles.Count : i + 1 * (BuildFiles.Count / threadCount);
                    int count = end - start;

                    if (count < 0)
                    {
                        count = 0;
                    }

                    List<string> filesForThread = new List<string>(BuildFiles.GetRange(start, count));
                    FilesPerThread.Add(filesForThread);
                    ThreadsCompleted.Add(false);

                    ThreadPool.QueueUserWorkItem(new WaitCallback(CompileFiles_ForThread), i);
                }

                while (ThreadsCompleted.Contains(false))
                {
                    // Wait
                    Thread.Sleep(1);
                }
            }
            else
            {
                Console.WriteLine("Compiling single threaded.");

                // Pass all the files for the one thread.
                FilesPerThread.Add(BuildFiles);
                ThreadsCompleted.Add(false);

                CompileFiles_ForThread(0);
            }
        }

        private void SearchForFiles() 
        {
            SourceFiles = DirectoryTools.DirSearch(ProjectInfo.Instance.SourceDir, true);
            BuildFiles = DirectoryTools.DirSearch(ProjectInfo.Instance.BuildDir);

            // Remove the entry file if we are not an exe.
            if (ProjectInfo.Instance.CurrentConfigKind != ConfigKind.Dist)
            {
                string EntryFilepath = Path.Combine(ProjectInfo.Instance.BuildDir, string.Format("{0}.Entry.cpp", ProjectInfo.Instance.Name));

                BuildFiles.Remove(EntryFilepath);
            }
        }

        private bool ExecuteHeaderTool() 
        {
            return HeaderToolExt.RunHeaderTool();
        }

        private void RemoveTimestampFile() 
        {
            string outputPath = TargetToBuild.GetBinDir();
            outputPath = Path.Combine(outputPath, "Timestamp");

            if( File.Exists(outputPath) )
            {
                string time;
                time = File.ReadAllText(outputPath);

                // Delete timestamp
                File.Delete(outputPath);
            }
        }

        // Create the timestamp file for hot reloading.
        private void CreateTimestampFile() 
        {
            string outputPath = TargetToBuild.GetBinDir();
            outputPath = Path.Combine(outputPath, "Timestamp");
            
            FileStream fs = File.Create(outputPath);
            StreamWriter sw = new StreamWriter(fs);

            sw.Write(TargetToBuild.Timestamp);

            sw.Close();
            fs.Close();

            // Now, delete any left over files that match the pattern: {PrjName}_{Timestamp}.{dll/lib/pdb}
            string pattern = @"^.*_\d+\.[^.]+$";
            Regex regex = new Regex(pattern, RegexOptions.IgnoreCase);

            string[] files = Directory.GetFiles(TargetToBuild.GetBinDir());

            foreach (string file in files)
            {
                string stem = Path.GetFileNameWithoutExtension(file);

                if (regex.IsMatch(stem) && !stem.Contains(TargetToBuild.Timestamp))
                {
                    try
                    {
                        File.Delete(file);
                    }
                    catch (System.IO.IOException ex) // some files may be in use
                    {
                        Console.WriteLine(ex.ToString());
                    }
                }
            }
        }

        private void ActionBuild() 
        {
            Stopwatch time = Stopwatch.StartNew();

            RemoveTimestampFile();

            // Compile all source files and all "build folder" files next.
            CompileSourceFiles();
            CompileBuildFolderFiles();

            Console.WriteLine(string.Format("{0} task(s) failed.", NumTasksFailed));

            if (HasCompiledAnyFile && NumTasksFailed == 0)
            {
                if( Toolchain.Link() == 0 ) 
                {
                    CreateTimestampFile();
                }
            }

            if (HasCompiledAnyFile)
            {
                FileCache.RT_WriteCache(FileCache);
            }

            Console.WriteLine("Done building in {0}", time.Elapsed);
        }

        private void CleanBinaryFolder()
        {
            // Binary folder.
            try
            {
                Directory.Delete(TargetToBuild.GetBinDir(), true);
            }
            catch (Exception e)
            {
                Console.WriteLine(string.Format("Could not delete dir/file: {0}", e.Message));
            }

            // Intermediate binary folder.
            try
            {
                Directory.Delete(TargetToBuild.OutputPath, true);
            }
            catch (IOException e)
            {
                Console.WriteLine(string.Format("Skipping file dir/file as it could not be deleted {0}", e.Message));
            }

            FileCache.Clean();
        }

        private void CleanBuildFolder()
        {
            // Maybe not?
        }

        private void ActionClean() 
        {
            Stopwatch time = Stopwatch.StartNew();

            CleanBinaryFolder();
            CleanBuildFolder();

            FileCache.RT_WriteCache(FileCache);

            Console.WriteLine("Done cleaning in {0}", time.Elapsed);
        }

        private void AnalyseFileChanges() 
        {
            if (Action == ActionType.Rebuild) 
            {
                SourceFilesToBuild = SourceFiles;
                return;
            }

            Console.WriteLine("Analyzing Changes...");

            foreach( string sourceFile in SourceFiles )
            {
                if(FileCache.HasSourceFileBeenModified(sourceFile)) 
                {
                    SourceFilesToBuild.Add(sourceFile);
                }
            }
        }

        public void Run() 
        {
            SearchForFiles();

            switch( Action )
            {
                case ActionType.Build:
                case ActionType.Rebuild:
                    {
                        if (ExecuteHeaderTool()) 
                        {
                            ActionBuild();
                        }
                        else 
                        {
                            Console.WriteLine("ERROR: Stopping compilation, header tool failed -- FAILED");
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
