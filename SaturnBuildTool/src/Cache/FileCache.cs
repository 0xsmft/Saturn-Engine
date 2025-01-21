using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using SaturnBuildTool.Auxiliary;

namespace SaturnBuildTool.Cache
{
    internal class FileCache
    {
        public struct FileCacheTime
        {
            // C# Ticks
            public long Time;

            // For C++ (unix time, system time)
            public long UnixTime;
 
            public static bool operator !=(FileCacheTime t1, FileCacheTime t2)
            {
                return t1.Time != t2.Time;
            }

            public static bool operator ==(FileCacheTime t1, FileCacheTime t2)
            {
                return t1.Time == t2.Time;
            }

            public static bool operator !=(FileCacheTime t1, DateTime d1)
            {
                return t1.Time != d1.Ticks;
            }

            public static bool operator ==(FileCacheTime t1, DateTime d1)
            {
                return t1.Time == d1.Ticks;
            }

            public override bool Equals( object obj )
            {
                if( obj is FileCacheTime other ) 
                {
                    return this.Time == other.Time;
                }
                return false;
            }

            public override int GetHashCode()
            {
                return Time.GetHashCode();
            }
        }

        public IDictionary<string, FileCacheTime> FilesToCache { get; }
        public IDictionary<string, FileCacheTime> FilesInCache { get; }

        private string Filepath;

        public FileCache(string CacheLocation)
        {
            FilesToCache = new Dictionary<string, FileCacheTime>();
            FilesInCache = new Dictionary<string, FileCacheTime>();

            Filepath = CacheLocation;
        }

        public FileCache()
        {
            FilesToCache = new Dictionary<string, FileCacheTime>();
            FilesInCache = new Dictionary<string, FileCacheTime>();

            Filepath = "";
        }

        public void CacheFile(string Filepath)
        {
            if (IsCppFile(Filepath)) 
            {
                DateTime lastWriteTime = File.GetLastWriteTime(Filepath);

                FileCacheTime fct = new FileCacheTime();
                fct.Time = lastWriteTime.Ticks;
                fct.UnixTime = (long)lastWriteTime.Subtract(new DateTime(1970, 1, 1)).TotalMilliseconds;

                FilesToCache.Add(Filepath, fct);
            }
        }

        public bool IsCppFile( string Filepath )
        {
            return Path.GetExtension(Filepath) == ".cpp" || Path.GetExtension(Filepath) == ".h" || Path.GetExtension(Filepath) == ".hpp";
        }

        public bool IsSourceFile(string Filepath)
        {
            return Path.GetExtension(Filepath) == ".cpp";
        }

        public bool IsFileInCache(string Filepath) 
        {
            return FilesToCache.ContainsKey(Filepath);
        }

        public void Clean() 
        {
            FilesToCache.Clear();
            FilesInCache.Clear();
        }

        public bool HasSourceFileBeenModified( string path, bool includeHeaderFile = false ) 
        {
            // Get cached value
            FilesInCache.TryGetValue(path, out FileCacheTime sourceLastTime);
            DateTime fsLastWriteTime = File.GetLastWriteTime(path);

            bool sourceModifed = ( sourceLastTime != fsLastWriteTime );

            bool headerModifed = false;
            if ( includeHeaderFile && Path.GetExtension( path ) != ".h" ) 
            {
                string headerPath = Path.ChangeExtension(path, ".h");
                if (FilesInCache.TryGetValue(headerPath, out FileCacheTime headerLastTime)) 
                {
                    DateTime headerFsLastWriteTime = File.GetLastWriteTime(headerPath);
                    headerModifed = (headerLastTime != headerFsLastWriteTime);
                }
            }

            return sourceModifed || headerModifed;
        }

        public static void RT_WriteCache( FileCache fileCache ) 
        {
            foreach (KeyValuePair<string, FileCacheTime> kv in fileCache.FilesToCache) 
            {
                FileCacheTime time;
                fileCache.FilesInCache.TryGetValue(kv.Key, out time);

                // Has the file been updated?
                if(time != kv.Value)
                {
                    // Yes, lets try to add it in the cache
                    if(fileCache.FilesInCache.ContainsKey(kv.Key))
                    {
                        if (File.Exists(kv.Key)) 
                        {
                            fileCache.FilesInCache[kv.Key] = kv.Value;
                        }
                    }
                    else
                    {
                        fileCache.FilesInCache.Add(kv);
                    }
                }
            }
            
            fileCache.FilesToCache.Clear();

            // HACK: Clear the file.
            File.WriteAllText(fileCache.Filepath, string.Empty);

            // --- Begin write 
            // We have to write this in a way that when the header tool reads this it can understand it
            // So this has to be C++ compatible.
            // See, FileCache.cpp (SaturnBuildTool)

            FileStream fs = new FileStream(fileCache.Filepath, FileMode.Truncate, FileAccess.Write, FileShare.ReadWrite);
            BinaryWriter writer = new BinaryWriter(fs, Encoding.UTF8, false);
            
            writer.Write( fileCache.FilesInCache.Count );

            foreach(KeyValuePair<string, FileCacheTime> kv in fileCache.FilesInCache)
            {
                writer.Write( (ulong)kv.Key.Length );
                writer.Write( Encoding.UTF8.GetBytes( kv.Key ) );

                // Write Ticks for us
                writer.Write(kv.Value.Time);

                // Write unix time for Header Tool
                writer.Write(kv.Value.UnixTime);
            }

            writer.Close();
            fs.Close();
        }

        public static FileCache Load()
        {
            string FileCachePath = ProjectInfo.Instance.FileCacheLocation;

            FileCache fc = new FileCache(FileCachePath);
            fc.Filepath = FileCachePath;

            FileStream fs;
            if (!File.Exists(FileCachePath))
            {
                fs = File.Create(FileCachePath);
            }
            else
            {
                try
                {
                    fs = new FileStream(FileCachePath, FileMode.Open);
                }
                catch (Exception e)
                {
                    Console.WriteLine("Error when trying open filecache: {0}", e.Message);
                    Console.WriteLine("Filecache is empty, creating new cache...");

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

            BinaryReader reader = new BinaryReader(fs, Encoding.UTF8);

            int count = reader.ReadInt32();

            for (int i = 0; i < count; i++)
            {
                ulong length = reader.ReadUInt64();
                string key = Encoding.UTF8.GetString(reader.ReadBytes((int)length));

                long ticks = reader.ReadInt64();
                long unixTime = reader.ReadInt64();

                FileCacheTime time = new FileCacheTime();
                time.UnixTime = unixTime;
                time.Time = ticks;

                fc.FilesInCache.Add(key, time);
            }

            reader.Close();
            fs.Close();

            return fc;
        }
    }
}
