using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

using SaturnBuildTool.Tools;

namespace SaturnBuildTool
{
    public class RulesAssembly
    {
        public Assembly Assembly { get; private set; }

        public Dictionary<string, Target> BuildTargets { get; private set; } = new Dictionary<string, Target>();

        public Dictionary<string, Module> Modules { get; private set; } = new Dictionary<string, Module>();

        public static RulesAssembly CompileRules()
        {
            RulesAssembly ra = new RulesAssembly();

            // Search for all C# files
            var csFiles = DirectoryTools.CsSourceSearch( Shared.ProjectInfo.SourceDir, DirectoryTools.CSharpSearchOptions.All );

            ra.Assembly = CSharpCompiler.CompileCSharpFiles( csFiles.ToArray() );

            if( ra.Assembly == null )
                return null;

            Dictionary<string, Target> targets = new Dictionary<string, Target>();
            Dictionary<string, Module> modules = new Dictionary<string, Module>();

            Type[] types = ra.Assembly.GetTypes();
            for( var i = 0; i < types.Length; ++i )
            {
                var type = types[ i ];
                if( !type.IsClass || type.IsAbstract )
                {
                    continue;
                }

                if( type.IsSubclassOf( typeof( Target ) ) )
                {
                    Target target = ( Target ) Activator.CreateInstance( type );
                    target.Init();

                    targets.Add( target.Name, target );
                    Console.WriteLine( $"Found build target: TRGT/{type.Name}" );
                }
                else if( type.IsSubclassOf( typeof( Module ) ) )
                {
                    Module module = ( Module ) Activator.CreateInstance( type );
                    module.Init();

                    modules.Add( module.Name, module );
                    Console.WriteLine( $"Found build target: MODULE/{type.Name}" );
                }
                else
                {
                    Console.WriteLine( $"Skipping unknown class type: ASM/{type.FullName}" );
                }
            }

            ra.Modules = modules;
            ra.BuildTargets = targets;

            // Find best suited TargetToBuild
            foreach( var kv in ra.BuildTargets )
            {
                if( kv.Value.BuildConfigs.Contains( Shared.ProjectInfo.CurrentConfigKind ) )
                {
                    Shared.TargetToBuild = kv.Value;
                }
            }

            return ra;
        }
    }
}
