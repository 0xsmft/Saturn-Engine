using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

namespace SaturnBuildTool
{
    internal class CSharpCompiler
    {
        public static Assembly CompileCSharpFiles( string[] paths )
        {
            // Set up CS build system.
            Dictionary<string, string> providerOptions = new Dictionary<string, string>
            {
                { "CompilerVersion", "v4.0" }
            };
            CodeDomProvider codeDomProvider = new Microsoft.CSharp.CSharpCodeProvider( providerOptions );

            Assembly[] defaultReferences = {
                typeof( object ).Assembly,
                typeof( Enumerable ).Assembly,
                typeof( ISet<> ).Assembly,
                typeof( CSharpCompiler ).Assembly,
            };

            HashSet<string> references = new HashSet<string>();
            foreach( var defaultReference in defaultReferences )
                references.Add( defaultReference.Location );

            CompilerParameters cp = new CompilerParameters
            {
                GenerateExecutable = false,
                WarningLevel = 3,
                TreatWarningsAsErrors = false,
                GenerateInMemory = true,
                IncludeDebugInformation = false
            };
            cp.ReferencedAssemblies.AddRange( references.ToArray() );

            CompilerResults result = codeDomProvider.CompileAssemblyFromFile( cp, paths );

            foreach( CompilerError ce in result.Errors )
            {
                Console.WriteLine( $"ERROR: {ce.FileName}({ce.Line},{ce.Column}): Unable to compile C# file: {ce.ErrorText}" );
            }

            return result.Errors.HasErrors ? null : result.CompiledAssembly;
        }
    }
}
