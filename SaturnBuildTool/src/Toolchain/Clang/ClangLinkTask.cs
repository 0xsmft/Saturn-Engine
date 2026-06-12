using System;

namespace SaturnBuildTool
{
    internal class ClangLinkTask : TaskBase
    {
        private readonly LinkSettings LinkSettings;

        public ClangLinkTask( LinkSettings linkSettings )
        {
            LinkSettings = linkSettings;
        }

        public override int Execute( ToolchainBase toolchainBase )
        {
            return 0;
        }
    }
}
