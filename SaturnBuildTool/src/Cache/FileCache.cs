using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Text;

namespace SaturnBuildTool.Cache
{
    public class FileCache
    {
        public class FileCacheTime : IEquatable<FileCacheTime>
        {
            public static readonly FileCacheTime Zero = new FileCacheTime();

            // C# Ticks
            public long Time;

            // For C++ (unix time, system time)
            public long UnixTime;

            public string Hash;

            public List<string> ImmediatesIncludes = new List<string>();

            public static bool operator !=( FileCacheTime t1, FileCacheTime t2 )
            {
                return !t1.Equals( t2 );
            }

            public static bool operator ==( FileCacheTime t1, FileCacheTime t2 )
            {
                return t1.Equals( t2 );
            }

            public static bool operator !=( FileCacheTime t1, DateTime d1 )
            {
                return t1.Time != d1.Ticks;
            }

            public static bool operator ==( FileCacheTime t1, DateTime d1 )
            {
                return t1.Time == d1.Ticks;
            }

            public bool Equals( FileCacheTime other )
            {
                return Time == other.Time;
            }

            public override bool Equals( object obj )
            {
                return obj is FileCacheTime other && Equals( other );
            }

            public override int GetHashCode()
            {
                return Time.GetHashCode();
            }
        }

        public ConcurrentDictionary<string, FileCacheTime> PendingFilesToCache { get; } = new ConcurrentDictionary<string, FileCacheTime>();
        public ConcurrentDictionary<string, FileCacheTime> ResidentFilesInCache { get; } = new ConcurrentDictionary<string, FileCacheTime>();

        private readonly HashSet<string> VisitedFiles = new HashSet<string>( StringComparer.OrdinalIgnoreCase );

        public FileCache()
        {
        }

        public void CacheFile( string filePath )
        {
            if( !IsCppFile( filePath ) )
                return;

            // Prevent processing the same file multiple times.
            if( !VisitedFiles.Add( filePath ) )
                return;

            if( !File.Exists( filePath ) )
                return;

            DateTime lastWriteTime = File.GetLastWriteTime( filePath );
            string hash = ComputeHash( filePath );
            List<string> incs = CppIncludes.ParseImmediateIncludes( filePath );

            var fct = new FileCacheTime
            {
                Time = lastWriteTime.Ticks,
                UnixTime = ( long ) lastWriteTime.Subtract( new DateTime( 1970, 1, 1 ) ).TotalMilliseconds,
                Hash = hash,
                ImmediatesIncludes = incs
            };

            PendingFilesToCache[ filePath ] = fct;

            foreach( var inc in incs )
            {
                // Recursively get includes.
                CacheFile( inc );
            }
        }

        private string ComputeHash( string filepath )
        {
            FileStream stream = File.OpenRead( filepath );
            SHA256 sha = SHA256.Create();
            byte[] hash = sha.ComputeHash( stream );

            return BitConverter.ToString( hash ).Replace( "-", string.Empty );
        }

        public bool IsCppFile( string Filepath )
        {
            string ext = Path.GetExtension( Filepath );
            return ext == ".cpp" || ext == ".h" || ext == ".hpp";
        }

        public bool IsSourceFile( string Filepath )
        {
            return Path.GetExtension( Filepath ) == ".cpp";
        }

        public bool IsFileInCache( string Filepath )
        {
            return PendingFilesToCache.ContainsKey( Filepath );
        }

        public void Clean()
        {
            PendingFilesToCache.Clear();
            ResidentFilesInCache.Clear();
        }

        public bool HasFileBeenModified( string path )
        {
            // return true to force the file to be compiled.
            if( !ResidentFilesInCache.TryGetValue( path, out var cacheTime ) )
                return true;

            DateTime currentWriteTime = File.GetLastWriteTime( path );
            long currentTicks = currentWriteTime.Ticks;

            // Allow for minor timestamp drift, sometimes the times are off ever so slightly but the files haven't changed at all.
            long deltaTicks = Math.Abs( currentTicks - cacheTime.Time );
            const long toleranceTicks = TimeSpan.TicksPerMillisecond;

            if( deltaTicks > toleranceTicks )
            {
                // Timestamp changed enough so try double check with the hash.
                string currentHash = ComputeHash( path );
                if( currentHash != cacheTime.Hash )
                    return true;
            }

            return false;
        }

        public static void RT_WriteCache( FileCache fileCache )
        {
            foreach( KeyValuePair<string, FileCacheTime> kv in fileCache.PendingFilesToCache )
            {
                fileCache.ResidentFilesInCache.AddOrUpdate( kv.Key, kv.Value, ( key, oldValue ) => kv.Value );
            }

            fileCache.PendingFilesToCache.Clear();

            // --- Begin write 
            // We have to write this in a way that when the header tool reads this it can understand it
            // So this has to be C++ compatible.
            // See, FileCache.cpp (SaturnBuildTool)

            FileStream fs = new FileStream( Shared.ProjectInfo.FileCacheLocation, FileMode.Truncate, FileAccess.Write, FileShare.ReadWrite );
            BinaryWriter writer = new BinaryWriter( fs, Encoding.UTF8, false );

            writer.Write( fileCache.ResidentFilesInCache.Count );

            foreach( KeyValuePair<string, FileCacheTime> kv in fileCache.ResidentFilesInCache )
            {
                byte[] strBytes = Encoding.UTF8.GetBytes( kv.Key );
                writer.Write( ( ulong ) kv.Key.Length );
                writer.Write( strBytes );

                var fct = kv.Value;

                // Write Ticks for Build Tool
                writer.Write( fct.Time );

                // Write unix time for Header Tool
                writer.Write( fct.UnixTime );

                writer.Write( fct.Hash ?? string.Empty );

                writer.Write( fct.ImmediatesIncludes.Count );
                foreach( var inc in fct.ImmediatesIncludes )
                {
                    byte[] bytes = Encoding.UTF8.GetBytes( inc );
                    writer.Write( ( ulong ) inc.Length );
                    writer.Write( bytes );
                }
            }

            writer.Close();
            fs.Close();
        }

