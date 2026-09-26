using Mile.DotNet.Helpers;
using System.Text;
using System.Xml;

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

        private static XmlElement AddItem(
            XmlDocument Document,
            string ItemType,
            string Include)
        {
            XmlElement Root = Document.DocumentElement
                ?? throw new InvalidDataException(
                    "The project document has no root element.");

            XmlElement? TargetGroup = null;
            XmlElement? EmptyGroup = null;

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

        private static void AddMetadata(
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

        private static void AddFiles(
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

        private static List<XmlElement> GetProjectItems(
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

        private static void RemoveGeneratedItems(XmlDocument Document)
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

            XmlDocument ProjectRoot = new XmlDocument();
            ProjectRoot.Load(FullProjectPath);

            XmlDocument FiltersRoot = new XmlDocument();
            FiltersRoot.Load(FullProjectPath + ".filters");

            RemoveGeneratedItems(ProjectRoot);
            RemoveGeneratedItems(FiltersRoot);

            HashSet<string> ExistingFilters =
                new HashSet<string>(StringComparer.Ordinal);

            foreach (XmlElement Item in GetProjectItems(FiltersRoot))
            {
                if (Item.LocalName == "Filter")
                {
                    ExistingFilters.Add(Item.GetAttribute("Include"));
                }
            }

            foreach (var CurrentName in FilterNames)
            {
                if (!ExistingFilters.Add(CurrentName))
                {
                    continue;
                }

                {
                    XmlElement Item =
                        AddItem(FiltersRoot, "Filter", CurrentName);
                    AddMetadata(
                        Item,
                        "UniqueIdentifier",
                        string.Format("{{{0}}}", Guid.NewGuid()));
                }
            }

            AddFiles(ProjectRoot, FiltersRoot, FileNames);

            XmlWriterSettings Settings = new XmlWriterSettings
            {
                Encoding = Encoding.UTF8,
                Indent = true,
                IndentChars = "  ",
                NewLineChars = "\r\n"
            };

            using (XmlWriter Writer = XmlWriter.Create(
                FullProjectPath,
                Settings))
            {
                ProjectRoot.Save(Writer);
            }

            using (XmlWriter Writer = XmlWriter.Create(
                FullProjectPath + ".filters",
                Settings))
            {
                FiltersRoot.Save(Writer);
            }
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

            {
                string[] LibraryForceInOthersList =
                    new string[ForceInOthersList.Length + 2];

                Array.Copy(
                    ForceInOthersList,
                    LibraryForceInOthersList,
                    ForceInOthersList.Length);

                LibraryForceInOthersList[ForceInOthersList.Length] =
                    @"lvgl\demos";
                LibraryForceInOthersList[ForceInOthersList.Length + 1] =
                    @"lvgl\examples";

                UpdateProject(
                    @"LvglWindows\LvglWindowsStatic.vcxproj",
                    LibraryForceInOthersList,
                    "lvgl");
            }

            Console.WriteLine("Hello, World!");

            Console.ReadKey();
        }
    }
}
