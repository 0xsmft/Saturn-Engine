using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace SaturnBuildTool.Tools
{
    internal static class DirectoryTools
    {
        public static List<string> DirSearch( string sDir )
        {
            List<string> strings = new List<string>();

            try
            {
                foreach( string f in Directory.GetFiles( sDir ) )
                {
                    strings.Add( f );
                }

                foreach( string d in Directory.GetDirectories( sDir ) )
                {
                    strings.AddRange( DirSearch( d ) );
                }
            }
            catch( System.Exception excpt )
            {
                Console.WriteLine( excpt.Message );
            }

            return strings;
        }

        public static List<string> DirSearch( string sDir, string ext )
        {
            List<string> strings = new List<string>();

            try
            {
                foreach( string f in Directory.GetFiles( sDir ) )
                {
                    if( Path.GetExtension( f ) != ext )
                        continue;

                    strings.Add( f );
                }
            }
            catch( System.Exception excpt )
            {
                Console.WriteLine( excpt.Message );
            }

            return strings;
        }

        public static List<string> DirSearch( string sDir, List<string> exts )
        {
            List<string> strings = new List<string>();

            try
            {
                foreach( string f in Directory.GetFiles( sDir ) )
                {
                    if( exts.Contains( Path.GetExtension( f ) ) )
                    {
                        strings.Add( f );
                    }
                }
            }
            catch( System.Exception excpt )
            {
                Console.WriteLine( excpt.Message );
            }

            return strings;
        }

        public static List<string> CppSourceSearch( string sDir, bool isSourceOnly )
        {
            List<string> files = new List<string>();

            try
            {
                // Process current directory's files.
                foreach( string f in Directory.GetFiles( sDir ) )
                {
                    if( IsCppSourceFile( f ) )
                    {
                        files.Add( f );
                    }
                }

                // Recurse into subdirectories.
                foreach( string dir in Directory.GetDirectories( sDir ) )
                {
                    string dirName = Path.GetFileName( dir );
                    files.AddRange( CppSourceSearch( dir, isSourceOnly ) );
                }
            }
            catch( Exception ex )
            {
                Console.WriteLine( $"Error reading directory '{sDir}': {ex.Message}" );
            }

            return files;
        }

        [Flags]
        public enum CSharpSearchOptions
        {
            Targets = 0x1,
            Modules = 0x2,
            All = Targets | Modules
        }

        public static List<string> CsSourceSearch( string sDir, CSharpSearchOptions options )
        {
            List<string> files = new List<string>();

            try
            {
                // Process current directory's files.
                foreach( string f in Directory.GetFiles( sDir ) )
                {
                    string fileName = Path.GetFileName( f ).ToLowerInvariant();

                    if( ( options & CSharpSearchOptions.Targets ) != 0 )
                    {
                        string[] patten = new string[] { ".development.cs", ".dist.cs" };

                        if( patten.Any( p => fileName.EndsWith( p, StringComparison.InvariantCultureIgnoreCase ) ) )
                        {
                            files.Add( f );
                        }
                    }

                    if( ( options & CSharpSearchOptions.Modules ) != 0 )
                    {
                        string[] patten = new string[] { ".module.cs" };

                        if( patten.Any( p => fileName.EndsWith( p, StringComparison.InvariantCultureIgnoreCase ) ) )
                        {
                            files.Add( f );
                        }
                    }
                    else
                    {
                        string[] patten = new string[] { ".development.cs", ".dist.cs", ".module.cs" };

                        if( patten.Any( p => fileName.EndsWith( p, StringComparison.InvariantCultureIgnoreCase ) ) )
                        {
                            files.Add( f );
                        }
                    }
                }

                // Recurse into subdirectories.
                foreach( string dir in Directory.GetDirectories( sDir ) )
                {
                    string dirName = Path.GetFileName( dir );
                    files.AddRange( CsSourceSearch( dir, options ) );
                }
            }
            catch( Exception ex )
            {
                Console.WriteLine( $"Error reading directory '{sDir}': {ex.Message}" );
            }

            return files;
        }

        private static bool IsCppSourceFile( string path )
        {
            string ext = Path.GetExtension( path ).ToLowerInvariant();
            return ext == ".cpp" || ext == ".cc" || ext == ".c";
        }

        public static List<string> DirSearch( string sDir, bool isSourceOnly )
        {
            List<string> strings = new List<string>();

            try
            {
                foreach( string d in Directory.GetDirectories( sDir ) )
                {
                    if( isSourceOnly )
                    {
                        if( !d.EndsWith( "src" ) || d.EndsWith( "Source" ) )
                        {
                            continue;
                        }
                    }

                    foreach( string f in Directory.GetFiles( d ) )
                    {
                        strings.Add( f );
                    }

                    strings.AddRange( DirSearch( d, isSourceOnly ) );
                }

                foreach( string f in Directory.GetFiles( sDir ) )
                {
                    strings.Add( f );
                }
            }
            catch( System.Exception excpt )
            {
                Console.WriteLine( excpt.Message );
            }

            return strings;
        }
    }
}
