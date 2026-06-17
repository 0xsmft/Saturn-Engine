using System;
using SaturnBuildTool;

public class %PROJECT_NAME%Game : GameDistTarget
{
    public override void Init()
    {
        base.Init();

        Name = "%PROJECT_NAME%Game";

        // When adding modules the name of the module has to match the file name but not the folder name...
        Modules.Add( "%PROJECT_NAME%" );
    }
}
