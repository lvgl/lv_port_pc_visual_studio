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

        private static void AppendFilterElementToItems(
            XmlElement Item,
            string Name)
        {
            XmlElement Element = Item.OwnerDocument.CreateElement(
                "Filter",
                DefaultNamespaceString);
            Element.InnerText = Name;
            Item.AppendChild(Element);
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
            XmlElement HeaderItems = Project.OwnerDocument.CreateElement(
                "ItemGroup",
                DefaultNamespaceString);
            foreach (var Name in HeaderNames)
            {
                XmlElement Item = CreateItemElement(
                    Project.OwnerDocument,
                    "ClInclude",
                    Name.Target);
                HeaderItems.AppendChild(Item);
            }
            Project.AppendChild(HeaderItems);

            XmlElement SourceItems = Project.OwnerDocument.CreateElement(
               "ItemGroup",
               DefaultNamespaceString);
            foreach (var Name in SourceNames)
            {
                XmlElement Item = CreateItemElement(
                    Project.OwnerDocument,
                    "ClCompile",
                    Name.Target);
                SourceItems.AppendChild(Item);
            }
            Project.AppendChild(SourceItems);

            XmlElement OtherItems = Project.OwnerDocument.CreateElement(
               "ItemGroup",
               DefaultNamespaceString);
            foreach (var Name in OtherNames)
            {
                XmlElement Item = CreateItemElement(
                    Project.OwnerDocument,
                    "None",
                    Name.Target);
                OtherItems.AppendChild(Item);
            }
            Project.AppendChild(OtherItems);
        }

        public static void AppendItemsToCppFilters(
            XmlElement Project,
            List<string> FilterNames,
            List<(string Target, string Filter)> HeaderNames,
            List<(string Target, string Filter)> SourceNames,
            List<(string Target, string Filter)> OtherNames)
        {
            XmlElement FilterItems = Project.OwnerDocument.CreateElement(
                "ItemGroup",
                DefaultNamespaceString);
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
            Project.AppendChild(FilterItems);

            XmlElement HeaderItems = Project.OwnerDocument.CreateElement(
                "ItemGroup",
                DefaultNamespaceString);
            foreach (var Name in HeaderNames)
            {
                XmlElement Item = CreateItemElement(
                    Project.OwnerDocument,
                    "ClInclude",
                    Name.Target);
                AppendFilterElementToItems(Item, Name.Filter);
                HeaderItems.AppendChild(Item);
            }
            Project.AppendChild(HeaderItems);

            XmlElement SourceItems = Project.OwnerDocument.CreateElement(
               "ItemGroup",
               DefaultNamespaceString);
            foreach (var Name in SourceNames)
            {
                XmlElement Item = CreateItemElement(
                    Project.OwnerDocument,
                    "ClCompile",
                    Name.Target);
                AppendFilterElementToItems(Item, Name.Filter);
                SourceItems.AppendChild(Item);
            }
            Project.AppendChild(SourceItems);

            XmlElement OtherItems = Project.OwnerDocument.CreateElement(
               "ItemGroup",
               DefaultNamespaceString);
            foreach (var Name in OtherNames)
            {
                XmlElement Item = CreateItemElement(
                    Project.OwnerDocument,
                    "None",
                    Name.Target);
                AppendFilterElementToItems(Item, Name.Filter);
                OtherItems.AppendChild(Item);
            }
            Project.AppendChild(OtherItems);
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
