namespace SaturnBuildTool
{
    public class RuntimeGameTarget : GameUserTarget
    {
        public override void Init()
        {
            base.Init();

            BuildConfigs = new[] { ConfigKind.Dist };

            // Games are exes outside of the editor.
            OutputType = LinkerOutput.Executable;

            // Remove Tracy because this will never be used in Dist
            // And is only to be used in Debug and Release
            Links.Remove( "Tracy.lib" );

            // Shared storage will be linked statically.
            PreprocessorDefines.Add("SATURN_SS_STATIC");
            PreprocessorDefines.Remove("SATURN_SS_IMPORT");
        }
    }
}
