using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using SaturnBuildTool.Auxiliary;

namespace SaturnBuildTool
{
    internal static class HeaderToolExt
    {
        public static bool RunHeaderTool()
        {
            var Args = new List<string>();

            ProcessStartInfo processStart = new ProcessStartInfo
            {
                CreateNoWindow = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                FileName = Shared.ProjectInfo.HeaderToolExePath
            };

            Process headerToolProcess = new Process
            {
                StartInfo = processStart,
                EnableRaisingEvents = true,
            };

            headerToolProcess.OutputDataReceived += new DataReceivedEventHandler( ( _, e ) =>
            {
                if( e.Data != null )
                {
                    Console.WriteLine( e.Data );
                }
            } );

            headerToolProcess.ErrorDataReceived += new DataReceivedEventHandler( ( _, e ) =>
            {
                if( e.Data != null )
                {
                    Console.WriteLine( e.Data );
                }
            } );

            // Args
            Args.Add( " /NOMSG " );
            Args.Add( string.Concat( " /SRC=", Shared.ProjectInfo.SourceDir ) );
            Args.Add( string.Concat( " /OUT=", Shared.ProjectInfo.HeaderToolGeneratedRootPath ) );
            Args.Add( string.Concat( " /FC=", Path.Combine( Shared.ProjectInfo.BuildDir, $"{Shared.ProjectInfo.Name}-{Shared.ProjectInfo.CurrentConfigKind}.recipe" ) ) );

            switch( Shared.ProjectInfo.CurrentConfigKind )
            {
                default: break;

                case ConfigKind.Debug:
                    {
                        Args.Add( " /DEBUG" );
                    }
                    break;

                case ConfigKind.Release:
                    {
                        Args.Add( " /RELEASE" );
                    }
                    break;

                case ConfigKind.Dist:
                    {
                        Args.Add( " /DIST" );
                    }
                    break;
            }

            if( CommandLineParser.Instance.FindFlag( "HOTRELOAD" ) ) 
            {
                Args.Add( " /HOTRELOAD" );
            }

            processStart.Arguments = string.Join( "", Args );

            Console.WriteLine( "Generating Code..." );
            Stopwatch sw = Stopwatch.StartNew();

            try
            {
                headerToolProcess.Start();
            }
            catch( Exception ex )
            {
                Console.WriteLine( $"Failed to start header tool process: {ex.Message}" );
                Console.WriteLine( "FAILED" );

                return false;
            }

            headerToolProcess.BeginErrorReadLine();
            headerToolProcess.BeginOutputReadLine();
            headerToolProcess.WaitForExit();

            Console.WriteLine( "Done generating in {0}", sw.Elapsed );

            return headerToolProcess.ExitCode == 0;
        }
    }
}
