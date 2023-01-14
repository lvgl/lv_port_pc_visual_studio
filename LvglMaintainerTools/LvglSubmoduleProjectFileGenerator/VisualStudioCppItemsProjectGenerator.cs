using System.Text;
using System.Xml;

namespace LvglSubmoduleProjectFileGenerator
{
    public class VisualStudioCppItemsProjectGenerator
    {
        internal string defaultNamespace =
            @"http://schemas.microsoft.com/developer/msbuild/2003";

        internal XmlDocument projectDocument = null;
        internal XmlDocument filtersDocument = null;

        internal XmlElement projectNone = null;

        internal XmlElement filtersNone = null;

        List<string> FilterNames = new List<string>();
        List<(string, string)> HeaderNames = new List<(string, string)>();
        List<(string, string)> SourceNames = new List<(string, string)>();

        internal VisualStudioCppItemsProjectGenerator()
        {
            projectDocument = new XmlDocument();
            if (projectDocument != null)
            {
                projectNone = projectDocument.CreateElement(
                    "ItemGroup",
                    defaultNamespace);
            }

            filtersDocument = new XmlDocument();
            if (filtersDocument != null)
            {
                filtersNone = filtersDocument.CreateElement(
                    "ItemGroup",
                    defaultNamespace);
            }
        }

        internal void EnumerateFolder(
            string path)
        {
            DirectoryInfo folder = new DirectoryInfo(path);

            FilterNames.Add(folder.FullName);

            foreach (var item in folder.GetDirectories())
            {
                EnumerateFolder(item.FullName);
            }

            foreach (var item in folder.GetFiles())
            {
                if (item.Extension == ".h" || item.Extension == ".hpp")
                {
                    HeaderNames.Add((item.FullName, item.Directory.FullName));
                        }
                else if (item.Extension == ".c" || item.Extension == ".cpp")
                {
                    SourceNames.Add((item.FullName, item.Directory.FullName));
                }
                else
                {
                    if (projectDocument != null)
                    {
                        XmlElement none = projectDocument.CreateElement(
                            "None",
                            defaultNamespace);
                        if (none != null)
                        {
                            none.SetAttribute(
                                "Include",
                                item.FullName);
                            projectNone.AppendChild(none);
                        }
                    }

                    if (filtersDocument != null)
                    {
                        XmlElement none = filtersDocument.CreateElement(
                            "None",
                            defaultNamespace);
                        if (none != null)
                        {
                            none.SetAttribute(
                                "Include",
                                item.FullName);
                            XmlElement filter = filtersDocument.CreateElement(
                                "Filter",
                                defaultNamespace);
                            if (filter != null)
                            {
                                filter.InnerText = item.Directory.FullName;
                                none.AppendChild(filter);
                            }
                            filtersNone.AppendChild(none);
                        }
                    }
                }
            }
        }

        internal XmlElement BuildFilterItemsFromFilterNames(
            List<string> FilterNames)
        {
            XmlElement FilterItems = filtersDocument.CreateElement(
                "ItemGroup",
                defaultNamespace);
            foreach (var FilterName in FilterNames)
            {
                XmlElement FilterItem = filtersDocument.CreateElement(
                    "Filter",
                    defaultNamespace);
                if (FilterItem != null)
                {
                    FilterItem.SetAttribute(
                        "Include",
                        FilterName);
                    XmlElement UniqueIdentifier = filtersDocument.CreateElement(
                        "UniqueIdentifier",
                        defaultNamespace);
                    if (UniqueIdentifier != null)
                    {
                        UniqueIdentifier.InnerText =
                            string.Format("{{{0}}}", Guid.NewGuid());
                        FilterItem.AppendChild(UniqueIdentifier);
                    }
                    FilterItems.AppendChild(FilterItem);
                }
            }

            return FilterItems;
        }

        internal (XmlElement, XmlElement) BuildHeaderItemsFromHeaderNames(
            List<(string, string)> HeaderNames)
        {
            XmlElement ProjectItems = projectDocument.CreateElement(
                "ItemGroup",
                defaultNamespace);
            XmlElement FiltersItems = filtersDocument.CreateElement(
                "ItemGroup",
                defaultNamespace);

            foreach (var HeaderName in HeaderNames)
            {
                XmlElement ProjectItem = projectDocument.CreateElement(
                    "ClInclude",
                    defaultNamespace);
                if (ProjectItem != null)
                {
                    ProjectItem.SetAttribute(
                        "Include",
                        HeaderName.Item1);
                    ProjectItems.AppendChild(ProjectItem);
                }

                XmlElement FiltersItem = filtersDocument.CreateElement(
                    "ClInclude",
                    defaultNamespace);
                if (FiltersItem != null)
                {
                    FiltersItem.SetAttribute(
                        "Include",
                        HeaderName.Item1);
                    XmlElement Filter = filtersDocument.CreateElement(
                        "Filter",
                        defaultNamespace);
                    if (Filter != null)
                    {
                        Filter.InnerText = HeaderName.Item2;
                        FiltersItem.AppendChild(Filter);
                    }
                    FiltersItems.AppendChild(FiltersItem);
                }
            }

            return (ProjectItems, FiltersItems);
        }

