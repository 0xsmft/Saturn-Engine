using System;
using System.Collections.Generic;
using System.Globalization;

namespace SaturnBuildTool.Auxiliary
{
    internal class CommandLineParser
    {
        public static readonly CommandLineParser Instance = new CommandLineParser();

        private readonly Dictionary<string, string> ParsedMap = new Dictionary<string, string>();

        public void Parse( List<string> args )
        {
            CultureInfo culture = CultureInfo.InvariantCulture;
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
                        string key = arg.Substring( 1, position - 1 ).ToUpper( culture );
                        string value = arg.Substring( position + 1 );

                        ParsedMap[ key ] = value;
                    }
                    else
                    {
                        ParsedMap[ arg.Substring( 1 ).ToUpper( culture ) ] = "true";
                    }
                }
            }
        }

        public bool FindFlag( string key )
        {
            return ParsedMap.ContainsKey( key.ToUpper( CultureInfo.InvariantCulture ) );
        }

        public string FindValueFromKey( string key )
        {
            if( ParsedMap.ContainsKey( key.ToUpper( CultureInfo.InvariantCulture ) ) )
            {
                return ParsedMap[ key.ToUpper( CultureInfo.InvariantCulture ) ];
            }

            return null;
        }

        public bool HasArgument( string key )
        {
            return ParsedMap.ContainsKey( key.ToUpper( CultureInfo.InvariantCulture ) );
        }

        public int GetComamndCount()
        {
            return ParsedMap.Count;
        }

        public void PrintAllArgs()
        {
            Console.WriteLine( "==== Using arguments: ====" );

            foreach( KeyValuePair<string, string> arg in ParsedMap )
            {
                Console.WriteLine( $" {arg.Key} : {arg.Value}" );
            }

            Console.WriteLine( "==== [END OF ARGUMEMTS] ====" );
        }

        public string FindPlatformCmd()
        {
            if( HasArgument( "WIN64" ) )
            {
                return "WIN64";
            }
            else if( HasArgument( "LINUX64" ) )
            {
                return "LINUX64";
            }
            else if( HasArgument( "APPLE" ) )
            {
                return "APPLE";
            }

            return null;
        }

    }
}
