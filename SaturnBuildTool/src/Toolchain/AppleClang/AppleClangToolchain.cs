using System;

namespace SaturnBuildTool
{
    internal class AppleClangToolchain : ToolchainBase
    {
        public AppleClangToolchain()
        {
        }

        public override int Compile( string InputFile, CompileSettings compileSettings )
        {
            AppleClangCompileTask compileTask = new AppleClangCompileTask( InputFile, compileSettings );

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
            AppleClangLinkTask link = new AppleClangLinkTask( linkSettings );

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
