using System;

namespace SaturnBuildTool
{
    internal class GCCToolchain : ToolchainBase
    {
        public GCCToolchain()
        {
        }

        public override int Compile( string InputFile, CompileSettings compileSettings )
        {
            GCCCompileTask compileTask = new GCCCompileTask( InputFile, compileSettings );

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
            GCCLinkTask link = new GCCLinkTask( linkSettings );

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
