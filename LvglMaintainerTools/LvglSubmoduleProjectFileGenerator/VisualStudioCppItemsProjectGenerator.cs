using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Xml.Linq;

namespace LvglSubmoduleProjectFileGenerator
{
    public class VisualStudioCppItemsProjectGenerator
    {
        List<string> FilterNames = new List<string>();
        List<(string, string)> HeaderNames = new List<(string, string)>();
        List<(string, string)> SourceNames = new List<(string, string)>();
        List<(string, string)> OtherNames = new List<(string, string)>();

        internal VisualStudioCppItemsProjectGenerator()
        {
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
                    OtherNames.Add((item.FullName, item.Directory.FullName));
                }
            }
        }

        internal void CreateFiles(
            string rootPath,
            string filePath,
            string fileName)
        {
            List<(string, string)> NewHeaderNames = new List<(string, string)>();
            List<(string, string)> NewSourceNames = new List<(string, string)>();
            List<(string, string)> NewOtherNames = new List<(string, string)>();

            foreach (var HeaderName in HeaderNames)
            {
                NewHeaderNames.Add((
                    HeaderName.Item1.Replace(
                        rootPath,
                        "$(MSBuildThisFileDirectory)"),
                    HeaderName.Item2));
            }

            foreach (var SourceName in SourceNames)
            {
                NewSourceNames.Add((
                    SourceName.Item1.Replace(
                        rootPath,
                        "$(MSBuildThisFileDirectory)"),
                    SourceName.Item2));
            }

            foreach (var OtherName in OtherNames)
            {
                NewOtherNames.Add((
                    OtherName.Item1.Replace(
                        rootPath,
                        "$(MSBuildThisFileDirectory)"),
                    OtherName.Item2));
            }


            XmlWriterSettings WriterSettings = new XmlWriterSettings();
            WriterSettings.Indent = true;
            WriterSettings.IndentChars = "  ";
            WriterSettings.NewLineChars = "\r\n";
            WriterSettings.NewLineHandling = NewLineHandling.Replace;
            WriterSettings.Encoding = new UTF8Encoding(true);

            {
                XmlDocument Document =
                    VisualStudioCppSharedProjectCreator.CreateProjectDocument(
                        Guid.NewGuid(),
                        NewHeaderNames,
                        NewSourceNames,
                        NewOtherNames);
                Document.InnerXml = Document.InnerXml.Replace(
                    rootPath,
                    "");
                XmlWriter Writer = XmlWriter.Create(
                    string.Format(
                        @"{0}\{1}.vcxitems",
                        filePath,
                        fileName),
                    WriterSettings);
                Document.Save(Writer);
                Writer.Flush();
                Writer.Dispose();
            }

            {
                XmlDocument Document =
                    VisualStudioCppSharedProjectCreator.CreateFiltersDocument(
                        FilterNames,
                        NewHeaderNames,
                        NewSourceNames,
                        NewOtherNames);
                Document.InnerXml = Document.InnerXml.Replace(
                    rootPath,
                    "");
                XmlWriter Writer = XmlWriter.Create(
                    string.Format(
                        @"{0}\{1}.vcxitems.filters",
                        filePath,
                        fileName),
                    WriterSettings);
                Document.Save(Writer);
                Writer.Flush();
                Writer.Dispose();
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
