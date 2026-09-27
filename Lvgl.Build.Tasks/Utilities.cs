using Mile.DotNet.Helpers;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Xml;

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

        public static string GetItemType(string FilePath)
        {
            switch (Path.GetExtension(FilePath).ToLowerInvariant())
            {
                case ".h":
                case ".hh":
                case ".hpp":
                case ".hxx":
                case ".h++":
                case ".hm":
                case ".inl":
                case ".inc":
                case ".ipp":
                    return "ClInclude";
                case ".cpp":
                case ".c":
                case ".cc":
                case ".cxx":
                case ".c++":
                case ".cppm":
                case ".ixx":
                    return "ClCompile";
                default:
                    return "None";
            }
        }

        public static void AddMetadata(
            XmlElement Item,
            string Name,
            string Value)
        {
            XmlDocument Document = Item.OwnerDocument
                ?? throw new InvalidOperationException(
                    "The item does not belong to a document.");

            XmlElement Metadata = Document.CreateElement(
                Item.Prefix,
                Name,
                Item.NamespaceURI);

            Metadata.InnerText = Value;
            Item.AppendChild(Metadata);
        }

        public static List<XmlElement> GetProjectItems(
            XmlDocument Document)
        {
            XmlElement Root = Document.DocumentElement
                ?? throw new InvalidDataException(
                    "The project document has no root element.");

            List<XmlElement> Result = new List<XmlElement>();

            foreach (XmlNode Node in Root.ChildNodes)
            {
                if (!(Node is XmlElement Group) ||
                    Group.LocalName != "ItemGroup" ||
                    Group.NamespaceURI != Root.NamespaceURI)
                {
                    continue;
                }

                foreach (XmlNode Child in Group.ChildNodes)
                {
                    if (Child is XmlElement Item &&
                        Item.NamespaceURI == Root.NamespaceURI)
                    {
                        Result.Add(Item);
                    }
                }
            }

            return Result;
        }

        public static XmlElement AddItem(
            XmlDocument Document,
            string ItemType,
            string Include)
        {
            XmlElement Root = Document.DocumentElement
                ?? throw new InvalidDataException(
                    "The project document has no root element.");

            XmlElement TargetGroup = null;
            XmlElement EmptyGroup = null;

            foreach (XmlNode Node in Root.ChildNodes)
            {
                if (!(Node is XmlElement Group) ||
                    Group.LocalName != "ItemGroup" ||
                    Group.NamespaceURI != Root.NamespaceURI ||
                    Group.HasAttribute("Condition"))
                {
                    continue;
                }

                bool HasElements = false;

                foreach (XmlNode Child in Group.ChildNodes)
                {
                    if (!(Child is XmlElement Item))
                    {
                        continue;
                    }

                    HasElements = true;

                    if (Item.LocalName == ItemType &&
                        Item.NamespaceURI == Root.NamespaceURI)
                    {
                        TargetGroup = Group;
                        break;
                    }
                }

                if (TargetGroup != null)
                {
                    break;
                }

                if (!HasElements && EmptyGroup == null)
                {
                    EmptyGroup = Group;
                }
            }

            if (TargetGroup == null)
            {
                TargetGroup = EmptyGroup;
            }

            if (TargetGroup == null)
            {
                TargetGroup = Document.CreateElement(
                    Root.Prefix,
                    "ItemGroup",
                    Root.NamespaceURI);

                Root.AppendChild(TargetGroup);
            }

            XmlElement Result = Document.CreateElement(
                Root.Prefix,
                ItemType,
                Root.NamespaceURI);

            Result.SetAttribute("Include", Include);
            TargetGroup.AppendChild(Result);

            return Result;
        }

        public static void EnumerateFolder(
            string RootPath,
            string RelativeFolderPath,
            List<string> FilterNames,
            List<(string Target, string ItemType)> FileNames,
            string[] ForceInOthersList,
            bool ForceInOthers = false)
        {
            DirectoryInfo Folder = new DirectoryInfo(
                Path.Combine(RootPath, RelativeFolderPath));

            FilterNames.Add(RelativeFolderPath);

            foreach (var Item in Folder.GetDirectories())
            {
                bool CurrentForceInOthers = false;
                foreach (var ListItem in ForceInOthersList)
                {
                    if (Item.FullName.Contains(ListItem))
                    {
                        CurrentForceInOthers = true;
                        break;
                    }
                }

                EnumerateFolder(
                    RootPath,
                    Path.Combine(RelativeFolderPath, Item.Name),
                    FilterNames,
                    FileNames,
                    ForceInOthersList,
                    ForceInOthers || CurrentForceInOthers);
            }

            foreach (var Item in Folder.GetFiles())
            {
                string CurrentName =
                    Path.Combine(RelativeFolderPath, Item.Name);
                string ItemType = ForceInOthers
                    ? "None"
                    : GetItemType(Item.FullName);

                FileNames.Add((CurrentName, ItemType));
            }
        }

        public static void AddFiles(
            XmlDocument ProjectRoot,
            XmlDocument FiltersRoot,
            IEnumerable<(string Target, string ItemType)> Names)
        {
            foreach (var CurrentName in Names)
            {
                string Include =
                    @"$(MSBuildThisFileDirectory)..\LvglPlatform\" +
                    CurrentName.Target;
                string Filter =
                    Path.GetDirectoryName(CurrentName.Target) ?? "";
                string ItemType = CurrentName.ItemType;

                {
                    XmlElement Item =
                        AddItem(ProjectRoot, ItemType, Include);

                    if (ItemType == "ClCompile")
                    {
                        AddMetadata(
                            Item,
                            "AdditionalOptions",
                            "/utf-8 %(AdditionalOptions)");
                        AddMetadata(
                            Item,
                            "LanguageStandard",
                            "Default");
                    }
                }

                {
                    XmlElement Item =
                        AddItem(FiltersRoot, ItemType, Include);
                    AddMetadata(Item, "Filter", Filter);
                }
            }
        }

        public static void RemoveGeneratedItems(XmlDocument Document)
        {
            foreach (XmlElement Item in GetProjectItems(Document))
            {
                if (Item.GetAttribute("Include").StartsWith(
                    @"$(MSBuildThisFileDirectory)..\LvglPlatform\",
                    StringComparison.Ordinal))
                {
                    Item.ParentNode?.RemoveChild(Item);
                }
            }
        }
    }
}
