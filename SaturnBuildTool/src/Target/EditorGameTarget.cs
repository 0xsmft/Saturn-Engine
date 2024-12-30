namespace SaturnBuildTool
{
    public class EditorGameTarget : GameUserTarget
    {
        public override void Init() 
        {
            base.Init();

            BuildConfigs = new[] { ConfigKind.Debug, ConfigKind.Release };

            // Games are DLLs in the editor.
            OutputType = LinkerOutput.SharedLibrary;
        }
    }
}
