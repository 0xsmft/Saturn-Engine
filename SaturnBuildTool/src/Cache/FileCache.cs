using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace SaturnBuildTool.Cache
{
    internal class FileCache
    {
        public IDictionary<string, DateTime> FilesToCache { get; set; }
        public IDictionary<string, DateTime> FilesInCache { get; set; }

        private string Filepath;

        public FileCache(string CacheLocation)
        {
            FilesToCache = new Dictionary<string, DateTime>();
            FilesInCache = new Dictionary<string, DateTime>();

            Filepath = CacheLocation;
        }

        public FileCache()
        {
            FilesToCache = new Dictionary<string, DateTime>();
            FilesInCache = new Dictionary<string, DateTime>();

            Filepath = "";
        }

        public void CacheFile(string Filepath)
        {
            if (IsCppFile(Filepath)) 
            {
                FilesToCache.Add(Filepath, File.GetLastWriteTime(Filepath));
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
            string headerPath = string.Empty;
            bool headerModifed = false;

            if (includeHeaderFile) 
            {
                headerPath = Path.ChangeExtension(path, ".h");
                FilesInCache.TryGetValue(headerPath, out DateTime HeaderLastTime);
                headerModifed = HeaderLastTime != File.GetLastWriteTime(headerPath);
            }

            FilesInCache.TryGetValue(path, out DateTime SourceLastTime);

            bool sourceModifed = SourceLastTime != File.GetLastWriteTime( path );

            return sourceModifed || headerModifed;
        }

        public static void RT_WriteCache( FileCache fileCache ) 
        {
            foreach (KeyValuePair<string, DateTime> kv in fileCache.FilesToCache) 
            {
                DateTime time;
                fileCache.FilesInCache.TryGetValue(kv.Key, out time);

                // Has the file been updated?
                if(time != kv.Value)
                {
                    // Yes, lets try to add it in the cache
                    if(fileCache.FilesInCache.ContainsKey(kv.Key))
                    {
                        fileCache.FilesInCache[kv.Key] = kv.Value;
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

            FileStream fs = new FileStream(fileCache.Filepath, FileMode.Truncate, FileAccess.Write, FileShare.ReadWrite);

            var writer = new BinaryWriter(fs, Encoding.UTF8, false);
            
            writer.Write( fileCache.FilesInCache.Count );

            foreach(KeyValuePair<string, DateTime> kv in fileCache.FilesInCache)
            {
                writer.Write( (ulong)kv.Key.Length );
                writer.Write( Encoding.UTF8.GetBytes( kv.Key ) );

                Int64 unixTime = (Int64)kv.Value.Subtract(new DateTime(1970, 1, 1)).TotalSeconds;
                writer.Write(unixTime);
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

            BinaryReader reader = new BinaryReader(fs, Encoding.UTF8);

            int count = reader.ReadInt32();

            for (int i = 0; i < count; i++)
            {
                ulong length = reader.ReadUInt64();
                string key = Encoding.UTF8.GetString(reader.ReadBytes((int)length));

                Int64 unixTime = reader.ReadInt64();
                DateTimeOffset offset = DateTimeOffset.FromUnixTimeSeconds(unixTime);

                fc.FilesInCache.Add(key, offset.DateTime);
            }

            reader.Close();
            fs.Close();

            return fc;
        }
    }
}
