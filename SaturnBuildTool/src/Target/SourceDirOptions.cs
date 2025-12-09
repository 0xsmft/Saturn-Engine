namespace SaturnBuildTool
{
    public enum SourceDirectoryOptions 
    {
        /// <summary>
        /// The source will be the combined with the targets path and then the modules name.
        /// </summary>
        Default,

        /// <summary>
        /// A custom path for the source
        /// NOTE: When using this it must be relative to the root directory of the solution
        /// </summary>
        Custom,

        /// <summary>
        /// The source path will equal the targets path.
        /// </summary>
        UseTargetDirectory
    }

    public enum OutputDirectoryOptions
    {
        /// <summary>
        /// The output directory will be combined with the targets output and the modules name.
        /// So a path could look like: C:\MyProject\bin-int\{config}\MyTarget\MyModule
        /// </summary>
        Default, 

        /// <summary>
        /// The output items will be placed in the targets path.
        /// So a path could look like: C:\MyProject\bin-int\{config}\MyTarget
        /// </summary>
        UseTargetDirectory
    }

}
