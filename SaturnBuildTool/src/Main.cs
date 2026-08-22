using System.Reflection;

[assembly: AssemblyCopyright( "2020 - 2023" )]

namespace SaturnBuildTool
{
    static class EntryPoint
    {
        static int Main( string[] args )
        {
            Application app = new Application( args );

            if( app.Init() )
            {
                app.Run();
            }

            return ( int ) app.ExitCode;
        }
    }
}
