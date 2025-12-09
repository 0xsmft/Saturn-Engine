using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace SaturnBuildTool.Cache
{
    public class TaskCache
    {
        /// <summary>
        /// Map of an input file to an output file
        /// </summary>
        private ConcurrentDictionary<string, string> ProducedItems { get; set; } = new ConcurrentDictionary<string, string>();

        public TaskCache() { }

        public void CacheTask( string input, string output )
        {
            ProducedItems.TryAdd( input, output );
        }

        public void RemoveTask( string input )
        {
            ProducedItems.TryRemove( input, out var _ );
        }

        public bool TaskOutputExists( string file )
        {
            return ProducedItems.ContainsKey( file ) && File.Exists( file );
        }

        public bool LnkTaskOutputExists( string file )
        {
            return ProducedItems.ContainsKey( file );
        }

        public bool LnkFinalOutputExists( string outFile ) 
        {
            var itemKV = ProducedItems.FirstOrDefault( kv => string.Equals( kv.Value, outFile ) );

            if( !string.IsNullOrEmpty( itemKV.Key ) ) 
            {
                return File.Exists( itemKV.Value );
            }
            else 
            { 
                return false; 
            }
        }

        public List<string> GetOutputItemFromSource( string outPath ) 
        {
            List<string> outputItems = new List<string>();  
            foreach( var kv in ProducedItems )
            {
                var parentPath = Directory.GetParent( kv.Value );
                if( parentPath.FullName == outPath )
                {
                    outputItems.Add( kv.Value );
                }

                /*
                if( File.Exists( fullPath ) )
                {
                    outputItems.Add( fullPath );
                }
                */
            }

            return outputItems;
        }

        public void RT_WriteCache()
        {
            string path = Path.Combine( Shared.ProjectInfo.BuildDir, $"TaskCache-{Shared.ProjectInfo.CurrentConfigKind}.fc" );

            FileStream fs = new FileStream( path, FileMode.Truncate, FileAccess.Write, FileShare.ReadWrite );
            BinaryWriter writer = new BinaryWriter( fs, Encoding.UTF8, false );

            writer.Write( ProducedItems.Count );
            foreach( KeyValuePair<string, string> kv in ProducedItems )
            {
                byte[] strBuffer = Encoding.UTF8.GetBytes( kv.Key );
                writer.Write( ( ulong ) kv.Key.Length );
                writer.Write( strBuffer );

                strBuffer = Encoding.UTF8.GetBytes( kv.Value );
                writer.Write( ( ulong ) kv.Value.Length );
                writer.Write( strBuffer );
            }

            writer.Close();
            fs.Close();
        }

        public static TaskCache Load()
        {
            string path = Path.Combine( Shared.ProjectInfo.BuildDir, $"TaskCache-{Shared.ProjectInfo.CurrentConfigKind}.fc" );

            TaskCache tc = new TaskCache();

            FileStream fs;
            if( !File.Exists( path ) )
            {
                fs = File.Create( path );
            }
            else
            {
                try
                {
                    fs = new FileStream( path, FileMode.Open );
                }
                catch( Exception e )
                {
                    Console.WriteLine( "Error when trying open TaskCache: {0}", e.Message );
                    Console.WriteLine( "TaskCache is empty, creating new cache..." );

                    return tc;
                }
            }

            if( fs.Length == 0 )
            {
                Console.WriteLine( "TaskCache is empty, creating new cache..." );

                fs.Close();
                return tc;
            }

            // --- Begin read
            BinaryReader reader = new BinaryReader( fs, Encoding.UTF8 );

            int count = reader.ReadInt32();
            for( int i = 0; i < count; i++ )
            {
                ulong length = reader.ReadUInt64();
                string key = Encoding.UTF8.GetString( reader.ReadBytes( ( int ) length ) );

                length = reader.ReadUInt64();
                string value = Encoding.UTF8.GetString( reader.ReadBytes( ( int ) length ) );

                tc.ProducedItems.TryAdd( key, value );
            }

            reader.Close();
            fs.Close();

            return tc;
        }
    }
}
