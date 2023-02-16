using System.Collections.Generic;
using System.Reflection.Metadata;
using System.Xml;
using System.Xml.Linq;

namespace LvglSubmoduleProjectFileGenerator
{
    public class VisualStudioProjectUtilities
    {
        private static readonly string DefaultNamespaceString =
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

        private static XmlElement AppendItemToItemGroup(
            XmlElement ItemGroup,
            string Type,
            string Target)
        {
            XmlElement Element = ItemGroup.OwnerDocument.CreateElement(
                Type,
                DefaultNamespaceString);
            Element.SetAttribute(
                "Include",
                Target);
            ItemGroup.AppendChild(Element);
            return Element;
        }

        private static void AppendTagToElement(
            XmlElement Element,
            string Name,
            string Content)
        {
            XmlElement Tag = Element.OwnerDocument.CreateElement(
                Name,
                DefaultNamespaceString);
            Tag.InnerText = Content;
            Element.AppendChild(Tag);
        }

        private static void AppendFilterElementToItem(
            XmlElement Item,
            string Name)
        {
            if (Name != string.Empty)
            {
                AppendTagToElement(Item, "Filter", Name);
            }
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
                AppendItemToItemGroup(
                    HeaderItems,
                    "ClInclude",
                    Name.Target);
            }

            XmlElement SourceItems = AppendEmptyItemGroupToProject(Project);
            foreach (var Name in SourceNames)
            {
                AppendItemToItemGroup(
                    SourceItems,
                    "ClCompile",
                    Name.Target);
            }

            XmlElement OtherItems = AppendEmptyItemGroupToProject(Project);
            foreach (var Name in OtherNames)
            {
                AppendItemToItemGroup(
                    OtherItems,
                    "None",
                    Name.Target);
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
                XmlElement FilterItem = AppendItemToItemGroup(
                    FilterItems,
                    "Filter",
                    FilterName);
                AppendTagToElement(
                    FilterItem,
                    "UniqueIdentifier",
                    string.Format("{{{0}}}", Guid.NewGuid()));
            }

            XmlElement HeaderItems = AppendEmptyItemGroupToProject(Project);
            foreach (var Name in HeaderNames)
            {
                XmlElement Item = AppendItemToItemGroup(
                    HeaderItems,
                    "ClInclude",
                    Name.Target);
                AppendFilterElementToItem(Item, Name.Filter);
            }

            XmlElement SourceItems = AppendEmptyItemGroupToProject(Project);
            foreach (var Name in SourceNames)
            {
                XmlElement Item = AppendItemToItemGroup(
                    SourceItems,
                    "ClCompile",
                    Name.Target);
                AppendFilterElementToItem(Item, Name.Filter);
            }

            XmlElement OtherItems = AppendEmptyItemGroupToProject(Project);
            foreach (var Name in OtherNames)
            {
                XmlElement Item = AppendItemToItemGroup(
                    OtherItems,
                    "None",
                    Name.Target);
                AppendFilterElementToItem(Item, Name.Filter);
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

            XmlElement GlobalPropertyGroup = Document.CreateElement(
                "PropertyGroup",
                DefaultNamespaceString);
            GlobalPropertyGroup.SetAttribute(
                "Label",
                "Globals");
            AppendTagToElement(
                GlobalPropertyGroup,
                "ItemsProjectGuid",
                string.Format("{{{0}}}", ProjectGuid));
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
