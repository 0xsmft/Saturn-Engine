using System.Collections.Generic;
using System.IO;
using System.Text;

namespace SaturnBuildTool
{
    public class FileDependency
    {
        public string Filepath { get; set; }

        // The files that this files relies on
        // For example,
        // A.cpp includes A.h and B.h
        // B.h includes C.h
        // C.h has no dependencies 
        //
        // So in this List it would be 
        // FileDependencies.Count = 2
        // [0] = A.h
        // [1] = B.h (which would have a list to C.h)
        public List<FileDependency> FileDependencies = new List<FileDependency>();

        public FileDependency() { }

        public FileDependency( string path )
        {
            Filepath = path;
        }

        public void ParseIncludes()
        {
            foreach( var incs in CppIncludes.ParseImmediateIncludes( Filepath ) )
            {
                if( incs != null )
                {
                    var fd = new FileDependency( incs );
                    FileDependencies.Add( fd );
                    fd.ParseIncludes();
                }
            }
        }

        private void WriteDep( FileDependency other, StreamWriter streamWriter )
        {
            foreach( var dep in other.FileDependencies )
            {
                streamWriter.WriteLine( dep.Filepath );
                WriteDep( dep, streamWriter );
            }
        }

        public void Write()
        {
            string outFile = Path.Combine( Shared.ProjectInfo.BuildDir, $"{Path.GetFileName( Filepath )}.txt" );

            FileStream fs = new FileStream( outFile, FileMode.Create, FileAccess.Write, FileShare.ReadWrite );
            StreamWriter writer = new StreamWriter( fs, Encoding.UTF8 );

            foreach( var dep in FileDependencies )
            {
                writer.WriteLine( Filepath );

                WriteDep( dep, writer );
            }

            writer.Close();
            fs.Close();
        }

        public static FileDependency Read( string filePath )
        {
            var lines = File.ReadAllLines( filePath );
            if( lines.Length == 0 ) return null;

            var root = new FileDependency
            {
                Filepath = lines[ 0 ].Trim()
            };

            for( int i = 1; i < lines.Length; i++ )
            {
                var dep = new FileDependency
                {
                    Filepath = lines[ i ].Trim()
                };
                root.FileDependencies.Add( dep );
            }

            return root;
        }
    }
}
