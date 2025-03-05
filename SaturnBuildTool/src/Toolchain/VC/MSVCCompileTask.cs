using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using SaturnBuildTool.Auxiliary;
using SaturnBuildTool.Tools;

namespace SaturnBuildTool
{
    internal class MSVCCompileTask : TaskBase
    {
        private readonly string InputFile;
        private readonly UserTarget TargetToBuild;

        public MSVCCompileTask( string inputFile, UserTarget target )
        {
            InputFile = inputFile;
            TargetToBuild = target;
        }

        public override int Execute()
        {
            if( Path.GetExtension( InputFile ) != ".cpp" )
                return 0;

            string CLLocation = VSWhere.FindMSVCToolsDir();

            var Args = new List<string>();

            ProcessStartInfo processStart = new ProcessStartInfo();

            switch( ProjectInfo.Instance.TargetPlatformKind )
            {
                case ArchitectureKind.x64:
                    {
                        processStart.FileName = Path.Combine( CLLocation, "bin", "Hostx64", "x64", "cl.exe" );
                    }
                    break;

                case ArchitectureKind.x86:
                    {
                        processStart.FileName = Path.Combine( CLLocation, "bin", "Hostx86", "x86", "cl.exe" );
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
            Args.Add( " /nologo" );

            // Compile (without linking)
            Args.Add( " /c" );

            Args.Add( " /errorreport:prompt" );

            // Compile for C++ with C++23 working draft
            Args.Add( " /std:c++latest" );

            // Exception handling (EH) -- Unwind stack (s) extern "C" function can't throw a C++ exception (c).
            Args.Add( " /EHsc" );

            // Eliminate Duplicate Strings
            Args.Add( " /GF" );

            // Build with multiple processes
            Args.Add( " /MP" );

            // Buffer Security Check
            Args.Add( " /GS" );

            // Configuration specific
            switch( TargetToBuild.CurrentConfig )
            {
                case ConfigKind.Debug:
                case ConfigKind.Release:
                    {
                        if( TargetToBuild.CurrentConfig == ConfigKind.Debug )
                        {
                            Args.Add( " /D \"SAT_DEBUG\"" );
                            Args.Add( " /MTd" ); // Multithreaded debug RT
                        }
                        else
                        {
                            Args.Add( " /D \"SAT_RELEASE\"" );
                            Args.Add( " /MT" ); // Multithreaded RT
                        }

                        Args.Add( " /Z7" ); // Build with Z7 debug pdbs
                        Args.Add( " /Od" ); // No optimisation.
                        Args.Add( " /FS" ); // Force Synchronous PDB Writes
                        Args.Add( " /Gw" ); // Optimize Global Data
                    }
                    break;

                case ConfigKind.Dist:
                    {
                        Args.Add( " /D \"SAT_DIST\"" );
                        Args.Add( " /MT" ); // Multithreaded RT
                        Args.Add( " /Ox" ); // Favour speed (optimisation)
                    }
                    break;
            }

            if( CommandLineParser.Instance.FindFlag( "DISTASDBG" ) ) 
            {
                Args.Add( " /Z7" );
            }

            // Out
            string outFile = Path.GetFileName( Path.ChangeExtension( InputFile, ".obj" ) );
            Args.Add( string.Format( " /Fo\"{0}\"", Path.Combine( TargetToBuild.OutputPath, outFile ) ) );

            // Marcos
            List<string> marcos = TargetToBuild.PreprocessorDefines;

            foreach( string name in marcos )
            {
                Args.Add( string.Format( " /D\"{0}\"", name ) );
            }

            // Includes
            List<string> incs = TargetToBuild.Includes;
            foreach( string include in incs )
            {
                Args.Add( string.Format( " /I\"{0}\"", include ) );
            }

            // Include C++ STL
            Args.Add( string.Format( " /I\"{0}\"", Path.Combine( CLLocation, "include" ) ) );

            // Windows SDK
            string includeSDKFolder = WindowsSDK.GetIncludePaths();
            Args.Add( string.Format( " /I\"{0}\"", Path.Combine( includeSDKFolder, "ucrt" ) ) );
            Args.Add( string.Format( " /I\"{0}\"", Path.Combine( includeSDKFolder, "um" ) ) );
            Args.Add( string.Format( " /I\"{0}\"", Path.Combine( includeSDKFolder, "shared" ) ) );

            // In
            Args.Add( string.Format( " /Tp\"{0}\"", InputFile ) );

            // Start the compile
            processStart.Arguments = string.Join( "", Args );

            Console.WriteLine( "Building " + Path.GetFileName( InputFile ) );

            clProcess.EnableRaisingEvents = true;

            // Enable this for Debugging
            Console.WriteLine( "Command Line: {0}", processStart.Arguments );

            clProcess.OutputDataReceived += new DataReceivedEventHandler( ( _, e ) =>
            {
                if( e.Data != null )
                {
                    Console.WriteLine( e.Data );
                }
            } );

            clProcess.ErrorDataReceived += new DataReceivedEventHandler( ( _, e ) =>
            {
                if( e.Data != null )
                {
                    Console.WriteLine( e.Data );
                }
            } );

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
