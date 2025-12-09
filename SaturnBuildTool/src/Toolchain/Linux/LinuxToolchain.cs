using System;

namespace SaturnBuildTool
{
    internal class LinuxToolchain : ToolchainBase
    {
        public LinuxToolchain()
        {
        }

        public override int Compile( string InputFile, CompileSettings compileSettings )
        {
            //MSVCCompileTask compileTask = new MSVCCompileTask(InputFile, TargetToBuild);

            int result = -1;

            try
            {
                //result = compileTask.Execute();
            }
            catch( System.Exception excpt )
            {
                Console.WriteLine( excpt.Message );
            }

            return result;
        }

        public override int Link( LinkSettings linkSettings )
        {
            //MSVCLinkTask link = new MSVCLinkTask(TargetToBuild);

            int result = -1;

            try
            {
                //result = link.Execute();
            }
            catch( System.Exception excpt )
            {
                Console.WriteLine( excpt.Message );
            }

            return result;
        }
    }
}
