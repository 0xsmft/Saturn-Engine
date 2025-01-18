using System;
using System.Collections.Generic;

namespace SaturnBuildTool.Auxiliary
{
    internal class CommandLineParser
    {
        public static readonly CommandLineParser Instance = new CommandLineParser();

        private readonly Dictionary<string, string> ParsedMap = new Dictionary<string, string>();
        
        public void Parse( List<string> args )
        {
            for( int i = 0; i < args.Count; i++ )
            {
                string arg = args[ i ];

                if( arg.StartsWith( "/" ) )
                {
                    // Check for colon
                    Int32 position = arg.IndexOf( ':' );

                    // Check if arg has a key or is it a flag
                    // Substring( 1 ) to remove "/"
                    if( position > 0 )
                    {
                        string key = arg.Substring( 1, position - 1 ).ToUpper();
                        string value = arg.Substring( position + 1 );

                        ParsedMap[ key ] = value;
                    }
                    else
                    {
                        ParsedMap[ arg.Substring( 1 ) ] = "true";
                    }
                }
            }
        }

        public bool FindFlag( string key ) 
        {
            return ParsedMap.ContainsKey( key );
        }

        public string FindValueFromKey( string key ) 
        {
            if( ParsedMap.ContainsKey( key ) ) 
            {
                return ParsedMap[ key ];
            }

            return null;
        }

        public bool HasArgument( string key ) 
        {
            return ParsedMap.ContainsKey( key );
        }

        public int GetComamndCount() 
        {
            return ParsedMap.Count;
        }
    }
}