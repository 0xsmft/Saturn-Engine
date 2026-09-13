using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using SaturnBuildTool.Auxiliary;
using SaturnBuildTool.Tools;

namespace SaturnBuildTool
{
    internal class ARLibrarianTask : TaskBase
    {
        private readonly LinkSettings LinkSettings;

        public ARLibrarianTask( LinkSettings linkSettings )
        {
            LinkSettings = linkSettings;
        }

        public override int Execute( ToolchainBase toolchainBase )
        {
            var Args = new List<string>();

            ProcessStartInfo processStart = new ProcessStartInfo
            {
                CreateNoWindow = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false
            };

            switch( Shared.ProjectInfo.TargetArchitectureKind )
            {
                default:
                    return 1;

                case ArchitectureKind.x86_64:
                case ArchitectureKind.AArch64:
                    {
                        processStart.FileName = "ar";
                    }
                    break;
            }

            Process clProcess = new Process
            {
                StartInfo = processStart
            };

            Args.Add( " -rcs" );

            switch( LinkSettings.OutputType )
            {
                default:
                    {
                        Console.WriteLine( "Invalid output type for librarian task." );
                    }
                    return 0;

                case LinkerOutput.StaticLibrary:
                    {
                        Args.Add( string.Format( " \"{0}\"", LinkSettings.OutputPath ) );
                    }
                    break;
            }

            // Object files
            foreach( string file in toolchainBase.ProducedItems )
            {
                Args.Add( string.Format( " \"{0}\"", file ) );
            }

            // Start the link...
            Console.WriteLine( "Linking as static object" );

            clProcess.EnableRaisingEvents = true;

            processStart.Arguments = string.Join( "", Args );

            if( CommandLineParser.Instance.FindFlag( "args+" ) )
            {
                Console.WriteLine( $"Linking with args: {processStart.Arguments}" );
            }

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
            clProcess.BeginErrorReadLine();
            clProcess.BeginOutputReadLine();
            clProcess.WaitForExit();

            if( clProcess.ExitCode == 0 )
            {
                // Not ideal
                //              Shared.TaskCache.CacheTask( TargetToBuild.GetFullBinPath(), TargetToBuild.GetFullBinPath() );
            }
            else
            {
                //              Shared.TaskCache.RemoveTask( TargetToBuild.GetFullBinPath() );
            }

            return clProcess.ExitCode;
        }

        private string GetMSVCLibraryPath( MSVCToolchain toolchain )
        {
            string CLLocation = toolchain.VCToolsPath;

            switch( Shared.ProjectInfo.TargetArchitectureKind )
            {
                case ArchitectureKind.x86_64:
                    {
                        CLLocation = Path.Combine( CLLocation, "lib", "x64" );
                    }
                    break;
            }

            return CLLocation;
        }
    }
}
