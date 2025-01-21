using System;

namespace SaturnBuildTool
{
    internal class MSVCToolchain : ToolchainBase
    {
        public MSVCToolchain( UserTarget target )
        {
            TargetToBuild = target;
        }

        public override int Compile(string InputFile)
        {
            MSVCCompileTask compileTask = new MSVCCompileTask( InputFile, TargetToBuild );

            int result = -1;

            try
            {
                result = compileTask.Execute();
            }
            catch(System.Exception excpt)
            {
                Console.WriteLine(excpt.Message);
            }

            return result;
        }

        public override int Link()
        {
            MSVCLinkTask link = new MSVCLinkTask( TargetToBuild );

            int result = -1;

            try
            {
                result = link.Execute();
            }
            catch (System.Exception excpt)
            {
                Console.WriteLine(excpt.Message);
            }

            return result;
        }
    }
}
