using System.Collections.Generic;
using System.IO;
using SaturnBuildTool.Tools;

namespace SaturnBuildTool
{
    /// <summary>
    /// </summary>
    public class BuildModule
    {
        /// <summary>
        /// The rules which we use to configure this to be sent to the link/compiler.
        /// </summary>
        public readonly Module ModuleRules;

        /// <summary>
        /// The name of the *_API macro.
        /// </summary>
        public string ApiName { get; set; }

        /// <summary>
        /// The output name for this module.
        /// Most of the time the name is similar to the Target, it may look like {TargetName}-{ModuleName}
        /// </summary>
        public string OutputName { get; set; }

        /// <summary>
        /// The absolute output path of this module.
        /// </summary>
        public string OutputPath { get; set; }

        /// <summary>
        /// The compile settings for this module
        /// </summary>
        public CompileSettings ModuleCompileSettings { get; set; }

        /// <summary>
        /// 
        /// </summary>
        public CompileSettings PCHCompileSettings { get; set; }
        
        /// <summary>
        /// The link settings for this module
        /// </summary>
        public LinkSettings ModuleLinkSettings { get; set; }

        public BuildModule( BuildTarget parent, Module module )
        {
            ModuleRules = module;
            ApiName = $"{ModuleRules.Name.ToUpperInvariant()}_API";
            OutputName = $"{module.Name}-{parent.TargetRules.Name}";

            switch( module.OutputDirectoryOptions )
            {
                default:
                case OutputDirectoryOptions.Default:
                    {
                        OutputPath = Path.Combine( parent.IntermediateOutputPath, module.Name );
                    } break;

                case OutputDirectoryOptions.UseTargetDirectory:
                    {
                        OutputPath = parent.IntermediateOutputPath;
                    } break;
            }

            if( !Directory.Exists( OutputPath ) )
                Directory.CreateDirectory( OutputPath );
        }

        public void PostInit( BuildTarget parent )
        {
            InitCompileSettings( parent );
            if( ModuleRules.CompiledInDirectly )
            {
                // Add output path as a path for the target.
                parent.TargetLinkSettings.LibraryPaths.Add( ModuleCompileSettings.OutputPath );
            }
            else
            {
                InitLinkSettings( parent );
            }

            List<BuildModule> modules = new List<BuildModule>();
            foreach( var moduleName in ModuleRules.Modules )
            {
                // Go to the target for the module
                if( parent.Modules.TryGetValue( moduleName, out var parentMod ) )
                {
                    modules.Add( parentMod );
                }
                // Go to the rules assembly for the module
                else
                {
                    if( Shared.RulesAssembly.Modules.TryGetValue( moduleName, out var module ) )
                    {
                        BuildModule bm = new BuildModule( parent, module );

                        modules.Add( bm );
                        parent.Modules.Add( moduleName, bm );
                    }
                }
            }

            foreach( var module in modules )
            {
                module.PostInit( parent );

                if( ModuleRules.CompiledInDirectly )
                {
                    // If we need a module, we must add it to the target because that's what we'll be linked into.
                    AppendLinkForSubModuleCID( parent, module );
                }
                else
                {
                    AppendLinkForSubModuleNonCID( module );
                }
            }
        }

        private void AppendLinkForSubModuleCID( BuildTarget parent, BuildModule module )
        {
            parent.TargetLinkSettings.LibraryPaths.Add( module.ModuleLinkSettings.OutputDirectory );
            string libFilename = Path.ChangeExtension( module.ModuleLinkSettings.OutputName, Shared.Platform.StaticLibraryExtension );
            parent.TargetLinkSettings.Links.Add( libFilename );
        }

        private void AppendLinkForSubModuleNonCID( BuildModule module )
        {
            ModuleLinkSettings.LibraryPaths.Add( module.ModuleLinkSettings.OutputDirectory );

            string libFilename = Path.ChangeExtension( module.ModuleLinkSettings.OutputName, Shared.Platform.StaticLibraryExtension );
            ModuleLinkSettings.Links.Add( libFilename );
        }

        private void InitCompileSettings( BuildTarget parent )
        {
            List<string> fullPreprocessorDefines = new List<string>();
            fullPreprocessorDefines.AddRange( ModuleRules.PreprocessorDefines );
            fullPreprocessorDefines.AddRange( parent.TargetRules.PreprocessorDefines );

            // Hard coded for now because some of these options aren't yet accessible from the ModuleRules
            ModuleCompileSettings = new CompileSettings( 
                Shared.ProjectInfo.CurrentConfigKind, 
                true, 
                false, 
                true, 
                false, 
                true, 
                false, 
                CompileSettings.CppOptimisation.Off, 
                CompileSettings.CppVersion.Latest, 
                CompileSettings.PrecompiledHeaderAction.Use,
                new CompileSettings.FPCHInfo( ModuleRules.PCH.HeaderFile, ModuleRules.PCH.SourceFile ),
                fullPreprocessorDefines, 
                ModuleRules.Includes, 
                OutputPath 
            );

            PCHCompileSettings = new CompileSettings( ModuleCompileSettings )
            {
                PCHAction = CompileSettings.PrecompiledHeaderAction.Create
            };
        }

        private void InitLinkSettings( BuildTarget parent )
        {
            List<string> fullLinks = new List<string>();
            fullLinks.AddRange( parent.TargetRules.Links );
            fullLinks.AddRange( ModuleRules.Links );

            ModuleLinkSettings = new LinkSettings(
                true,
                true,
                true,
                GetFullBinaryPath(),
                OutputName,
                ModuleRules.Name,
                OutputPath,
                ModuleRules.OutputType,
                ModuleRules.LibraryPaths,
                fullLinks,
                ModuleRules.DynamicBase
            );
        }

        public string GetFullDebugDatabasePath()
        {
            return Path.Combine( Shared.ProjectInfo.RootDirectory, "bin", Shared.Platform.GetOutputFolderName( Shared.ProjectInfo.CurrentConfigKind ), Shared.TargetToBuild.Name ) + ModuleRules.Name + Shared.Platform.ProgramDebugDatabaseExtension;
        }

        public string GetFullBinaryPath()
        {
            return Path.Combine( Shared.ProjectInfo.RootDirectory, "bin", Shared.Platform.GetOutputFolderName( Shared.ProjectInfo.CurrentConfigKind ), Shared.TargetToBuild.Name );
        }

        public List<string> GetIntermediateFilesFromOutDir()
        {
            return DirectoryTools.DirSearch( OutputPath, Shared.Platform.ObjectFileExtension );
        }

        public void AppendOutputs() 
        {
            List<string> list;
            if( ModuleRules.CompiledInDirectly )
            {
                list = Shared.CurrentBuildTarget.TargetLinkSettings.ObjectFiles;
            }
            else 
            {
                list = ModuleLinkSettings.ObjectFiles;
            }

            // First, search the TaskCache
            var filesCreatedFromThisBuild = Shared.TaskCache.GetOutputItemFromSource( OutputPath );
            list.AddRange( filesCreatedFromThisBuild );
        }
    }
}
