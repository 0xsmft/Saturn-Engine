using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;

namespace SaturnBuildTool
{
    internal static class HeaderToolExt
    {
        public static bool RunHeaderTool() 
        {
            var Args = new List<string>();

            ProcessStartInfo processStart = new ProcessStartInfo();
            processStart.CreateNoWindow = true;
            processStart.RedirectStandardOutput = true;
            processStart.RedirectStandardError = true;
            processStart.UseShellExecute = false;
            processStart.FileName = ProjectInfo.Instance.HeaderToolExePath;

            Process headerToolProcess = new Process
            {
                StartInfo = processStart
            };
            headerToolProcess.EnableRaisingEvents = true;
            headerToolProcess.OutputDataReceived += new DataReceivedEventHandler((s, e) =>
            {
                if (e.Data != null)
                {
                    Console.WriteLine(e.Data);
                }
            });

            headerToolProcess.ErrorDataReceived += new DataReceivedEventHandler((s, e) =>
            {
                if (e.Data != null)
                {
                    Console.WriteLine(e.Data);
                }
            });

            // Args
            Args.Add(" /NOMSG ");
            Args.Add(string.Format(" /SRC={0}", ProjectInfo.Instance.SourceDir));
            Args.Add(string.Format(" /OUT={0}", ProjectInfo.Instance.BuildDir));

            processStart.Arguments = string.Join("", Args);

            Console.WriteLine( "Generating Code..." );
            Stopwatch sw = Stopwatch.StartNew();

            headerToolProcess.Start();
            headerToolProcess.BeginErrorReadLine();
            headerToolProcess.BeginOutputReadLine();
            headerToolProcess.WaitForExit();

            Console.WriteLine( "Done generating in {0}", sw.Elapsed );

            return headerToolProcess.ExitCode == 0;
        }
    }
}
