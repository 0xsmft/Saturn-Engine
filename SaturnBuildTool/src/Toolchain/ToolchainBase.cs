namespace SaturnBuildTool
{
    internal class ToolchainBase
    {
        protected UserTarget TargetToBuild;
        public virtual int Compile(string InputFile) { return 0; }
        public virtual int Link() { return 0; }
    }
}
