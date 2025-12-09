using System.Collections.Generic;

namespace SaturnBuildTool
{
    public class ToolchainBase
    {
        /// <summary>
        /// The list of items that this toolchain has created.
        /// </summary>
        public List<string> ProducedItems = new List<string>();

        public virtual int Compile( string InputFile, CompileSettings compileSettings ) { return 0; }
        public virtual int Link( LinkSettings linkSettings ) { return 0; }
    }
}
