#if SAT_BUILDTOOOL_NETFRAMEWORK
using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.Linq;
#endif

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;

namespace SaturnBuildTool
{
    internal class CSharpCompiler
    {
        public static Assembly CompileCSharpFiles( string[] paths )
        {
#if SAT_BUILDTOOOL_NETFRAMEWORK
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
#else
            var syntaxTree = paths.Select( file => CSharpSyntaxTree.ParseText( File.ReadAllText( file ), path: file ) ).ToArray();

            MetadataReference[] defaultReferences = {
                MetadataReference.CreateFromFile( typeof( object ).Assembly.Location ),
                MetadataReference.CreateFromFile( Assembly.Load( "System.Runtime" ).Location ),
                MetadataReference.CreateFromFile( Assembly.Load( "System.Collections" ).Location ),
                MetadataReference.CreateFromFile( typeof( Enumerable ).Assembly.Location ),
                MetadataReference.CreateFromFile( typeof( ISet<> ).Assembly.Location ),
                MetadataReference.CreateFromFile( typeof( CSharpCompiler ).Assembly.Location ),
            };

            var comp = CSharpCompilation.Create( assemblyName: Path.GetRandomFileName(), syntaxTrees: syntaxTree, references: defaultReferences, options: new CSharpCompilationOptions( OutputKind.DynamicallyLinkedLibrary ) );

            using var stream = new MemoryStream();
            var result = comp.Emit( stream );

            if( !result.Success )
            {
                foreach( var diagnostic in result.Diagnostics.Where( d => d.Severity == DiagnosticSeverity.Error ) )
                {
                    Console.WriteLine( $"ERROR: {diagnostic}" );
                }

                return null;
            }

            stream.Seek( 0, SeekOrigin.Begin );

            var asm = Assembly.Load( stream.ToArray() );
            return asm;
#endif
        }
    }
}