        internal (XmlElement, XmlElement) BuildSourceItemsFromSourceNames(
            List<(string, string)> SourceNames)
        {
            XmlElement ProjectItems = projectDocument.CreateElement(
                "ItemGroup",
                defaultNamespace);
            XmlElement FiltersItems = filtersDocument.CreateElement(
                "ItemGroup",
                defaultNamespace);

            foreach (var SourceName in SourceNames)
            {
                XmlElement ProjectItem = projectDocument.CreateElement(
                    "ClCompile",
                    defaultNamespace);
                if (ProjectItem != null)
                {
                    ProjectItem.SetAttribute(
                        "Include",
                        SourceName.Item1);
                    ProjectItems.AppendChild(ProjectItem);
                }

                XmlElement FiltersItem = filtersDocument.CreateElement(
                    "ClCompile",
                    defaultNamespace);
                if (FiltersItem != null)
                {
                    FiltersItem.SetAttribute(
                        "Include",
                        SourceName.Item1);
                    XmlElement Filter = filtersDocument.CreateElement(
                        "Filter",
                        defaultNamespace);
                    if (Filter != null)
                    {
                        Filter.InnerText = SourceName.Item2;
                        FiltersItem.AppendChild(Filter);
                    }
                    FiltersItems.AppendChild(FiltersItem);
                }
            }

            return (ProjectItems, FiltersItems);
        }

        internal void CreateFiles(
            string rootPath,
            string filePath,
            string fileName)
        {
            (XmlElement ProjectItems, XmlElement FiltersItems) HeaderItems =
                BuildHeaderItemsFromHeaderNames(HeaderNames);
            (XmlElement ProjectItems, XmlElement FiltersItems) SourceItems =
                BuildSourceItemsFromSourceNames(SourceNames);

            if (projectDocument != null)
            {
                projectDocument.InsertBefore(
                    projectDocument.CreateXmlDeclaration("1.0", "utf-8", null),
                    projectDocument.DocumentElement);

                XmlElement xmlElement = projectDocument.CreateElement(
                    "Project",
                    defaultNamespace);
                xmlElement.SetAttribute(
                    "ToolsVersion",
                    "4.0");

                xmlElement.AppendChild(HeaderItems.ProjectItems);
                xmlElement.AppendChild(SourceItems.ProjectItems);
                xmlElement.AppendChild(projectNone);

                projectDocument.AppendChild(xmlElement);

                projectDocument.InnerXml = projectDocument.InnerXml.Replace(
                    rootPath,
                    "");

                XmlWriterSettings writerSettings = new XmlWriterSettings();
                writerSettings.Indent = true;
                writerSettings.IndentChars = "  ";
                writerSettings.NewLineChars = "\r\n";
                writerSettings.NewLineHandling = NewLineHandling.Replace;
                writerSettings.Encoding = new UTF8Encoding(true);
                if (writerSettings != null)
                {
                    XmlWriter writer = XmlWriter.Create(
                       string.Format(
                           @"{0}\{1}.vcxitems",
                           filePath,
                           fileName),
                       writerSettings);
                    if (writer != null)
                    {
                        projectDocument.Save(writer);
                        writer.Flush();
                        writer.Dispose();
                    }

                }
            }

            if (filtersDocument != null)
            {
                filtersDocument.InsertBefore(
                    filtersDocument.CreateXmlDeclaration("1.0", "utf-8", null),
                    filtersDocument.DocumentElement);

                XmlElement xmlElement = filtersDocument.CreateElement(
                    "Project",
                    defaultNamespace);
                xmlElement.SetAttribute(
                    "ToolsVersion",
                    "4.0");

                xmlElement.AppendChild(
                    BuildFilterItemsFromFilterNames(FilterNames));
                xmlElement.AppendChild(HeaderItems.FiltersItems);
                xmlElement.AppendChild(SourceItems.FiltersItems);
                xmlElement.AppendChild(filtersNone);

                filtersDocument.AppendChild(xmlElement);

                filtersDocument.InnerXml = filtersDocument.InnerXml.Replace(
                    rootPath,
                    "");

                XmlWriterSettings writerSettings = new XmlWriterSettings();
                writerSettings.Indent = true;
                writerSettings.IndentChars = "  ";
                writerSettings.NewLineChars = "\r\n";
                writerSettings.NewLineHandling = NewLineHandling.Replace;
                writerSettings.Encoding = new UTF8Encoding(true);
                if (writerSettings != null)
                {
                    XmlWriter writer = XmlWriter.Create(
                       string.Format(
                           @"{0}\{1}.vcxitems.filters",
                           filePath,
                           fileName),
                       writerSettings);
                    if (writer != null)
                    {
                        filtersDocument.Save(writer);
                        writer.Flush();
                        writer.Dispose();
                    }
                }
            }
        }

        public static void Generate(
            string inputFolder,
            string inputRootPath,
            string outputFilePath,
            string outputFileName)
        {
            VisualStudioCppItemsProjectGenerator generator =
                new VisualStudioCppItemsProjectGenerator();
            if (generator != null)
            {
                generator.EnumerateFolder(
                    inputFolder);
                generator.CreateFiles(
                    inputRootPath,
                    outputFilePath,
                    outputFileName);
            }
        }
    }
}
