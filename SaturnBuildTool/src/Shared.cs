using SaturnBuildTool.Cache;

namespace SaturnBuildTool
{
    public static class Shared
    {
        public static Target TargetToBuild;

        public static BuildTarget CurrentBuildTarget;

        public static ToolchainBase Toolchain;

        public static FileCache FileCache;

        public static TaskCache TaskCache;

        public static LinkCache LinkCache;

        public static ProjectInfo ProjectInfo;

        public static Platform Platform;

        public static RulesAssembly RulesAssembly;
    }
}
