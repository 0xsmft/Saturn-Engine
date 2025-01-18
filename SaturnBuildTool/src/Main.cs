using System.Reflection;

[assembly: AssemblyVersion("0.0.3.0")]
[assembly: AssemblyCompany("Saturn Engine")]
[assembly: AssemblyCopyright("2020 - 2023")]
[assembly: AssemblyProduct("Saturn Build Tool")]
[assembly: AssemblyDescription("Saturn Build Tool used for compiling games.")]

namespace SaturnBuildTool
{
    class EntryPoint
    {
        static int Main(string[] args)
        {
            Application app = new Application(args);
            
            if(app.Init())
            {
                app.Run();
            }

            return app.ExitCode;
        }
    }
}
