using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

using SaturnBuildTool.Tools;

namespace SaturnBuildTool
{
    internal class MSVCLinkTask : TaskBase
    {
        private readonly UserTarget TargetToBuild;

        public MSVCLinkTask( UserTarget target )
        {
            TargetToBuild = target;
        }

        public override int Execute()
        {
            var Args = new List<string>();

            string LinkLocation = VSWhere.FindMSVCToolsDir();

            ProcessStartInfo processStart = new ProcessStartInfo();
            processStart.CreateNoWindow = true;

            switch( ProjectInfo.Instance.TargetPlatformKind )
            {
                default:
                case ArchitectureKind.x64:
                    {
                        processStart.FileName = LinkLocation + "/bin/Hostx64/x64/link.exe";
                    }
                    break;

                case ArchitectureKind.x86:
                    {
                        processStart.FileName = LinkLocation + "/bin/Hostx64/x86/link.exe";
                    }
                    break;
            }

            processStart.CreateNoWindow = false;
            processStart.RedirectStandardOutput = true;
            processStart.RedirectStandardError = true;
            processStart.UseShellExecute = false;

            Process clProcess = new Process();
            clProcess.StartInfo = processStart;

            Args.Add( " /NOLOGO" );

            // std libraries
            string sdkLibPath = WindowsSDK.GetLibraryPaths();
            string msvcLibPath = GetMSVCLibraryPath();

            // MSVC
            Args.Add( string.Format( " /LIBPATH:\"{0}\"", msvcLibPath ) );

            // SDK
            Args.Add( string.Format( " /LIBPATH:\"{0}\"", sdkLibPath + "/um" + "/x64" ) );
            Args.Add( string.Format( " /LIBPATH:\"{0}\"", sdkLibPath + "/ucrt" + "/x64" ) );

            foreach( string links in TargetToBuild.LibraryPaths )
            {
                Args.Add( string.Format( " /LIBPATH:\"{0}\"", links ) );
            }

            Args.Add( " /MACHINE:x64" );

            if( TargetToBuild.CurrentConfig != ConfigKind.Dist )
            {
                Args.Add( " /PDBALTPATH:%_PDB%" );
                Args.Add( " /INCREMENTAL" );
                Args.Add( " /DEBUG:FULL" );

                string pdbFile = TargetToBuild.GetFullPDBPath();
                Args.Add( $" /PDB:\"{pdbFile}\"" );
            }
            else
            {
                Args.Add( " /DEBUG:NONE" );
                Args.Add( " /INCREMENTAL:NO" );
            }

            switch( TargetToBuild.OutputType )
            {
                case LinkerOutput.Executable:
                    {
                        if( ProjectInfo.Instance.CurrentConfigKind == ConfigKind.Dist )
                        {
                            Args.Add( string.Format( " /SUBSYSTEM:WINDOWS /OUT:\"{0}\"", TargetToBuild.GetFullBinPath() ) );
                        }
                        else
                        {
                            Args.Add( string.Format( " /SUBSYSTEM:CONSOLE /OUT:\"{0}\"", TargetToBuild.GetFullBinPath() ) );
                        }
                    }
                    break;


                case LinkerOutput.StaticLibrary:
                    {
                        Args.Add( string.Format( " /LIB /OUT:\"{0}\"", TargetToBuild.GetFullBinPath() ) );
                    }
                    break;

                case LinkerOutput.SharedLibrary:
                    {
                        Args.Add( string.Format( " /DLL /OUT:\"{0}\"", TargetToBuild.GetFullBinPath() ) );
                    }
                    break;
            }

            string ilkPath = Path.Combine( TargetToBuild.OutputPath, TargetToBuild.ProjectName );
            ilkPath = Path.ChangeExtension( ilkPath, ".ilk" );

            Args.Add( string.Format( " /ILK:\"{0}\"", ilkPath ) );

            foreach( string links in TargetToBuild.Links )
            {
                Args.Add( string.Format( " \"{0}\"", links ) );
            }

            // Dynamic base
            if( TargetToBuild.DynamicBase.Count > 0 )
            {
                List<string> bases = new List<string>();

                foreach( string file in TargetToBuild.DynamicBase )
                {
                    bases.Add( string.Format( " \"{0}\"", file ) );
                }

                Args.Add( string.Format( " /DYNAMICBASE{0}", string.Join( "", bases ) ) );
            }

            // Object files
            foreach( string file in TargetToBuild.GetIntermediateFiles() )
            {
                Args.Add( string.Format( " \"{0}\"", file ) );
            }

            // Start the link...
            Console.WriteLine( "Linking" );

            clProcess.EnableRaisingEvents = true;

            processStart.Arguments = string.Join( "", Args );

            Console.WriteLine( $"Linking with args: {processStart.Arguments}" );

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

            return clProcess.ExitCode;
        }

        private string GetMSVCLibraryPath()
        {
            string CLLocation = VSWhere.FindMSVCToolsDir();

            switch( ProjectInfo.Instance.TargetPlatformKind )
            {
                case ArchitectureKind.x64:
                    {
                        CLLocation += "/lib/x64";
                    }
                    break;

                case ArchitectureKind.x86:
                    {
                        CLLocation += "/lib/x86";
                    }
                    break;
            }

            return CLLocation;
        }
    }
}
