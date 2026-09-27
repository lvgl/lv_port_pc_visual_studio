using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;

namespace Lvgl.Build.Tasks
{
    public class MigrateLvglProject : Task
    {
        [Required]
        public string TargetFilePath { get; set; }

        [Required]
        public string RootPath { get; set; }

        public bool IncludeFreetype { get; set; }

        public bool ExcludeDemosAndExamples { get; set; }

        [Output]
        public bool ProjectChanged { get; set; }

        public override bool Execute()
        {
            ProjectChanged = false;

            try
            {
                string FullRootPath = Path.GetFullPath(RootPath);
                string FullProjectPath = Path.GetFullPath(TargetFilePath);
                string FullFiltersPath = FullProjectPath + ".filters";

                List<string> ForceInOthersList = new List<string>
                {
                    @".devcontainer",
                    @".github",
                    @"docs",
                    @"tests",
                    @"lvgl\env_support",
                    @"lvgl\scripts",
                    @"freetype\"
                };

                if (ExcludeDemosAndExamples)
                {
                    ForceInOthersList.Add(@"lvgl\demos");
                    ForceInOthersList.Add(@"lvgl\examples");
                }

                string[] ForceInOthers = ForceInOthersList.ToArray();

                List<string> FilterNames = new List<string>();
                List<(string Target, string ItemType)> FileNames =
                    new List<(string Target, string ItemType)>();

                if (IncludeFreetype)
                {
                    Utilities.EnumerateFolder(
                        FullRootPath,
                        "freetype",
                        FilterNames,
                        FileNames,
                        ForceInOthers);
                }

                Utilities.EnumerateFolder(
                    FullRootPath,
                    "lvgl",
                    FilterNames,
                    FileNames,
                    ForceInOthers);

                XmlDocument ProjectRoot = new XmlDocument();
                ProjectRoot.Load(FullProjectPath);

                XmlDocument FiltersRoot = new XmlDocument();
                FiltersRoot.Load(FullFiltersPath);

                string OriginalProject = ProjectRoot.OuterXml;
                string OriginalFilters = FiltersRoot.OuterXml;

                Utilities.RemoveGeneratedItems(ProjectRoot);
                Utilities.RemoveGeneratedItems(FiltersRoot);

                HashSet<string> ExistingFilters =
                    new HashSet<string>(StringComparer.Ordinal);

                foreach (XmlElement Item in
                    Utilities.GetProjectItems(FiltersRoot))
                {
                    if (Item.LocalName == "Filter")
                    {
                        ExistingFilters.Add(Item.GetAttribute("Include"));
                    }
                }

                foreach (string CurrentName in FilterNames)
                {
                    if (!ExistingFilters.Add(CurrentName))
                    {
                        continue;
                    }

                    XmlElement Item =
                        Utilities.AddItem(
                            FiltersRoot,
                            "Filter",
                            CurrentName);

                    Utilities.AddMetadata(
                        Item,
                        "UniqueIdentifier",
                        string.Format("{{{0}}}", Guid.NewGuid()));
                }

                Utilities.AddFiles(ProjectRoot, FiltersRoot, FileNames);

                bool ProjectNeedsSave = !string.Equals(
                    OriginalProject,
                    ProjectRoot.OuterXml,
                    StringComparison.Ordinal);

                bool FiltersNeedsSave = !string.Equals(
                    OriginalFilters,
                    FiltersRoot.OuterXml,
                    StringComparison.Ordinal);

                XmlWriterSettings Settings = new XmlWriterSettings
                {
                    Encoding = Encoding.UTF8,
                    Indent = true,
                    IndentChars = "  ",
                    NewLineChars = "\r\n"
                };

                if (ProjectNeedsSave)
                {
                    using (XmlWriter Writer = XmlWriter.Create(
                        FullProjectPath,
                        Settings))
                    {
                        ProjectRoot.Save(Writer);
                    }

                    ProjectChanged = true;
                }

                if (FiltersNeedsSave)
                {
                    using (XmlWriter Writer = XmlWriter.Create(
                        FullFiltersPath,
                        Settings))
                    {
                        FiltersRoot.Save(Writer);
                    }
                }

                if (ProjectNeedsSave || FiltersNeedsSave)
                {
                    Log.LogMessage(
                        MessageImportance.High,
                        "Migrated LVGL project '{0}'.",
                        FullProjectPath);
                }

                return !Log.HasLoggedErrors;
            }
            catch (Exception Exception)
            {
                Log.LogErrorFromException(Exception, true);
                return false;
            }
        }
    }
}
