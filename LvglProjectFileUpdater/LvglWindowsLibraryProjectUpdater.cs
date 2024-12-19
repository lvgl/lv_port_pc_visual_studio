using Microsoft.Build.Construction;
using Mile.Project.Helpers;
using System.Text;

namespace LvglProjectFileUpdater
{
    public class LvglWindowsLibraryProjectUpdater
    {
        private static string RepositoryRoot = GitRepository.GetRootPath();

        private static List<string> FilterNames =
            new List<string>();
        private static List<(string Target, string Filter)> HeaderNames =
            new List<(string Target, string Filter)>();
        private static List<(string Target, string Filter)> SourceNames =
            new List<(string Target, string Filter)>();
        private static List<(string Target, string Filter)> OtherNames =
            new List<(string Target, string Filter)>();

        private static string[] HeaderFileTypes = new string[]
        {
            ".h",
            ".hh",
            ".hpp",
            ".hxx",
            ".h++",
            ".hm",
            ".inl",
            ".inc",
            ".ipp"
        };

        private static string[] SourceFileTypes = new string[]
        {
            ".cpp",
            ".c",
            ".cc",
            ".cxx",
            ".c++",
            ".cppm",
            ".ixx"
        };

        private static string[] ForceInOthersList = new string[]
        {
            @".devcontainer",
            @".github",
            @"docs",
            @"tests",
            @"lvgl\env_support",
            @"lvgl\scripts",
            @"freetype\",
            @"lvgl\demos",
            @"lvgl\examples"
        };

        private static void EnumerateFolder(
            string Path,
            bool ForceInOthers = false)
        {
            DirectoryInfo Folder = new DirectoryInfo(Path);

            FilterNames.Add(Folder.FullName);

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
                    Item.FullName,
                    ForceInOthers || CurrentForceInOthers);
            }

            foreach (var Item in Folder.GetFiles())
            {
                (string Target, string Filter) Current =
                    (Item.FullName, Item.Directory.FullName);

                if (ForceInOthers)
                {
                    OtherNames.Add(Current);
                    continue;
                }

                if (HeaderFileTypes.Contains(Item.Extension))
                {
                    HeaderNames.Add(Current);
                }
                else if (SourceFileTypes.Contains(Item.Extension))
                {
                    SourceNames.Add(Current);
                }
                else
                {
                    OtherNames.Add(Current);
                }
            }
        }

        public static void Run()
        {
            string RootPath = Path.GetFullPath(
                RepositoryRoot + @"\LvglPlatform\");

            FilterNames.Clear();
            HeaderNames.Clear();
            SourceNames.Clear();
            OtherNames.Clear();

            EnumerateFolder(RootPath + @"lvgl");

            List<string> NewFilterNames = new List<string>();
            List<(string, string)> NewHeaderNames = new List<(string, string)>();
            List<(string, string)> NewSourceNames = new List<(string, string)>();
            List<(string, string)> NewOtherNames = new List<(string, string)>();

            foreach (var FilterName in FilterNames)
            {
                NewFilterNames.Add(
                    FilterName.Replace(
                        RootPath,
                        @""));
            }

            foreach (var HeaderName in HeaderNames)
            {
                NewHeaderNames.Add((
                    HeaderName.Item1.Replace(
                        RootPath,
                        @"$(MSBuildThisFileDirectory)..\LvglPlatform\"),
                    HeaderName.Item2.Replace(
                        RootPath,
                        @"")));
            }

            foreach (var SourceName in SourceNames)
            {
                NewSourceNames.Add((
                    SourceName.Item1.Replace(
                        RootPath,
                        @"$(MSBuildThisFileDirectory)..\LvglPlatform\"),
                    SourceName.Item2.Replace(
                        RootPath,
                        @"")));
            }

            foreach (var OtherName in OtherNames)
            {
                NewOtherNames.Add((
                    OtherName.Item1.Replace(
                        RootPath,
                        @"$(MSBuildThisFileDirectory)..\LvglPlatform\"),
                    OtherName.Item2.Replace(
                        RootPath,
                        @"")));
            }

            ProjectRootElement ProjectRoot = ProjectRootElement.Open(
                string.Format(
                    @"{0}\LvglWindowsStatic.vcxproj",
                    Path.GetFullPath(
                        RepositoryRoot + @"\LvglWindows\")));

            foreach (ProjectItemElement Item in ProjectRoot.Items)
            {
                if (Item.Include.StartsWith(
                    @"$(MSBuildThisFileDirectory)..\LvglPlatform\"))
                {
                    Item.Parent.RemoveChild(Item);
                }
            }

            ProjectRootElement FiltersRoot = ProjectRootElement.Open(
                string.Format(
                    @"{0}\LvglWindowsStatic.vcxproj.filters",
                    Path.GetFullPath(
                        RepositoryRoot + @"\LvglWindows\")));

            foreach (ProjectItemElement Item in FiltersRoot.Items)
            {
                if (Item.Include.StartsWith(@"lvgl\") ||
                    Item.Include == "lvgl" ||
                    Item.Include.StartsWith(
                        @"$(MSBuildThisFileDirectory)..\LvglPlatform\"))
                {
                    Item.Parent.RemoveChild(Item);
                }
            }

            foreach (var CurrentName in NewFilterNames)
            {
                ProjectItemElement Item =
                    FiltersRoot.AddItem("Filter", CurrentName);
                Item.AddMetadata(
                    "UniqueIdentifier",
                    string.Format("{{{0}}}", Guid.NewGuid()));
            }

            foreach (var CurrentName in NewHeaderNames)
            {
                ProjectRoot.AddItem("ClInclude", CurrentName.Item1);

                {
                    ProjectItemElement Item =
                        FiltersRoot.AddItem("ClInclude", CurrentName.Item1);
                    Item.AddMetadata("Filter", CurrentName.Item2);
                }
            }

            foreach (var CurrentName in NewSourceNames)
            {
                {
                    ProjectItemElement Item =
                        ProjectRoot.AddItem("ClCompile", CurrentName.Item1);
                    Item.AddMetadata(
                        "AdditionalOptions",
                        "/utf-8 %(AdditionalOptions)");
                    Item.AddMetadata(
                        "LanguageStandard",
                        "Default");
                }

                {
                    ProjectItemElement Item =
                        FiltersRoot.AddItem("ClCompile", CurrentName.Item1);
                    Item.AddMetadata("Filter", CurrentName.Item2);
                }
            }

            foreach (var CurrentName in NewOtherNames)
            {
                ProjectRoot.AddItem("None", CurrentName.Item1);

                {
                    ProjectItemElement Item =
                        FiltersRoot.AddItem("None", CurrentName.Item1);
                    Item.AddMetadata("Filter", CurrentName.Item2);
                }
            }

            ProjectRoot.Save(Encoding.UTF8);

            FiltersRoot.Save(Encoding.UTF8);
        }
    }
}
