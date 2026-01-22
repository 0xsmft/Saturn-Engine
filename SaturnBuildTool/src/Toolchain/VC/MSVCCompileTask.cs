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
        private readonly CompileSettings CompileSettings;

        public MSVCCompileTask( string inputFile, CompileSettings compileSettings )
        {
            InputFile = inputFile;
            CompileSettings = compileSettings;
        }

        public override int Execute( ToolchainBase toolchainBase )
        {
            MSVCToolchain vcToolchain = toolchainBase as MSVCToolchain;
            string CLLocation = vcToolchain.VCToolsPath;

            var Args = new List<string>();

            ProcessStartInfo processStart = new ProcessStartInfo();

            switch( Shared.ProjectInfo.TargetPlatformKind )
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
            processStart.WorkingDirectory = Shared.ProjectInfo.RootDirectory;

            Process clProcess = new Process
            {
                StartInfo = processStart
            };

            // nologo: Suppress startup banner,
            // C: Compile,
            // errorreport:prompt: prompt to report edits,
            // Gy: Function level linking,
            // GS: Buffer Security Check,
            // MP: Build with multiple processes,
            // GF: Eliminate Duplicate Strings,
            // EHsc: Exception handling (EH) -- Unwind stack (s) extern "C" function can't throw a C++ exception (c).
            Args.Add( " /nologo /c /errorreport:prompt /Gy /GS /MP /GF /EHsc" );

            // Most important arg here is /Zc:wchar_t, enforce that a wchar_t is a native type and not a typedef!
            Args.Add( " /fp:precise /Zc:wchar_t /Zc:forScope /Zc:inline" );

            switch( CompileSettings.Version )
            {
                default:
                case CompileSettings.CppVersion.Minimum:
                    {
                        Args.Add( " /std:c++20" );
                    }
                    break;

                case CompileSettings.CppVersion.Cpp23:
                    {
                        // NOTE: Would be /std:c++23preview and then /std:c++23
                        // My version of MSVC doesn't have that yet...
                        Args.Add( " /std:c++latest" );
                    }
                    break;

                // NOTE: C++26 support for MSVC would be under c++latest,
                //       this is here when C++26 will become fully supported.
                case CompileSettings.CppVersion.Cpp26:
                    {
                        // await c++26, c++latest for now...
                        //                      Args.Add( " /std:c++26" );
                        Args.Add( " /std:c++latest" );
                    }
                    break;

                case CompileSettings.CppVersion.Latest:
                    {
                        Args.Add( " /std:c++latest" );
                    }
                    break;
            }

            if( CommandLineParser.Instance.FindFlag( "includestree" ) )
            {
                Args.Add( " /showIncludes" );
            }

            if( CommandLineParser.Instance.FindFlag( "xw+" ) )
            {
                // Treat warnings as errors
                Args.Add( " /WX" );
            }

            // Warning level
            Args.Add( ConvertWarningLevel() );

            // Precompiled headers
            // PCHs are set from the target and NOT the module.
            if( CompileSettings.PCHInfo.Valid() )
            {
                switch( CompileSettings.PCHAction )
                {
                    default:
                    case CompileSettings.PrecompiledHeaderAction.NoAction: Debugger.Break(); break;

                    case CompileSettings.PrecompiledHeaderAction.Create: 
                        {
                            Args.Add( $" /Yc\"{CompileSettings.PCHInfo.HeaderFile}\"" );
                        } break;

                    case CompileSettings.PrecompiledHeaderAction.Use: 
                        {
                            Args.Add( $" /Yu\"{CompileSettings.PCHInfo.HeaderFile}\"" );
                        } break;
                }

                // PCHs are global, so we need to use the Targets output path not the module's
                string pchOutFile = Path.GetFileName( Path.ChangeExtension( Shared.ProjectInfo.Name, ".pch" ) );
                Args.Add( string.Format( " /Fp\"{0}\"", Path.Combine( Shared.CurrentBuildTarget.IntermediateOutputPath, pchOutFile ) ) );

                Args.Add( $" /FI{CompileSettings.PCHInfo.HeaderFile}" );
            }

            // Options set from CompileSettings
            if( CompileSettings.ExperimentalFeatures )
                Args.Add( " /experimental" );

            if( CompileSettings.JustMyCodeDebugging )
                Args.Add( " /JMC" );

            if( CompileSettings.X31ShowConsole || CommandLineParser.Instance.FindFlag( "showconsole" ) )
                Args.Add( " /D\"__X31_SHOWCONSOLE__\"" );

            if( CompileSettings.OptimiseGlobalData )
                Args.Add( " /Gw" );

            if( !CompileSettings.EnableEditAndContinue )
                Args.Add( " /Zo" );

            switch( CompileSettings.Optimisation )
            {
                default:
                case CompileSettings.CppOptimisation.Off:
                    {
                        Args.Add( " /Od" );
                    }
                    break;

                case CompileSettings.CppOptimisation.Debug:
                    {
                        Args.Add( " /Od" );
                    }
                    break;

                case CompileSettings.CppOptimisation.Size:
                    {
                        Args.Add( " /O1" );
                    }
                    break;

                case CompileSettings.CppOptimisation.Speed:
                    {
                        Args.Add( " /O2" );
                    }
                    break;

                case CompileSettings.CppOptimisation.Full:
                    {
                        Args.Add( " /Ox" );
                    }
                    break;
            }

            // Configuration specific
            switch( Shared.ProjectInfo.CurrentConfigKind )
            {
                case ConfigKind.Debug:
                case ConfigKind.Release:
                    {
                        if( Shared.ProjectInfo.CurrentConfigKind == ConfigKind.Debug )
                        {
                            Args.Add( " /D\"SAT_DEBUG\"" );
                            Args.Add( " /MDd" ); // Multithreaded debug RT
                        }
                        else
                        {
                            Args.Add( " /D\"SAT_RELEASE\"" );
                            Args.Add( " /MD" ); // Multithreaded RT
                        }

                        Args.Add( " /Z7" ); // Build with C7 debug pdbs
                        Args.Add( " /FS" ); // Force Synchronous PDB Writes
                    }
                    break;

                case ConfigKind.Dist:
                    {
                        Args.Add( " /D\"SAT_DIST\"" );
                        Args.Add( " /MD" ); // Multithreaded RT
                    }
                    break;
            }

            // Suppress warning:
            // 'type': 'type1' needs to have dll-interface to be used by clients of 'type2'
            // See: https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-1-c4251
            Args.Add( " /wd4251" );

            // Preprocessor defines
            foreach( string name in CompileSettings.PreprocessorDefines )
            {
                Args.Add( string.Format( " /D\"{0}\"", name ) );
            }

            // Includes
            foreach( string include in CompileSettings.Includes )
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

            // Out
            string outFile = Path.GetFileName( Path.ChangeExtension( InputFile, ".obj" ) );
            Args.Add( string.Format( " /Fo\"{0}\"", Path.Combine( CompileSettings.OutputPath, outFile ) ) );

            // In
            Args.Add( string.Format( " /Tp\"{0}\"", InputFile ) );

            // Start the compile
            processStart.Arguments = string.Join( "", Args );

            Console.WriteLine( "Building " + Path.GetFileName( InputFile ) );

            // Enable this for Debugging
            clProcess.EnableRaisingEvents = true;

            if( CommandLineParser.Instance.FindFlag( "args+" ) )
            {
                Console.WriteLine( "Command Line: {0}", processStart.Arguments );
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

            // Debugging
            clProcess.BeginErrorReadLine();
            clProcess.BeginOutputReadLine();

            clProcess.WaitForExit();

            // Write error output (disable this when using synchronous output)
            //Console.WriteLine(clProcess.StandardOutput.ReadToEnd().Trim());

            if( clProcess.ExitCode == 0 )
            {
                vcToolchain.ProducedItems.Add( outFile );
                Shared.TaskCache.CacheTask( InputFile, Path.Combine( CompileSettings.OutputPath, outFile ) );
            }
            else
            {
                Shared.TaskCache.RemoveTask( InputFile );
            }

            return clProcess.ExitCode;
        }

        private string ConvertWarningLevel()
        {
            if( CommandLineParser.Instance.FindFlag( "/XW+" ) )
            {
                return " /WX";
            }

            if( CommandLineParser.Instance.FindFlag( "/XW-" ) )
            {
                return " /w";
            }

            if( CommandLineParser.Instance.FindFlag( "/XW1" ) )
            {
                return " /W1";
            }

            if( CommandLineParser.Instance.FindFlag( "/XW2" ) )
            {
                return " /W2";
            }

            if( CommandLineParser.Instance.FindFlag( "/XW3" ) )
            {
                return " /W3";
            }

            if( CommandLineParser.Instance.FindFlag( "/XW4" ) )
            {
                return " /W4";
            }

            if( CommandLineParser.Instance.FindFlag( "/XWAx" ) )
            {
                return " /Wall";
            }

            return " /W3";
        }
    }
}
