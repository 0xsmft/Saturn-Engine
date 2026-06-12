using System;

namespace SaturnBuildTool
{
    internal class GCCLinkTask : TaskBase
    {
        private readonly LinkSettings LinkSettings;

        public GCCLinkTask( LinkSettings linkSettings )
        {
            LinkSettings = linkSettings;
        }

        public override int Execute( ToolchainBase toolchainBase )
        {
            return 0;
        }
    }
}
