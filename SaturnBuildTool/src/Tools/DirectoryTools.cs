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
