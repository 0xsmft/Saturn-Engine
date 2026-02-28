using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace SaturnBuildTool.Cache
{
    public class LinkCache
    {
        // Filepath -> C# Time Ticks
        public ConcurrentDictionary<string, long> PendingFilesToCache;

        // Filepath -> C# Time Ticks
        public ConcurrentDictionary<string, long> ResidentFiles;

        public LinkCache()
        {
            PendingFilesToCache = new ConcurrentDictionary<string, long>();
            ResidentFiles = new ConcurrentDictionary<string, long>();
        }

        private bool IsCppFile( string Filepath )
        {
            string ext = Path.GetExtension( Filepath );
            return ext == ".cpp" || ext == ".h" || ext == ".hpp";
        }

        public void CacheFile( string filePath ) 
        {
            if( IsCppFile( filePath ) ) return;

            DateTime lastWriteTime = File.GetLastWriteTime( filePath );

            PendingFilesToCache[ filePath ] = lastWriteTime.Ticks;
        }

        public void Clean() 
        {
            PendingFilesToCache.Clear();
            ResidentFiles.Clear();
        }

        public bool HasFileBeenModified( string path ) 
        {
            if( !ResidentFiles.TryGetValue( path, out var timeInTicks ) )
                return true;

            DateTime currentWriteTime = File.GetLastWriteTime( path );
            long currentTicks = currentWriteTime.Ticks;

            long deltaTicks = Math.Abs( currentTicks - timeInTicks );
            const long toleranceTicks = TimeSpan.TicksPerMillisecond;

            if( deltaTicks > toleranceTicks ) 
            {
                return true;
            }

            return false;
        }
        
        public void RT_WriteCache()
        {
            foreach( KeyValuePair<string, long> kv in PendingFilesToCache )
            {
                ResidentFiles.AddOrUpdate( kv.Key, kv.Value, ( key, oldValue ) => kv.Value );
            }

            PendingFilesToCache.Clear();

            // --- Begin write
            FileStream fs = new FileStream( Shared.ProjectInfo.LinkCacheLocation, FileMode.Truncate, FileAccess.Write, FileShare.ReadWrite );
            BinaryWriter writer = new BinaryWriter( fs, Encoding.UTF8, false );

            writer.Write( ResidentFiles.Count );

            foreach( KeyValuePair<string, long> kv in ResidentFiles )
            {
                byte[] strBytes = Encoding.UTF8.GetBytes( kv.Key );
                writer.Write( ( ulong ) kv.Key.Length );
                writer.Write( strBytes );

                // Write Ticks for Build Tool
                writer.Write( kv.Value );
            }

            writer.Close();
            fs.Close();
        }

        public static LinkCache Load( string cachePath = null )
        {
            string LinkCachePath = cachePath ?? Shared.ProjectInfo.LinkCacheLocation;

            LinkCache fc = new LinkCache();

            FileStream fs;
            if( !File.Exists( LinkCachePath ) )
            {
                fs = File.Create( LinkCachePath );
            }
            else
            {
                try
                {
                    fs = new FileStream( LinkCachePath, FileMode.Open );
                }
                catch( Exception e )
                {
                    Console.WriteLine( "Error when trying open LinkCache: {0}", e.Message );
                    Console.WriteLine( "LinkCache is empty, creating new cache..." );

                    return fc;
                }
            }

            if( fs.Length == 0 )
            {
                Console.WriteLine( "LinkCache is empty, creating new cache..." );

                fs.Close();
                return fc;
            }

            // --- Begin read
            BinaryReader reader = new BinaryReader( fs, Encoding.UTF8 );

            int count = reader.ReadInt32();

            for( int i = 0; i < count; i++ )
            {
                ulong length = reader.ReadUInt64();
                string key = Encoding.UTF8.GetString( reader.ReadBytes( ( int ) length ) );

                long ticks = reader.ReadInt64();
                fc.ResidentFiles.TryAdd( key, ticks );
            }

            reader.Close();
            fs.Close();

            return fc;
        }
    }
}
