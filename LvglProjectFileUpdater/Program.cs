using Microsoft.Build.Construction;
using Mile.DotNet.Helpers;
using System.Text;

namespace LvglProjectFileUpdater
{
    internal class Program
    {
        private static string GetItemType(string FilePath)
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

        private static void EnumerateFolder(
            string RootPath,
            string FolderPath,
            List<string> FilterNames,
            List<(string Target, string ItemType)> FileNames,
            string[] ForceInOthersList,
            bool ForceInOthers = false)
        {
            DirectoryInfo Folder = new DirectoryInfo(FolderPath);

            FilterNames.Add(Path.GetRelativePath(RootPath, Folder.FullName));

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
                    Item.FullName,
                    FilterNames,
                    FileNames,
                    ForceInOthersList,
                    ForceInOthers || CurrentForceInOthers);
            }

            foreach (var Item in Folder.GetFiles())
            {
                string CurrentName =
                    Path.GetRelativePath(RootPath, Item.FullName);
                string ItemType = ForceInOthers
                    ? "None"
                    : GetItemType(Item.FullName);

                FileNames.Add((CurrentName, ItemType));
            }
        }

        private static void AddFiles(
            ProjectRootElement ProjectRoot,
            ProjectRootElement FiltersRoot,
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
                    ProjectItemElement Item =
                        ProjectRoot.AddItem(ItemType, Include);

                    if (ItemType == "ClCompile")
                    {
                        Item.AddMetadata(
                            "AdditionalOptions",
                            "/utf-8 %(AdditionalOptions)");
                        Item.AddMetadata(
                            "LanguageStandard",
                            "Default");
                    }
                }

                {
                    ProjectItemElement Item =
                        FiltersRoot.AddItem(ItemType, Include);
                    Item.AddMetadata("Filter", Filter);
                }
            }
        }

        private static string RepositoryRoot = Git.GetRootPath();

        private static void UpdateProject(
            string ProjectPath,
            string[] ForceInOthersList,
            params string[] FolderNames)
        {
            string RootPath = Path.GetFullPath(
                RepositoryRoot + @"\LvglPlatform\");

            List<string> FilterNames = new List<string>();
            List<(string Target, string ItemType)> FileNames =
                new List<(string Target, string ItemType)>();

            foreach (var FolderName in FolderNames)
            {
                EnumerateFolder(
                    RootPath,
                    Path.Combine(RootPath, FolderName),
                    FilterNames,
                    FileNames,
                    ForceInOthersList);
            }

            string FullProjectPath = Path.GetFullPath(
                Path.Combine(RepositoryRoot, ProjectPath));

            ProjectRootElement ProjectRoot =
                ProjectRootElement.Open(FullProjectPath);

            foreach (ProjectItemElement Item in ProjectRoot.Items)
            {
                if (Item.Include.StartsWith(
                    @"$(MSBuildThisFileDirectory)..\LvglPlatform\"))
                {
                    Item.Parent.RemoveChild(Item);
                }
            }

            ProjectRootElement FiltersRoot =
                ProjectRootElement.Open(FullProjectPath + ".filters");

            foreach (ProjectItemElement Item in FiltersRoot.Items)
            {
                if (Item.Include.StartsWith(
                    @"$(MSBuildThisFileDirectory)..\LvglPlatform\"))
                {
                    Item.Parent.RemoveChild(Item);
                }
            }

            HashSet<string> ExistingFilters =
                new HashSet<string>(StringComparer.Ordinal);

            foreach (ProjectItemElement Item in FiltersRoot.Items)
            {
                if (Item.ItemType == "Filter")
                {
                    ExistingFilters.Add(Item.Include);
                }
            }

            foreach (var CurrentName in FilterNames)
            {
                if (!ExistingFilters.Add(CurrentName))
                {
                    continue;
                }

                {
                    ProjectItemElement Item =
                        FiltersRoot.AddItem("Filter", CurrentName);
                    Item.AddMetadata(
                        "UniqueIdentifier",
                        string.Format("{{{0}}}", Guid.NewGuid()));
                }
            }

            AddFiles(ProjectRoot, FiltersRoot, FileNames);

            ProjectRoot.Save(Encoding.UTF8);

            FiltersRoot.Save(Encoding.UTF8);
        }

        static void Main(string[] args)
        {
            if (RepositoryRoot == string.Empty)
            {
                throw new NotSupportedException();
            }
            Console.WriteLine(RepositoryRoot);

            string[] ForceInOthersList = new string[]
            {
                @".devcontainer",
                @".github",
                @"docs",
                @"tests",
                @"lvgl\env_support",
                @"lvgl\scripts",
                @"freetype\"
            };

            UpdateProject(
                @"LvglWindowsSimulator\LvglWindowsSimulator.vcxproj",
                ForceInOthersList,
                "freetype",
                "lvgl");

            UpdateProject(
                @"LvglWindowsDesktopApplication\LvglWindowsDesktopApplication.vcxproj",
                ForceInOthersList,
                "lvgl");

            UpdateProject(
                @"LvglWindows\LvglWindowsStatic.vcxproj",
                ForceInOthersList.Concat(new string[]
                {
                    @"lvgl\demos",
                    @"lvgl\examples"
                }).ToArray(),
                "lvgl");

            Console.WriteLine("Hello, World!");

            Console.ReadKey();
        }
    }
}
