using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

namespace SaturnBuildTool
{
    public static class CppIncludes
    {
        private static readonly string INCLUDES_TEXT = "include";

        private static string ResolveInc( string unresolvedInc )
        {
            foreach( var dir in Shared.TargetToBuild.Includes )
            {
                string candidate = Path.Combine( dir, unresolvedInc );
                if( File.Exists( candidate ) )
                {
                    return Path.GetFullPath( candidate );
                }
            }

            return unresolvedInc;
        }

        public static List<string> ParseAllIncludes( string filePath )
        {
            var result = new List<string>();
            var visited = new HashSet<string>( StringComparer.OrdinalIgnoreCase );

            void Recurse( string path )
            {
                if( !visited.Add( path ) )
                    return;

                foreach( var inc in ParseImmediateIncludes( path ) )
                {
                    if( inc != null )
                    {
                        result.Add( inc );
                        Recurse( inc );
                    }
                }
            }

            Recurse( filePath );
            return result;
        }

        public static List<string> ParseImmediateIncludes( string filePath )
        {
            List<string> includes = new List<string>();
            HashSet<string> seenIncludes = new HashSet<string>( StringComparer.OrdinalIgnoreCase );

            string fileText = File.ReadAllText( filePath );
            int lineCount = fileText.Length;

            for( int i = 0; i < lineCount; i++ )
            {
                // Skip single-line comments
                if( i < lineCount - 1 && fileText[ i ] == '/' && fileText[ i + 1 ] == '/' )
                {
                    i += 2;
                    while( i < lineCount && fileText[ i ] != '\n' )
                        i++;
                    continue;
                }

                // Skip multi-line comments
                if( i < lineCount - 1 && fileText[ i ] == '/' && fileText[ i + 1 ] == '*' )
                {
                    i += 2;
                    while( i < lineCount - 1 && !( fileText[ i ] == '*' && fileText[ i + 1 ] == '/' ) )
                        i++;
                    i += 2; // Skip '*/'
                    continue;
                }

                // Look for preprocessor directive
                if( fileText[ i ] != '#' )
                    continue;

                // Read all until preprocessor instruction begin
                if( fileText[ i ] != '#' )
                    continue;

                // Skip spaces and tabs
                while( ++i < lineCount && ( fileText[ i ] == ' ' || fileText[ i ] == '\t' ) )
                {
                }

                // Skip anything other than 'include' text
                if( i + INCLUDES_TEXT.Length >= lineCount )
                    break;
                var token = fileText.Substring( i, INCLUDES_TEXT.Length );
                if( token != INCLUDES_TEXT )
                    continue;
                i += INCLUDES_TEXT.Length;

                // Skip all before path start
                while( ++i < lineCount && fileText[ i ] != '\n' && fileText[ i ] != '"' && fileText[ i ] != '<' )
                {
                }

                // Skip all until path end
                var includeStart = i;
                while( ++i < lineCount && fileText[ i ] != '\n' && fileText[ i ] != '"' && fileText[ i ] != '>' )
                {
                }

                // Extract included file path
                var includedFile = fileText.Substring( includeStart, i - includeStart );
                includedFile = includedFile.Trim();
                if( includedFile.Length == 0 )
                    continue;

                includedFile = includedFile.Substring( 1, includedFile.Length - 1 );

                string fullPath = ResolveInc( includedFile );

                if( fullPath == null )
                {
                    Debugger.Break();
                    continue;
                }

                if( seenIncludes.Add( fullPath ) )
                {
                    includes.Add( fullPath );
                }
            }

            return includes;
        }
    }
}
