using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Security.Cryptography;
using System.Text;

using SaturnBuildTool.Auxiliary;
using SaturnBuildTool.Tools;

namespace SaturnBuildTool
{
    internal class MSVCLinkTask : TaskBase
    {
        private readonly LinkSettings LinkSettings;

        public MSVCLinkTask( LinkSettings linkSettings )
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
                UseShellExecute = false,
                WorkingDirectory = Shared.ProjectInfo.RootDirectory
            };

            switch( Shared.ProjectInfo.TargetArchitectureKind )
            {
                default: return 1;

                case ArchitectureKind.x86_64:
                    {
                        processStart.FileName = Path.Combine( toolsDir, "bin", "Hostx64", "x64", "link.exe" );
                    }
                    break;
            }

            Process clProcess = new Process
            {
                StartInfo = processStart

            };

            Args.Add( " /NOLOGO /MACHINE:x64" );

            switch( LinkSettings.OutputType )
            {
                case LinkerOutput.Executable:
                    {
                        if( Shared.ProjectInfo.CurrentConfigKind == ConfigKind.Dist && !CommandLineParser.Instance.FindFlag( "showconsole" ) )
                        {
                            Args.Add( string.Format( " /SUBSYSTEM:WINDOWS /OUT:\"{0}\"", LinkSettings.OutputPath ) );
                        }
                        else
                        {
                            Args.Add( string.Format( " /SUBSYSTEM:CONSOLE /OUT:\"{0}\"", LinkSettings.OutputPath ) );
                        }
                    }
                    break;

                case LinkerOutput.SharedLibrary:
                    {
                        Args.Add( string.Format( " /DLL /OUT:\"{0}\"", LinkSettings.OutputPath ) );
                    }
                    break;
            }

            string sdkLibPath = WindowsSDK.GetLibraryPaths();
            string msvcLibPath = GetMSVCLibraryPath( vcToolchain );

            // MSVC
            Args.Add( $" /LIBPATH:\"{msvcLibPath}\"" );

            // SDK
            Args.Add( string.Format( " /LIBPATH:\"{0}\"", Path.Combine( sdkLibPath, "um", "x64" ) ) );
            Args.Add( string.Format( " /LIBPATH:\"{0}\"", Path.Combine( sdkLibPath, "ucrt", "x64" ) ) );

            foreach( string links in LinkSettings.LibraryPaths )
            {
                Args.Add( string.Format( " /LIBPATH:\"{0}\" ", links ) );
            }

            Args.Add( $" /LIBPATH:\"{LinkSettings.IntermediateDirectory}\"" );

            // Options from Linker Settings
            if( LinkSettings.RemoveUnreferencedFunctions )
            {
                Args.Add( " /OPT:NOREF" );
            }
            else
            {
                Args.Add( " /OPT:REF" );
            }

            if( LinkSettings.IncrementalLink )
            {
                Args.Add( " /INCREMENTAL" );
            }
            else
            {
                Args.Add( " /INCREMENTAL:NO" );
            }

            bool debugLinkDueToConfigOrCmd = Shared.ProjectInfo.CurrentConfigKind != ConfigKind.Dist || CommandLineParser.Instance.FindFlag( "DISTASDBG" );
            if( LinkSettings.IncrementalLink || debugLinkDueToConfigOrCmd )
            {
                Args.Add( " /DEBUG:FULL /PDBALTPATH:%_PDB%" );

                string pdbFile = LinkSettings.GetFullDebugDatabasePath();
                Args.Add( $" /PDB:\"{pdbFile}\"" );
            }
            else
            {
                Args.Add( " /DEBUG:NO" );
            }

            string ilkPath = LinkSettings.OutputPath;
            ilkPath = Path.ChangeExtension( ilkPath, ".ilk" );

            Args.Add( string.Format( " /ILK:\"{0}\"", ilkPath ) );

            foreach( string link in LinkSettings.Links )
            {
                Args.Add( string.Format( " \"{0}\"", link ) );
            }

            // Dynamic base
            if( LinkSettings.DynamicBases.Count > 0 )
            {
                List<string> bases = new List<string>();
                foreach( string file in LinkSettings.DynamicBases )
                {
                    Shared.LinkCache.CacheFile( file );
                }

                Args.Add( string.Format( " /DYNAMICBASE{0}", string.Join( "", bases ) ) );
            }

            // Object files
            foreach( string file in LinkSettings.ObjectFiles )
            {
                Args.Add( string.Format( " \"{0}\"", file ) );
            }

            /*
            string pchOutFile = Path.ChangeExtension( Shared.TargetToBuild.PCH.HeaderFile, ".obj" );
            string pchOutPath = Path.Combine( Shared.CurrentBuildTarget.IntermediateOutputPath, pchOutFile );
            if( !vcToolchain.ProducedItems.Contains( pchOutPath ) )
            {
                Args.Add( $" \"{pchOutPath}\"" );
            }
            */

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
