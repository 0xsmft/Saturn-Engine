using System.Diagnostics;
using System.IO;
using System.Linq;

namespace SaturnBuildTool.Tools
{
    internal class VSWhere
    {
        public static string FindVSRootDir() 
        {
            string VSWherePath = "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe";

            ProcessStartInfo vswinfo = new ProcessStartInfo();
            vswinfo.FileName = VSWherePath;
            vswinfo.CreateNoWindow = true;
            vswinfo.Arguments = "-legacy -prerelease -latest -property installationPath";
            vswinfo.RedirectStandardOutput = true;
            vswinfo.RedirectStandardError = true;
            vswinfo.UseShellExecute = false;

            Process vswhere = new Process();
            vswhere.StartInfo = vswinfo;
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

            foreach (string d in files)
            {
                // Return first one should be the newest
                return d;
            }

            return null;
        }
    }
}
