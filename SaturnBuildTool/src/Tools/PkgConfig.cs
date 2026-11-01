using System.Diagnostics;

namespace SaturnBuildTool.Tools
{
    static internal class PkgConfig
    {
        public static string FindPackageLibs( string packageName )
        {
            ProcessStartInfo pkgConfigInfo = new ProcessStartInfo
            {
                FileName = "pkg-config",
                CreateNoWindow = true,
                Arguments = $"--libs {packageName}",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false
            };

            Process pkgConfig = new Process
            {
                StartInfo = pkgConfigInfo
            };

            pkgConfig.Start();
            pkgConfig.WaitForExit();

            return pkgConfig.StandardOutput.ReadToEnd().Trim();
        }

        public static string FindPackageCFlags( string packageName )
        {
            ProcessStartInfo pkgConfigInfo = new ProcessStartInfo
            {
                FileName = "pkg-config",
                CreateNoWindow = true,
                Arguments = $"--cflags {packageName}",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false
            };

            Process pkgConfig = new Process
            {
                StartInfo = pkgConfigInfo
            };

            pkgConfig.Start();
            pkgConfig.WaitForExit();

            return pkgConfig.StandardOutput.ReadToEnd().Trim();
        }
    }
}
