using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

using SaturnBuildTool.Tools;

namespace SaturnBuildTool
{
    internal class CompileTask : AsyncTask
    {
        private string InputFile;
        private UserTarget TargetToBuild;

        public CompileTask(string inputFile, UserTarget target)
        {
            InputFile = inputFile;
            TargetToBuild = target;
        }

        public override int Execute()
        {
            if (Path.GetExtension(InputFile) != ".cpp")
                return 0;

            string CLLocation = VSWhere.FindMSVCToolsDir();

            var Args = new List<string>();

            ProcessStartInfo processStart = new ProcessStartInfo();

            switch (ProjectInfo.Instance.TargetPlatformKind)
            {
                case ArchitectureKind.x64:
                    {
                        processStart.FileName = CLLocation + "/bin/Hostx64/x64/cl.exe";
                    }
                    break;

                case ArchitectureKind.x86:
                    {
                        processStart.FileName = CLLocation + "/bin/Hostx64/x86/cl.exe";
                    }
                    break;
            }

            processStart.CreateNoWindow = true;
            processStart.RedirectStandardOutput = true;
            processStart.RedirectStandardError = true;
            processStart.UseShellExecute = false;

            Process clProcess = new Process
            {
                StartInfo = processStart
            };

            // Parse Args
            Args.Add(" /nologo");

            // Compile (without linking)
            Args.Add(" /c");

            Args.Add(" /errorreport:prompt");

            // Compile for C++
            Args.Add(" /std:c++latest /D _HAS_EXCEPTIONS=0");

            // Unwind semantics.
            Args.Add(" /EHsc");

            // Eliminate Duplicate Strings
            Args.Add(" /GF");

            Args.Add(" /MP");

            Args.Add(" /GS");

            switch( TargetToBuild.CurrentConfig ) 
            {
                case ConfigKind.Debug:
                case ConfigKind.Release:
                    {
                        if(TargetToBuild.CurrentConfig == ConfigKind.Debug)
                        {
                            Args.Add(" /D \"SAT_DEBUG\"");
                            Args.Add(" /MTd");
                        }
                        else 
                        {
                            Args.Add(" /D \"SAT_RELEASE\"");
                            Args.Add(" /MT");
                        }

                        Args.Add(" /ZI");
                        // No optimzation.
                        Args.Add(" /Od");
                        Args.Add(" /FS");
                    } break;

                case ConfigKind.Dist:
                    {
                        Args.Add(" /D \"SAT_DIST\"");
                        Args.Add(" /MT");
                        Args.Add(" /Ox");
                    } break;
            }

            // Out
            string outFile = string.Format("{0}\\{1}", TargetToBuild.OutputPath, Path.GetFileName(Path.ChangeExtension(InputFile, ".obj")));

            Args.Add(string.Format(" /Fo\"{0}\"", outFile));

            // Marcos
            List<string> marcos = TargetToBuild.PreprocessorDefines;

            foreach (string name in marcos)
            {
                Args.Add(string.Format(" /D\"{0}\"", name));
            }

            // Includes
            List<string> incs = TargetToBuild.Includes;
            foreach (string include in incs)
            {
                Args.Add(string.Format(" /I\"{0}\"", include));
            }

            Args.Add(string.Format(" /I\"{0}\"", CLLocation + "/include"));

            // Windows SDK
            string includeSDKFolder = WindowsSDK.GetIncludePaths();
            Args.Add(string.Format(" /I\"{0}\"", includeSDKFolder + "/ucrt"));
            Args.Add(string.Format(" /I\"{0}\"", includeSDKFolder + "/um"));
            Args.Add(string.Format(" /I\"{0}\"", includeSDKFolder + "/shared"));

            // In
            Args.Add(string.Format(" /Tp\"{0}\"", InputFile));

            // Start the compile
            processStart.Arguments = string.Join("", Args);

            Console.WriteLine("Building " + Path.GetFileName(InputFile));

            clProcess.EnableRaisingEvents = true;

            // Enable this for Debugging
            Console.WriteLine( "Command Line: {0}", processStart.Arguments );
            
            clProcess.OutputDataReceived += new DataReceivedEventHandler((s, e) =>
            {
                if (e.Data != null)
                {
                    Console.WriteLine(e.Data);
                }
            });

            clProcess.ErrorDataReceived += new DataReceivedEventHandler((s, e) =>
            {
                if (e.Data != null)
                {
                    Console.WriteLine(e.Data);
                }
            });

            clProcess.Start();
            
            // Debugging
            clProcess.BeginErrorReadLine();
            clProcess.BeginOutputReadLine();

            clProcess.WaitForExit();

            // Write error output (disable this when using synchronous output)
            //Console.WriteLine(clProcess.StandardOutput.ReadToEnd().Trim());

            return clProcess.ExitCode;
        }
    }
}
