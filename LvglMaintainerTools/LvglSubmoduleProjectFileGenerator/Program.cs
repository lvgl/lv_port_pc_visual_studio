using System.Diagnostics;

namespace LvglSubmoduleProjectFileGenerator
{
    internal class Program
    {
        private static string GetGitRepositoryRoot()
        {
            Process process = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    CreateNoWindow = true,
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    FileName = "git.exe",
                    Arguments = "rev-parse --show-toplevel"
                }
            };

            if (process.Start())
            {
                process.WaitForExit();
                if (process.ExitCode == 0)
                {
                    string? result = process.StandardOutput.ReadLine();
                    if (result != null)
                    {
                        return Path.GetFullPath(result);
                    }
                }
            }

            return string.Empty;
        }

        static void Main(string[] args)
        {
            string Root = GetGitRepositoryRoot();

            Console.WriteLine(Root);

            Console.WriteLine("Hello, World!");

            Console.ReadKey();
        }
    }
}
