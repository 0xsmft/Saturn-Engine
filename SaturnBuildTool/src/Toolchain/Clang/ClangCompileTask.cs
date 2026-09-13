using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

using SaturnBuildTool.Auxiliary;
using SaturnBuildTool.Tools;

namespace SaturnBuildTool
{
    internal class ClangCompileTask : TaskBase
    {
        private readonly string InputFile;
        private readonly CompileSettings CompileSettings;

        public ClangCompileTask( string inputFile, CompileSettings compileSettings ) 
        {
            InputFile = inputFile;
            CompileSettings = compileSettings;
        }

        public override int Execute( ToolchainBase toolchainBase )
        {
            ClangToolchain clangToolchain = toolchainBase as ClangToolchain;

            ProcessStartInfo processStart = new ProcessStartInfo();
            switch( Shared.ProjectInfo.TargetArchitectureKind )
            {
                default:
                    {
                        processStart.FileName = "clang++";
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

            var Args = new List<string>();

            // Input
            Args.Add( $" -c \"{InputFile}\"" );

            switch( Shared.ProjectInfo.TargetArchitectureKind )
            {
                case ArchitectureKind.x86_64:
                    {
                        Args.Add( " -arch x86_64" );
                    } break;

                case ArchitectureKind.AArch64:
                    {
                        Args.Add( " -arch arm64" );
                    } break;
            }

            switch( CompileSettings.CppStdVersion ) 
            {
                default:
                case CompileSettings.CppVersion.Minimum:
                    {
                        Args.Add( " -std=c++20" );
                    } break;

                case CompileSettings.CppVersion.Cpp23:
                    {
                        Args.Add( " -std=c++23" );
                    }
                    break;

                case CompileSettings.CppVersion.Latest:
                case CompileSettings.CppVersion.Cpp26:
                    {
                        Args.Add( " -std=c++2c" );
                    }
                    break;
            }

            if( CommandLineParser.Instance.FindFlag( "xw+" ) )
            {
                // Treat warnings as errors
                Args.Add( " -Werror" );
            }

            string outFile = Path.GetFileName( Path.ChangeExtension( InputFile, ".o" ) );

            // Precompiled headers
            // PCHs are set from the target and NOT the module.
            if( CompileSettings.PCHInfo.Valid() )
            {
                switch( CompileSettings.PCHAction )
                {
                    default: break;

                    case CompileSettings.PrecompiledHeaderAction.Create:
                        {
                            outFile += ".pch";
                            Args.Add( " -emit-pch" );
                        }
                        break;

                    case CompileSettings.PrecompiledHeaderAction.Use: 
                        {
                            Args.Add( $" -include-pch \"{CompileSettings.PCHInfo.HeaderFile}\"" );
                        } break;
                }
            }

            // Output
            Args.Add( string.Format( " -o\"{0}\"", Path.Combine( CompileSettings.OutputPath, outFile ) ) );

            if( CompileSettings.X31ShowConsole || CommandLineParser.Instance.FindFlag( "showconsole" ) )
                Args.Add( " -D\"__X31_SHOWCONSOLE__\"" );

            // Learn more: https://clang.llvm.org/docs/CommandGuide/clang.html#code-generation-options
            switch( CompileSettings.Optimisation )
            {
                default:
                case CompileSettings.CppOptimisation.Off:
                    {
                        Args.Add( " -O0" );
                    }
                    break;

                case CompileSettings.CppOptimisation.Debug:
                    {
                        Args.Add( " -O0" );
                    }
                    break;

                case CompileSettings.CppOptimisation.Size:
                    {
                        Args.Add( " -Os" );
                    }
                    break;

                case CompileSettings.CppOptimisation.Speed:
                    {
                        Args.Add( " -O3" );
                    }
                    break;

                case CompileSettings.CppOptimisation.Full:
                    {
                        Args.Add( " -Oz" );
                    } break;
            }

            // Configuration specific
            switch( Shared.ProjectInfo.CurrentConfigKind )
            {
                case ConfigKind.Debug:
                    Args.Add( " -D\"SAT_DEBUG\" -g" );
                    break;
    
                case ConfigKind.Release:
                    Args.Add( " -D\"SAT_RELEASE\" -g" );
                    break;

                case ConfigKind.Dist:
                    {
                        Args.Add( " -D\"SAT_DIST\"" );

                        if( CommandLineParser.Instance.FindFlag( "DISTASDBG" ) )
                        {
                            Args.Add( " -g" );
                        }
                    }
                    break;
            }

            // Preprocessor defines
            foreach( string name in CompileSettings.PreprocessorDefines )
            {
                Args.Add( string.Format( " -D \"{0}\"", name ) );
            }

            // Includes
            foreach( string include in CompileSettings.Includes )
            {
                Args.Add( string.Format( " -I\"{0}\"", include ) );
            }

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
                gccToolchain.ProducedItems.Add( outFile );
                Shared.TaskCache.CacheTask( InputFile, Path.Combine( CompileSettings.OutputPath, outFile ) );
            }
            else
            {
                Shared.TaskCache.RemoveTask( InputFile );
            }

            return clProcess.ExitCode;
        }
    }
}
