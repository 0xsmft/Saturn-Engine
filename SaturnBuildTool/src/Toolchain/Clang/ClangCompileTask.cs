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

            var Args = new List<string>();

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

            // Preprocessor defines
            foreach( string name in CompileSettings.PreprocessorDefines )
            {
                Args.Add( string.Format( " -D \"{0}\"", name ) );
            }

            throw new NotImplementedException();
        }
    }
}
