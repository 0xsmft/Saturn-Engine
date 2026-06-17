using System;
using System.IO;

using SaturnBuildTool;

public class %PROJECT_NAME%Module : GameModule
{
    public override void Init()
    {
        base.Init();

        // Add our source dir as an include.
        Includes.Add( "Source/%PROJECT_NAME%" );

        // Add our source dir.
        SourcePaths.Add( "Source/%PROJECT_NAME%" );

        // NOTE: When using UseTargetDirectory we don't have to specify a SourcePath
        SourceDirectoryOptions = SourceDirectoryOptions.Custom;
        OutputDirectoryOptions = OutputDirectoryOptions.UseTargetDirectory;
    }
}
