using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection.Emit;
using System.Runtime.Remoting.Messaging;
using System.Text;

namespace SaturnBuildTool
{
    public class CompileSettings
    {
        public enum CppVersion
        {
            // Cpp20 is required for most projects
            Minimum,

            // Cpp23 is required for Saturn itself
            Cpp23,

            Cpp26,

            Latest,
        }

        public enum CppOptimisation
        {
            /// <summary>
            /// On MSVC: /Od
            /// </summary>
            Off,

            /// <summary>
            /// ???
            /// </summary>
            Debug,

            /// <summary>
            /// On MSVC: /01
            /// </summary>
            Size,

            /// <summary>
            /// On MSVC: /02 
            /// </summary>
            Speed,

            /// <summary>
            /// On MSVC: /Ox 
            /// </summary>
            Full,
        }

        public enum PrecompiledHeaderAction 
        {
            /// <summary>
            /// Do nothing, no PCH.
            /// </summary>
            NoAction,

            /// <summary>
            /// Create the PCH.
            /// </summary>
            Create,

            /// <summary>
            /// Use the PCH file that was created before.
            /// </summary>
            Use
        }

        public struct FPCHInfo
        {
            public string HeaderFile;
            public string SourceFile;

            public FPCHInfo( string headerPath, string sourcePath )
            {
                HeaderFile = headerPath;
                SourceFile = sourcePath;

                if( Valid() )
                {
                    if( Shared.Platform.PlatformType == PlatformType.Windows )
                    {
                        SourceFile = SourceFile.Replace( "/", "\\" );
                    }

                    SourceFile = Path.Combine( Shared.ProjectInfo.RootDirectory, SourceFile );
                }
            }

            public bool Valid()
            {
                return HeaderFile != null && SourceFile != null;
            }
        }

        public readonly ConfigKind ConfigKind;

        /// <summary>
        /// Visual Studio 2017+ option only, add /JMC
        /// </summary>
        public bool JustMyCodeDebugging = false;

        /// <summary>
        /// Adds __X31_SHOWCONSOLE__ to the Preprocessor Defines
        /// </summary>
        public bool X31ShowConsole = false;

        /// <summary>
        /// Enables /Gw on MSVC
        /// </summary>
        public bool OptimiseGlobalData = false;

        /// <summary>
        /// Enables /Zo, /Od also disables it 
        /// </summary>
        public bool EnableEditAndContinue = false;

        /// <summary>
        /// If true, then /Zc:wchar_t is suggested, else wchar_t becomes a typedef.
        /// </summary>
        public bool WCharIsABuiltInType = true;

        /// <summary>
        /// /experimental flag in MSVC
        /// </summary>
        public bool ExperimentalFeatures = false;

        /// <summary>
        /// Optimisation levels
        /// </summary>
        public CppOptimisation Optimisation = CppOptimisation.Off;

        /// <summary>
        /// Specify C++ Standard version to use
        /// </summary>
        public CppVersion Version = CppVersion.Minimum;

        /// <summary>
        /// The current Precompiled Header (PCH) action that this settings should do.
        /// </summary>
        public PrecompiledHeaderAction PCHAction = PrecompiledHeaderAction.NoAction;

        /// <summary>
        /// The paths to the PCH source and header files.
        /// </summary>
        public FPCHInfo PCHInfo = new FPCHInfo();

        /// <summary>
        /// Preprocessor Defines
        /// </summary>
        public List<string> PreprocessorDefines = new List<string>();

        /// <summary>
        /// The additional include directories
        /// </summary>
        public List<string> Includes = new List<string>();

        /// <summary>
        /// The output path.
        /// Example path may be: C:\Projects\MyProject\bin-int\MyProject\MyModule\MyFile.obj
        /// </summary>
        public string OutputPath = null;

        public CompileSettings() { }

        public CompileSettings(
            ConfigKind configKind,
            bool justMyCodeDebugging,
            bool showConsole,
            bool optimiseGlobalData,
            bool enableEditAndContinue,
            bool wCharIsABuiltInType,
            bool experimentalFeatures,
            CppOptimisation optimisation,
            CppVersion version,
            PrecompiledHeaderAction precompiledHeaderAction,
            FPCHInfo precompiledHeaderInfo,
            List<string> preprocessorDefines,
            List<string> includes,
            string outputPath )
        {
            ConfigKind = configKind;
            JustMyCodeDebugging = justMyCodeDebugging;
            X31ShowConsole = showConsole;
            OptimiseGlobalData = optimiseGlobalData;
            EnableEditAndContinue = enableEditAndContinue;
            WCharIsABuiltInType = wCharIsABuiltInType;
            ExperimentalFeatures = experimentalFeatures;
            Optimisation = optimisation;
            Version = version;
            PCHAction = precompiledHeaderAction;
            PCHInfo = precompiledHeaderInfo;
            PreprocessorDefines = preprocessorDefines;
            Includes = includes;
            OutputPath = outputPath;
        }

        // Copy constructor
        public CompileSettings( CompileSettings other ) 
        {
            ConfigKind            = other.ConfigKind;
            JustMyCodeDebugging   = other.JustMyCodeDebugging;
            X31ShowConsole        = other.X31ShowConsole;
            OptimiseGlobalData    = other.OptimiseGlobalData;
            EnableEditAndContinue = other.EnableEditAndContinue;
            WCharIsABuiltInType   = other.WCharIsABuiltInType;
            ExperimentalFeatures  = other.ExperimentalFeatures;
            Optimisation          = other.Optimisation;
            Version               = other.Version;
            PreprocessorDefines   = other.PreprocessorDefines;
            Includes              = other.Includes;
            OutputPath            = other.OutputPath;
            PCHAction             = other.PCHAction;
            PCHInfo               = other.PCHInfo;
        }
    }
}
