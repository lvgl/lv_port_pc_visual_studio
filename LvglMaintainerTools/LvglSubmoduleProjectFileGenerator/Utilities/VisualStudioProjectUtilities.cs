using System.Collections.Generic;
using System.Reflection.Metadata;
using System.Xml;
using System.Xml.Linq;

namespace LvglSubmoduleProjectFileGenerator
{
    public class VisualStudioProjectUtilities
    {
        private static string DefaultNamespaceString =
            @"http://schemas.microsoft.com/developer/msbuild/2003";

        private static XmlElement AppendEmptyItemGroupToProject(
            XmlElement Project)
        {
            XmlElement Element = Project.OwnerDocument.CreateElement(
               "ItemGroup",
               DefaultNamespaceString);
            Project.AppendChild(Element);
            return Element;
        }

        private static void AppendFilterElementToItem(
            XmlElement Element,
            string Name)
        {
            if (Name != string.Empty)
            {
                XmlElement Filter = Element.OwnerDocument.CreateElement(
                    "Filter",
                    DefaultNamespaceString);
                Filter.InnerText = Name;
                Element.AppendChild(Filter);
            }
        }

        private static XmlElement CreateItemElement(
            XmlDocument Document,
            string Type,
            string Target)
        {
            XmlElement Element = Document.CreateElement(
                Type,
                DefaultNamespaceString);
            Element.SetAttribute(
                "Include",
                Target);
            return Element;
        }

        public static void AppendItemsToCppProject(
            XmlElement Project,
            List<(string Target, string Filter)> HeaderNames,
            List<(string Target, string Filter)> SourceNames,
            List<(string Target, string Filter)> OtherNames)
        {
            XmlElement HeaderItems = AppendEmptyItemGroupToProject(Project);
            foreach (var Name in HeaderNames)
            {
                XmlElement Item = CreateItemElement(
                    Project.OwnerDocument,
                    "ClInclude",
                    Name.Target);
                HeaderItems.AppendChild(Item);
            }

            XmlElement SourceItems = AppendEmptyItemGroupToProject(Project);
            foreach (var Name in SourceNames)
            {
                XmlElement Item = CreateItemElement(
                    Project.OwnerDocument,
                    "ClCompile",
                    Name.Target);
                SourceItems.AppendChild(Item);
            }

            XmlElement OtherItems = AppendEmptyItemGroupToProject(Project);
            foreach (var Name in OtherNames)
            {
                XmlElement Item = CreateItemElement(
                    Project.OwnerDocument,
                    "None",
                    Name.Target);
                OtherItems.AppendChild(Item);
            }
        }

        public static void AppendItemsToCppFilters(
            XmlElement Project,
            List<string> FilterNames,
            List<(string Target, string Filter)> HeaderNames,
            List<(string Target, string Filter)> SourceNames,
            List<(string Target, string Filter)> OtherNames)
        {
            XmlElement FilterItems = AppendEmptyItemGroupToProject(Project);
            foreach (var FilterName in FilterNames)
            {
                XmlElement FilterItem = Project.OwnerDocument.CreateElement(
                    "Filter",
                    DefaultNamespaceString);
                if (FilterItem != null)
                {
                    FilterItem.SetAttribute(
                        "Include",
                        FilterName);
                    XmlElement UniqueIdentifier =
                        Project.OwnerDocument.CreateElement(
                            "UniqueIdentifier",
                            DefaultNamespaceString);
                    if (UniqueIdentifier != null)
                    {
                        UniqueIdentifier.InnerText =
                            string.Format("{{{0}}}", Guid.NewGuid());
                        FilterItem.AppendChild(UniqueIdentifier);
                    }
                    FilterItems.AppendChild(FilterItem);
                }
            }

            XmlElement HeaderItems = AppendEmptyItemGroupToProject(Project);
            foreach (var Name in HeaderNames)
            {
                XmlElement Item = CreateItemElement(
                    Project.OwnerDocument,
                    "ClInclude",
                    Name.Target);
                AppendFilterElementToItem(Item, Name.Filter);
                HeaderItems.AppendChild(Item);
            }

            XmlElement SourceItems = AppendEmptyItemGroupToProject(Project);
            foreach (var Name in SourceNames)
            {
                XmlElement Item = CreateItemElement(
                    Project.OwnerDocument,
                    "ClCompile",
                    Name.Target);
                AppendFilterElementToItem(Item, Name.Filter);
                SourceItems.AppendChild(Item);
            }

            XmlElement OtherItems = AppendEmptyItemGroupToProject(Project);
            foreach (var Name in OtherNames)
            {
                XmlElement Item = CreateItemElement(
                    Project.OwnerDocument,
                    "None",
                    Name.Target);
                AppendFilterElementToItem(Item, Name.Filter);
                OtherItems.AppendChild(Item);
            }
        }

        public static XmlDocument CreateCppSharedProject(
            Guid ProjectGuid,
            List<(string Target, string Filter)> HeaderNames,
            List<(string Target, string Filter)> SourceNames,
            List<(string Target, string Filter)> OtherNames)
        {
            XmlDocument Document = new XmlDocument();

            Document.InsertBefore(
                Document.CreateXmlDeclaration("1.0", "utf-8", null),
                Document.DocumentElement);

            XmlElement Project = Document.CreateElement(
                "Project",
                DefaultNamespaceString);
            Project.SetAttribute(
                "ToolsVersion",
                "4.0");

            XmlElement ItemsProjectGuid = Document.CreateElement(
                "ItemsProjectGuid",
                DefaultNamespaceString);
            ItemsProjectGuid.InnerText =
                string.Format("{{{0}}}", ProjectGuid);
            XmlElement GlobalPropertyGroup = Document.CreateElement(
                "PropertyGroup",
                DefaultNamespaceString);
            GlobalPropertyGroup.SetAttribute(
                "Label",
                "Globals");
            GlobalPropertyGroup.AppendChild(ItemsProjectGuid);
            Project.AppendChild(GlobalPropertyGroup);

            AppendItemsToCppProject(
                Project,
                HeaderNames,
                SourceNames,
                OtherNames);

            Document.AppendChild(Project);

            return Document;
        }

        public static XmlDocument CreateCppSharedFilters(
            List<string> FilterNames,
            List<(string Target, string Filter)> HeaderNames,
            List<(string Target, string Filter)> SourceNames,
            List<(string Target, string Filter)> OtherNames)
        {
            XmlDocument Document = new XmlDocument();

            Document.InsertBefore(
                Document.CreateXmlDeclaration("1.0", "utf-8", null),
                Document.DocumentElement);

            XmlElement Project = Document.CreateElement(
                "Project",
                DefaultNamespaceString);
            Project.SetAttribute(
                "ToolsVersion",
                "4.0");

            AppendItemsToCppFilters(
                Project,
                FilterNames,
                HeaderNames,
                SourceNames,
                OtherNames);

            Document.AppendChild(Project);

            return Document;
        }
    }
}