        public static void RT_WriteCacheHumanReadable( FileCache fileCache )
        {
            string filepath = Shared.ProjectInfo.FileCacheLocation.Replace( ".fc", ".txt" );
            if( !File.Exists( filepath ) )
                File.Create( filepath ).Close();

            // --- Begin write 
            // We have to write this in a way that when the header tool reads this it can understand it
            // So this has to be C++ compatible.
            // See, FileCache.cpp (SaturnBuildTool)

            FileStream fs = new FileStream( filepath, FileMode.Truncate, FileAccess.Write, FileShare.ReadWrite );
            StreamWriter writer = new StreamWriter( fs, Encoding.UTF8 );

            writer.WriteLine( $"FilesInCache.Count {fileCache.ResidentFilesInCache.Count}" );

            writer.WriteLine( "{" );
            foreach( KeyValuePair<string, FileCacheTime> kv in fileCache.ResidentFilesInCache )
            {
                writer.WriteLine( string.Format( "\t{0}:", kv.Key ) );

                // Write Ticks for Build Tool
                writer.WriteLine( string.Format( "\t\tC# Ticks: {0}", kv.Value.Time ) );

                // Write unix time for Header Tool
                writer.WriteLine( string.Format( "\t\tUnix Timestamp: {0}", kv.Value.UnixTime ) );

                writer.WriteLine( string.Format( "\t\tHash: {0}", kv.Value.Hash ) );

                foreach( string inc in kv.Value.ImmediatesIncludes )
                {
                    writer.WriteLine( string.Format( "\t\tINC: {0}", inc ) );
                }
            }
            writer.WriteLine( "}" );

            writer.WriteLine( "[END OF EXPORTED FILECACHE]" );

            writer.Close();
            fs.Close();
        }

        public static FileCache Load( string cachePath = null )
        {
            string FileCachePath = cachePath ?? Shared.ProjectInfo.FileCacheLocation;

            FileCache fc = new FileCache();

            FileStream fs;
            if( !File.Exists( FileCachePath ) )
            {
                fs = File.Create( FileCachePath );
            }
            else
            {
                try
                {
                    fs = new FileStream( FileCachePath, FileMode.Open );
                }
                catch( Exception e )
                {
                    Console.WriteLine( "Error when trying open FileCache: {0}", e.Message );
                    Console.WriteLine( "Filecache is empty, creating new cache..." );

                    return fc;
                }
            }

            if( fs.Length == 0 )
            {
                Console.WriteLine( "Filecache is empty, creating new cache..." );

                fs.Close();
                return fc;
            }

            // --- Begin read
            // We have to read this in a way that when the header tool reads this it can understand it
            // So this has to be C++ compatible.
            // See, FileCache.cpp (SaturnBuildTool)

            BinaryReader reader = new BinaryReader( fs, Encoding.UTF8 );

            int count = reader.ReadInt32();

            for( int i = 0; i < count; i++ )
            {
                ulong length = reader.ReadUInt64();
                string key = Encoding.UTF8.GetString( reader.ReadBytes( ( int ) length ) );

                long ticks = reader.ReadInt64();
                long unixTime = reader.ReadInt64();
                string hash = reader.ReadString();

                int includeCount = reader.ReadInt32();

                var includeList = new List<string>();
                for( int j = 0; j < includeCount; j++ )
                {
                    ulong len = reader.ReadUInt64();
                    includeList.Add( Encoding.UTF8.GetString( reader.ReadBytes( ( int ) len ) ) );
                }

                FileCacheTime time = new FileCacheTime
                {
                    UnixTime = unixTime,
                    Time = ticks,
                    Hash = hash,
                    ImmediatesIncludes = includeList
                };

                fc.ResidentFilesInCache.TryAdd( key, time );
            }

            reader.Close();
            fs.Close();

            return fc;
        }

        public List<string> Analyse( List<string> allKnownSrcFiles )
        {
            List<string> result = new List<string>();

            // Search the source dir for any new/removed files from the last build
            foreach( string file in allKnownSrcFiles )
            {
                if( ResidentFilesInCache.ContainsKey( file ) )
                {
                    if( HasFileBeenModified( file ) )
                    {
                        Console.WriteLine( $"{file} has been modified!" );

                        Shared.TaskCache.RemoveTask( file );
                        result.Add( file );
                    }
                }
                else
                {
                    Console.WriteLine( $"{file} is not in the File Cache!" );
                    Shared.TaskCache.RemoveTask( file );

                    CacheFile( file );
                    result.Add( file );
                }
            }

            return result;
        }
    }

}
