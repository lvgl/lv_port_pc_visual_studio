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

        internal XmlElement projectClInclude = null;
        internal XmlElement projectClCompile = null;
        internal XmlElement projectNone = null;

        internal XmlElement filtersFilter = null;
        internal XmlElement filtersClInclude = null;
        internal XmlElement filtersClCompile = null;
        internal XmlElement filtersNone = null;

        internal VisualStudioCppItemsProjectGenerator()
        {
            projectDocument = new XmlDocument();
            if (projectDocument != null)
            {
                projectClInclude = projectDocument.CreateElement(
                    "ItemGroup",
                    defaultNamespace);
                projectClCompile = projectDocument.CreateElement(
                    "ItemGroup",
                    defaultNamespace);
                projectNone = projectDocument.CreateElement(
                    "ItemGroup",
                    defaultNamespace);
            }

            filtersDocument = new XmlDocument();
            if (filtersDocument != null)
            {
                filtersFilter = filtersDocument.CreateElement(
                    "ItemGroup",
                    defaultNamespace);
                filtersClInclude = filtersDocument.CreateElement(
                    "ItemGroup",
                    defaultNamespace);
                filtersClCompile = filtersDocument.CreateElement(
                    "ItemGroup",
                    defaultNamespace);
                filtersNone = filtersDocument.CreateElement(
                    "ItemGroup",
                    defaultNamespace);
            }
        }

        internal void EnumerateFolder(
            string path)
        {
            DirectoryInfo folder = new DirectoryInfo(path);

            if (filtersFilter != null)
            {
                XmlElement filter = filtersDocument.CreateElement(
                    "Filter",
                    defaultNamespace);
                if (filter != null)
                {
                    filter.SetAttribute(
                        "Include",
                        folder.FullName);
                    XmlElement uniqueIdentifier = filtersDocument.CreateElement(
                        "UniqueIdentifier",
                        defaultNamespace);
                    if (uniqueIdentifier != null)
                    {
                        uniqueIdentifier.InnerText =
                            "{" + Guid.NewGuid().ToString() + "}";
                        filter.AppendChild(uniqueIdentifier);
                    }
                    filtersFilter.AppendChild(filter);
                }
            }

            foreach (var item in folder.GetDirectories())
            {
                EnumerateFolder(item.FullName);
            }

            foreach (var item in folder.GetFiles())
            {
                if (item.Extension == ".h" || item.Extension == ".hpp")
                {
                    if (projectDocument != null)
                    {
                        XmlElement clInclude = projectDocument.CreateElement(
                            "ClInclude",
                            defaultNamespace);
                        if (clInclude != null)
                        {
                            clInclude.SetAttribute(
                                "Include",
                                item.FullName);
                            projectClInclude.AppendChild(clInclude);
                        }
                    }

                    if (filtersDocument != null)
                    {
                        XmlElement clInclude = filtersDocument.CreateElement(
                            "ClInclude",
                            defaultNamespace);
                        if (clInclude != null)
                        {
                            clInclude.SetAttribute(
                                "Include",
                                item.FullName);
                            XmlElement filter = filtersDocument.CreateElement(
                                "Filter",
                                defaultNamespace);
                            if (filter != null)
                            {
                                filter.InnerText = item.Directory.FullName;
                                clInclude.AppendChild(filter);
                            }
                            filtersClInclude.AppendChild(clInclude);
                        }
                    }
                }
                else if (item.Extension == ".c" || item.Extension == ".cpp")
                {
                    if (projectDocument != null)
                    {
                        XmlElement clCompile = projectDocument.CreateElement(
                            "ClCompile",
                            defaultNamespace);
                        if (clCompile != null)
                        {
                            clCompile.SetAttribute(
                                "Include",
                                item.FullName);
                            projectClCompile.AppendChild(clCompile);
                        }
                    }

                    if (filtersDocument != null)
                    {
                        XmlElement clCompile = filtersDocument.CreateElement(
                            "ClCompile",
                            defaultNamespace);
                        if (clCompile != null)
                        {
                            clCompile.SetAttribute(
                                "Include",
                                item.FullName);
                            XmlElement filter = filtersDocument.CreateElement(
                                "Filter",
                                defaultNamespace);
                            if (filter != null)
                            {
                                filter.InnerText = item.Directory.FullName;
                                clCompile.AppendChild(filter);
                            }
                            filtersClCompile.AppendChild(clCompile);
                        }
                    }
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

        internal void CreateFiles(
            string rootPath,
            string filePath,
            string fileName)
        {
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

                xmlElement.AppendChild(projectClInclude);
                xmlElement.AppendChild(projectClCompile);
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

                xmlElement.AppendChild(filtersFilter);
                xmlElement.AppendChild(filtersClInclude);
                xmlElement.AppendChild(filtersClCompile);
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
