using System;

namespace SaturnBuildTool
{
    internal class ClangToolchain : ToolchainBase
    {
        public ClangToolchain()
        {
        }

        public override int Compile( string InputFile, CompileSettings compileSettings )
        {
            ClangCompileTask compileTask = new ClangCompileTask( InputFile, compileSettings );

            int result = -1;
            try
            {
                result = compileTask.Execute( this );
            }
            catch( System.Exception excpt )
            {
                Console.WriteLine( excpt.Message );
            }

            return result;
        }

        public override int Link( LinkSettings linkSettings )
        {
            ClangLinkTask link = new ClangLinkTask( linkSettings );

            int result = -1;
            try
            {
                result = link.Execute( this );
            }
            catch( System.Exception excpt )
            {
                Console.WriteLine( excpt.Message );
            }

            return result;
        }
    }
}
