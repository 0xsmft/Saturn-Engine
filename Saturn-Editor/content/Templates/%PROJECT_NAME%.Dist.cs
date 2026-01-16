using System;
using SaturnBuildTool;

public class %PROJECT_NAME%Game : Target
{
    public override void Init()
    {
        base.Init();

        Name = "%PROJECT_NAME%";
        Architectures = new[] { ArchitectureKind.x64 };

        BuildConfigs = new[] { ConfigKind.Dist };
        OutputType = LinkerOutput.Executable;

        // When adding modules the name of the module has to match the file name but not the folder name...
        Modules.AddRange( new string[] {
            "Saturn",
            "%PROJECT_NAME%"
        } );

        Links.AddRange( new string[] {
            "ole32.lib",
            "kernel32.lib",
            "comdlg32.lib",
            "shell32.lib",
            "Advapi32.lib",
            "Saturn.lib",
        } );

    }
}
