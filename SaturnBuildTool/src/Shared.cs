using SaturnBuildTool.Cache;

namespace SaturnBuildTool
{
    public static class Shared
    {
        public static Target TargetToBuild = null;

        public static BuildTarget CurrentBuildTarget = null;

        public static ToolchainBase Toolchain = null;

        public static FileCache FileCache = null;

        public static TaskCache TaskCache = null;

        public static ProjectInfo ProjectInfo = null;

        public static Platform Platform = null;

        public static RulesAssembly RulesAssembly = null;
    }
}
