using System;
using SaturnBuildTool;

public class %PROJECT_NAME%Editor : GameDevelopmentTarget
{
    public override void Init()
    {
        base.Init();

        Name = "%PROJECT_NAME%";

        // When adding modules the name of the module has to match the file name but not the folder name...
        Modules.Add( "%PROJECT_NAME%" );
        // Include directories relative to root folder (solution directory)
    
        Includes.Add( "%PROJECT_NAME%/Source/%PROJECT_NAME%" );
    }
}
