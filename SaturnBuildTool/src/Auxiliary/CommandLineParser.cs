using System;
using System.Collections.Generic;
using System.Diagnostics.SymbolStore;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Web;

namespace SaturnBuildTool.Auxiliary
{
    internal class CommandLineParser
    {
        public static readonly CommandLineParser Instance = new CommandLineParser();

        private Dictionary<string, string> ParsedMap = new Dictionary<string, string>();

        public CommandLineParser()
        {
        }

        public bool Parse( List<string> args )
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

            return CheckForEssentialArgs();
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

        // Essential args are:
        // The Action, BUILD, REBULD, CLEAN
        // The project name (/NAME:)
        // The target platform, /Win64
        // The configuration, /Debug, /Release, /Dist
        // The project location (/PROJECT:)
        private bool CheckForEssentialArgs()
        {
            bool result = false;

            foreach( KeyValuePair<string, string> kv in ParsedMap )
            {
                if( kv.Value == "NAME" && kv.Value != null )
                {
                    result = true;
                    continue;
                }
                else if( kv.Value == "WIN64" )
                {
                    result = true;
                    continue;
                }
                else if( kv.Value == "DEBUG" || kv.Value == "RELEASE" || kv.Value == "DIST" )
                {
                    result = true;
                    continue;
                }
                else if( kv.Value == "PROJECT" && kv.Value != null )
                {
                    result = true;
                    continue;
                }
            }

            return result;
        }
    }
}