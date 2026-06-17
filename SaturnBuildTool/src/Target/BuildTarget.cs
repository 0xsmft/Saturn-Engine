using System;
using System.Collections.Generic;
using System.IO;

namespace SaturnBuildTool
{
    /// <summary>
    /// </summary>
    public class BuildTarget
    {
        /// <summary>
        /// The rules which we use to configure this to be sent to the linker/compiler.
        /// </summary>
        public readonly Target TargetRules;

        /// <summary>
        /// Is this build target the one that WILL be get built.
        /// </summary>
        public bool IsTargetToBuild { get; set; }

        /// <summary>
        /// The absolute output path to this target.
        /// </summary>
        public string IntermediateOutputPath { get; set; }

        /// <summary>
        /// The _global_ map for modules
        /// NOTE: Modules that need other modules, will use this map as a build Module does not contain a map with modules...
        /// </summary>
        public Dictionary<string, BuildModule> Modules = new Dictionary<string, BuildModule>();

        /// <summary>
        /// The compile settings for this target
        /// </summary>
        public CompileSettings TargetCompileSettings { get; set; }

        /// <summary>
        /// The link settings for this target
        /// </summary>
        public LinkSettings TargetLinkSettings { get; set; }

        /// <summary>
        /// Hot reloading only, the time at which this target was built.
        /// </summary>
        public int Timestamp { get; } = ( int ) DateTime.UtcNow.Subtract( new DateTime( 1970, 1, 1 ) ).TotalSeconds;

        public BuildTarget( Target target )
        {
            TargetRules = target;
            IsTargetToBuild = true;

            IntermediateOutputPath = Path.Combine( Shared.ProjectInfo.RootDirectory, "bin-int", Shared.Platform.GetOutputFolderName( Shared.ProjectInfo.CurrentConfigKind ), TargetRules.Name );
        }

        public void Init()
        {
            TargetCompileSettings = new CompileSettings(
                Shared.ProjectInfo.CurrentConfigKind,
                true,
                false,
                true,
                false,
                true,
                false,
                CompileSettings.CppOptimisation.Off,
                CompileSettings.CppVersion.Latest,
                CompileSettings.PrecompiledHeaderAction.NoAction,
                new CompileSettings.FPCHInfo(),
                TargetRules.PreprocessorDefines,
                TargetRules.Includes,
                IntermediateOutputPath
            );

            TargetLinkSettings = new LinkSettings(
                true,
                true,
                true,
                TargetRules.GetBinDir(),
                TargetRules.Name,
                TargetRules.Name,
                IntermediateOutputPath,
                TargetRules.OutputType,
                TargetRules.LibraryPaths,
                TargetRules.Links,
                TargetRules.DynamicBase,
                Timestamp
            );

            foreach( var moduleName in TargetRules.Modules )
            {
                if( Shared.RulesAssembly.Modules.TryGetValue( moduleName, out var module ) )
                {
                    Modules.Add( moduleName, new BuildModule( this, module ) );
                }
            }

            foreach( var kv in Modules )
            {
                kv.Value.PostInit( this );
            }

            ResolveLinks();
        }

        private void ResolveLinks()
        {
            for( int i = 0; i < TargetLinkSettings.Links.Count; ++i )
            {
                var link = TargetLinkSettings.Links[ i ];
                
                // If we do not have an extension then we add it.
                if( !Path.HasExtension( link ) ) 
                {
                    TargetLinkSettings.Links[ i ] += Shared.Platform.StaticLibraryExtension;
                }
            }
        }

        public static BuildTarget Create( Target target )
        {
            return new BuildTarget( target );
        }
    }
}
