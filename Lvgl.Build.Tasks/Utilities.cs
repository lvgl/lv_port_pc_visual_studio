using Mile.DotNet.Helpers;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

namespace Lvgl.Build.Tasks
{
    public class Utilities
    {
        public static void GenerateDefinitionFile(
            string OutputFilePath,
            string LibraryName,
            SortedSet<string> Symbols)
        {
            string Content = string.Format(
                "LIBRARY {0}\r\n\r\nEXPORTS\r\n\r\n",
                LibraryName);

            foreach (string Symbol in Symbols)
            {
                Content += string.Format("{0}\r\n", Symbol);
            }

            Text.SaveTextToFileAsUtf8WithBom(OutputFilePath, Content);
        }

        private static string RunProcess(
            string FileName,
            string Arguments,
            string WorkingDirectory)
        {
            try
            {
                Process CurrentProcess = new Process
                {
                    StartInfo = new ProcessStartInfo
                    {
                        CreateNoWindow = true,
                        UseShellExecute = false,
                        RedirectStandardOutput = true,
                        FileName = FileName,
                        Arguments = Arguments,
                        WorkingDirectory = WorkingDirectory
                    }
                };
                if (CurrentProcess.Start())
                {
                    string Result = CurrentProcess.StandardOutput.ReadToEnd();
                    CurrentProcess.WaitForExit();
                    if (CurrentProcess.ExitCode == 0)
                    {
                        return Result;
                    }
                }
            }
            catch
            {
                // Process execution is allowed to fail.
            }

            return string.Empty;
        }

        private static string RunGit(
            string Arguments,
            string WorkingDirectory)
        {
            return RunProcess(
                "git",
                Arguments,
                WorkingDirectory).Trim();
        }

        public static bool IsSubmoduleMigrationRequired(string SubmodulePath)
        {
            try
            {
                string RepositoryRoot = Git.GetRootPath();
                if (string.IsNullOrEmpty(RepositoryRoot))
                {
                    return false;
                }

                string SubmoduleFullPath = Path.GetFullPath(
                    Path.Combine(RepositoryRoot, SubmodulePath));
                if (!File.Exists(Path.Combine(SubmoduleFullPath, ".git")) &&
                    !Directory.Exists(Path.Combine(SubmoduleFullPath, ".git")))
                {
                    return false;
                }

                string BranchName = RunGit(
                    "symbolic-ref --quiet --short HEAD",
                    RepositoryRoot);
                if (string.IsNullOrEmpty(BranchName))
                {
                    return false;
                }

                string BaseCommit = RunGit(
                    string.Format(
                        "merge-base HEAD \"refs/remotes/origin/{0}\"",
                        BranchName),
                    RepositoryRoot);
                if (string.IsNullOrEmpty(BaseCommit))
                {
                    return false;
                }

                string BaseSubmoduleCommit = RunGit(
                    string.Format(
                        "rev-parse --verify \"{0}:{1}\"",
                        BaseCommit,
                        SubmodulePath.Replace('\\', '/')),
                    RepositoryRoot);
                if (string.IsNullOrEmpty(BaseSubmoduleCommit))
                {
                    return false;
                }

                string CurrentSubmoduleCommit = RunGit(
                    "rev-parse --verify HEAD",
                    SubmoduleFullPath);
                if (string.IsNullOrEmpty(CurrentSubmoduleCommit))
                {
                    return false;
                }

                return !string.Equals(
                    BaseSubmoduleCommit,
                    CurrentSubmoduleCommit,
                    StringComparison.Ordinal);
            }
            catch
            {
                // Skip automatic migration when Git inspection fails.
                return false;
            }
        }
    }
}
