using System;
using System.Diagnostics;
using SaturnBuildTool.Tools;

namespace SaturnBuildTool
{
    internal class MSVCToolchain : ToolchainBase
    {
        public string VCToolsPath { get; }

        public MSVCToolchain()
        {
            VCToolsPath = VSWhere.FindMSVCToolsDir();
        }

        public override int Compile( string InputFile, CompileSettings settings )
        {
            MSVCCompileTask compileTask = new MSVCCompileTask( InputFile, settings );

            int result = -1;

            try
            {
                result = compileTask.Execute( this );
            }
            catch( System.Exception excpt )
            {
                Console.WriteLine( excpt.Message );
                Debugger.Break();
            }

            return result;
        }

        public override int Link( LinkSettings linkSettings )
        {
            switch( linkSettings.OutputType )
            {
                default:
                case LinkerOutput.SharedLibrary:
                case LinkerOutput.Executable:
                    return LinkInternal( linkSettings );

                case LinkerOutput.StaticLibrary:
                    return LinkWithLib( linkSettings );
            }
        }

        private int LinkInternal( LinkSettings linkSettings )
        {
            MSVCLinkTask link = new MSVCLinkTask( linkSettings );

            int result = -1;

            try
            {
                result = link.Execute( this );
            }
            catch( System.Exception excpt )
            {
                Console.WriteLine( excpt.Message );
                Debugger.Break();
            }

            return result;
        }

        private int LinkWithLib( LinkSettings linkSettings )
        {
            MSVCLibrarianTask lib = new MSVCLibrarianTask( linkSettings );

            int result = -1;

            try
            {
                result = lib.Execute( this );
            }
            catch( System.Exception excpt )
            {
                Console.WriteLine( excpt.Message );
                Debugger.Break();
            }

            return result;
        }
    }
}
