using System;
using System.Reflection;

[assembly: AssemblyVersion("0.0.2.0")]
[assembly: AssemblyCompany("Saturn Engine")]
[assembly: AssemblyCopyright("2020 - 2023")]
[assembly: AssemblyProduct("Saturn Build Tool")]
[assembly: AssemblyDescription("Saturn Build Tool used for compiling games.")]

namespace SaturnBuildTool
{
    class EntryPoint
    {
        // Args:
        // 0: The Action, BUILD, REBULD, CLEAN
        // 1: The project name
        // 2: The target platform, Win64
        // 3: The configuration, Debug, Release, Dist
        // 4: The project location
        static int Main(string[] args)
        {
            if( IsHelpCommand( args ) ) { return 0; }

            if (args.Length <= 4)
            {
                Console.WriteLine("ERROR: You must provide 5 arguments!");
                return 1;
            }

            // Safe to continue
            Application app = new Application(args);
            
            if(app.Init())
            {
                app.Run();
            }

            return app.ExitCode;
        }

        static bool IsHelpCommand(string[] args) 
        {
            if( args.Length > 0 )
            {
                if ( args[0] == "/HELP" ) 
                {
                    Console.WriteLine("Saturn Build Tool v0.2.0");
                    Console.WriteLine("Valid usage:");
                    Console.WriteLine(" 1) Argument must be /BUILD, /REBUILD or /CLEAN");
                    Console.WriteLine(" 2) Argument must not contain spaces");
                    Console.WriteLine(" 3) Argument must either be /Win64 or /Win86 ");
                    Console.WriteLine(" 4) Argument must be /DEBUG, /RELEASE /DIST");
                    Console.WriteLine(" 5) Argument must path must exist");
                    Console.WriteLine(" 6) You must provide 5 arguments");
                    Console.WriteLine("Arguments");
                    Console.WriteLine(" 1) The Action to do for the project: /BUILD, /REBUILD or /CLEAN");
                    Console.WriteLine(" 2) The Project name");
                    Console.WriteLine(" 3) The Target platform: /Win64 or /Win86 ");
                    Console.WriteLine(" 4) The Target configuration: /DEBUG, /RELEASE /DIST");
                    Console.WriteLine(" 5) The Project location");

                    return true;
                }
            }

            return false;
        }
    }
}
