using System;
using System.IO;
using System.Collections.Generic;

namespace SaturnBuildTool.Tools
{
    internal static class DirectoryTools
    {
        public static List<string> DirSearch(string sDir)
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

        public static List<string> DirSearch(string sDir, string ext)
        {
            List<string> strings = new List<string>();

            try
            {
                foreach (string f in Directory.GetFiles(sDir))
                {
                    if (Path.GetExtension(f) != ext)
                        continue;

                    strings.Add(f);
                }
            }
            catch (System.Exception excpt)
            {
                Console.WriteLine(excpt.Message);
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

        public static List<string> SourceSearch( string sDir, bool isSourceOnly )
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

                    if( isSourceOnly && !( dirName.Equals( "src", StringComparison.OrdinalIgnoreCase ) || dirName.Equals( "source", StringComparison.OrdinalIgnoreCase ) ) )
                    {
                        continue;
                    }

                    files.AddRange( DirSearch( dir, isSourceOnly ) );
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

        public static List<string> DirSearch(string sDir, bool isSourceOnly)
        {
            List<string> strings = new List<string>();

            try
            {
                foreach (string d in Directory.GetDirectories(sDir))
                {
                    if (isSourceOnly) 
                    {
                        if (!d.EndsWith("src") || d.EndsWith("Source")) 
                        {
                            continue;
                        }
                    }

                    foreach (string f in Directory.GetFiles(d))
                    {
                        strings.Add(f);
                    }

                    strings.AddRange( DirSearch(d, isSourceOnly) );
                }

                foreach (string f in Directory.GetFiles(sDir))
                {
                    strings.Add(f);
                }
            }
            catch (System.Exception excpt)
            {
                Console.WriteLine(excpt.Message);
            }

            return strings;
        }
    }
}
