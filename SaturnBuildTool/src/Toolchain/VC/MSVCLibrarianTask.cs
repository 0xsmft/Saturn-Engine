using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

using SaturnBuildTool.Auxiliary;
using SaturnBuildTool.Tools;

namespace SaturnBuildTool
{
    internal class MSVCLibrarianTask : TaskBase
    {
        private readonly LinkSettings LinkSettings;

        public MSVCLibrarianTask( LinkSettings linkSettings )
        {
            LinkSettings = linkSettings;
        }

        public override int Execute( ToolchainBase toolchainBase )
        {
            MSVCToolchain vcToolchain = toolchainBase as MSVCToolchain;

            var Args = new List<string>();

            string toolsDir = vcToolchain.VCToolsPath;

            ProcessStartInfo processStart = new ProcessStartInfo
            {
                CreateNoWindow = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false
            };

            switch( Shared.ProjectInfo.TargetPlatformKind )
            {
                default:
                case ArchitectureKind.x64:
                    {
                        processStart.FileName = Path.Combine( toolsDir, "bin", "Hostx64", "x64", "lib.exe" );
                    }
                    break;

                case ArchitectureKind.x86:
                    {
                        processStart.FileName = Path.Combine( toolsDir, "bin", "Hostx86", "x86", "lib.exe" );
                    }
                    break;
            }

            Process clProcess = new Process
            {
                StartInfo = processStart
            };

            switch( LinkSettings.OutputType )
            {
                default:
                    {
                        Console.WriteLine( "Invalid output type for librarian task." );
                    }
                    return 0;

                case LinkerOutput.StaticLibrary:
                    {
                        Args.Add( string.Format( " /OUT:\"{0}\"", LinkSettings.OutputPath ) );
                    }
                    break;
            }

            Args.Add( " /NOLOGO" );
            Args.Add( " /MACHINE:x64" );

            // std libraries
            string sdkLibPath = WindowsSDK.GetLibraryPaths();
            string msvcLibPath = GetMSVCLibraryPath( vcToolchain );

            // MSVC
            Args.Add( $" /LIBPATH:\"{msvcLibPath}\"" );

            // SDK
            Args.Add( string.Format( " /LIBPATH:\"{0}\"", Path.Combine( sdkLibPath, "um", "x64" ) ) );
            Args.Add( string.Format( " /LIBPATH:\"{0}\"", Path.Combine( sdkLibPath, "ucrt", "x64" ) ) );

            foreach( string links in LinkSettings.LibraryPaths )
            {
                Args.Add( string.Format( " /LIBPATH:\"{0}\"", links ) );
            }

            foreach( string links in LinkSettings.Links )
            {
                Args.Add( string.Format( " \"{0}\"", links ) );
            }

            // Dynamic base
            if( LinkSettings.DynamicBases.Count > 0 )
            {
                List<string> bases = new List<string>();

                foreach( string file in LinkSettings.DynamicBases )
                {
                    bases.Add( string.Format( " \"{0}\"", file ) );
                }

                Args.Add( string.Format( " /DYNAMICBASE{0}", string.Join( "", bases ) ) );
            }

            // Object files
            foreach( string file in vcToolchain.ProducedItems )
            {
                Args.Add( string.Format( " \"{0}\"", file ) );
            }

            // Start the link...
            Console.WriteLine( "Linking as static library" );

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

            switch( Shared.ProjectInfo.TargetPlatformKind )
            {
                case ArchitectureKind.x64:
                    {
                        CLLocation = Path.Combine( CLLocation, "lib", "x64" );
                    }
                    break;

                case ArchitectureKind.x86:
                    {
                        CLLocation = Path.Combine( CLLocation, "lib", "x86" );
                    }
                    break;
            }

            return CLLocation;
        }
    }
}
