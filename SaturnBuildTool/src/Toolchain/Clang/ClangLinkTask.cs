using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Security.Cryptography;
using System.Text;

using SaturnBuildTool.Auxiliary;

namespace SaturnBuildTool
{
    internal class ClangLinkTask : TaskBase
    {
        private readonly LinkSettings LinkSettings;

        public ClangLinkTask( LinkSettings linkSettings )
        {
            LinkSettings = linkSettings;
        }

        public override int Execute( ToolchainBase toolchainBase )
        {
            var Args = new List<string>();

            ProcessStartInfo processStart = new ProcessStartInfo();
            processStart.FileName = "clang++";
            processStart.CreateNoWindow = true;
            processStart.RedirectStandardOutput = true;
            processStart.RedirectStandardError = true;
            processStart.UseShellExecute = false;
            processStart.WorkingDirectory = Shared.ProjectInfo.RootDirectory;

            Process clProcess = new Process
            {
                StartInfo = processStart
            };

            Args.Add( $" -o \"{LinkSettings.OutputPath}\"" );

            switch( LinkSettings.OutputType )
            {
                case LinkerOutput.SharedLibrary:
                    {
                        Args.Add( $" -shared -Wl,-soname={LinkSettings.OutputName}" );
                    } break;
            }

            // Object files
            foreach( string file in LinkSettings.ObjectFiles )
            {
                Args.Add( string.Format( " \"{0}\"", file ) );
            }

            foreach( string libPath in LinkSettings.LibraryPaths )
            {
                Args.Add( string.Format( " -L\"{0}\" ", libPath ) );
            }

            Args.Add( " -Wl,--start-group" );
            foreach( string link in LinkSettings.Links )
            {
                Args.Add( string.Format( " -l\"{0}\"", link ) );
            }
            Args.Add( " -Wl,--end-group" );

            foreach( string rpath in LinkSettings.RTLibraryPaths )
            {
                Args.Add( string.Format( " -Wl,-rpath,{0}", rpath ) );
            }

            switch( Shared.ProjectInfo.TargetArchitectureKind )
            {
                case ArchitectureKind.x86_64:
                    {
                        Args.Add( " -m64 -L/usr/lib64" );
                    } break;

                case ArchitectureKind.AArch64:
                    {
                        Args.Add( " -march=armv8-a" );
                    } break;
            }

            // Start the link...
            Console.WriteLine( "Linking" );

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
                // For final link outputs, we hash key instead of storing a folder.
                // TODO: Not great...
                SHA256 sha = SHA256.Create();
                byte[] hash = sha.ComputeHash( Encoding.UTF8.GetBytes( LinkSettings.Name ) );

                Shared.TaskCache.CacheTask( BitConverter.ToString( hash ).Replace( "-", string.Empty ), LinkSettings.OutputPath );
            }
            else
            {
                Shared.TaskCache.RemoveTask( LinkSettings.OutputDirectory );
            }

            return clProcess.ExitCode;
        }
    }
}
