using System.Diagnostics;
using System.IO;
using System.Linq;

namespace SaturnBuildTool.Tools
{
    static internal class VSWhere
    {
        public static string FindVSRootDir()
        {
            const string VSWherePath = "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe";

            ProcessStartInfo vswinfo = new ProcessStartInfo
            {
                FileName = VSWherePath,
                CreateNoWindow = true,
                Arguments = "-legacy -prerelease -latest -property installationPath",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false
            };

            Process vswhere = new Process
            {
                StartInfo = vswinfo
            };

            vswhere.Start();
            vswhere.WaitForExit();

            return vswhere.StandardOutput.ReadToEnd().Trim();
        }

        public static string FindMSVCToolsDir()
        {
            string VSWherePath = FindVSRootDir();
            string CLLocation = Path.Combine( VSWherePath, "VC", "Tools", "MSVC" );

            // We now have folder with the version name, but we need make sure that the first one will be the highest version.
            var files = Directory.EnumerateDirectories( CLLocation ).OrderByDescending( filename => filename );

            foreach( string d in files )
            {
                // Return first one should be the newest
                return d;
            }

            return null;
        }
    }
}
