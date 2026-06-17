using System;

namespace SaturnBuildTool
{
    /// <summary>
    /// This class is a helper class to reduce the boilerplate code in the Game's MyProject.Dist.cs file
    /// </summary>
    public class GameDistTarget : Target
    {
        public override void Init()
        {
            base.Init();

            Architectures = new[] { ArchitectureKind.x86_64 };

            BuildConfigs = new[] { ConfigKind.Dist };
            OutputType = LinkerOutput.Executable;

            // When adding modules the name of the module has to match the file name but not the folder name...
            Modules.Add( "Saturn" );
            Links.Add( "Saturn" );

            AddPlatformRequirements();
        }

        private void AddPlatformRequirements() 
        {
            switch( Shared.Platform.PlatformType )
            {
                default: break;

                case PlatformType.Windows:
                {
                    Links.AddRange( new string[] { "ole32", "kernel32", "comdlg32", "shell32", "Advapi32" } );
                } break;
            }
        }
    }
}
