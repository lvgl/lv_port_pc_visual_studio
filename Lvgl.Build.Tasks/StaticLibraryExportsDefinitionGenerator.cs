using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using Mile.Project.Helpers;
using System.Collections.Generic;
using System.IO;

namespace Lvgl.Build.Tasks
{
    public class StaticLibraryExportsDefinitionGenerator : Task
    {
        [Required]
        public string InputLibraryFilePath { get; set; }

        [Required]
        public string TargetPlatform { get; set; }

        [Required]
        public string OutputExportsDefinitionPath { get; set; }

        public override bool Execute()
        {
            if (!File.Exists(InputLibraryFilePath))
            {
                Log.LogError(
                    "Cannot generate the exports definition because the " +
                    "input library '{0}' does not exist. Build the static " +
                    "library successfully first. If project migration " +
                    "stopped the build, reload the updated projects and " +
                    "build again.",
                    InputLibraryFilePath);

                return false;
            }

            ImageArchive.Archive Archive =
                ImageArchive.Parse(InputLibraryFilePath);

            if (Archive.Symbols != null)
            {
                SortedSet<string> Symbols = new SortedSet<string>();
                foreach (string Symbol in ImageArchive.ListSymbols(
                        Archive.Symbols,
                        TargetPlatform == "x86"))
                {
                    if (Symbol.StartsWith("lv_") ||
                        Symbol.StartsWith("_lv_"))
                    {
                        Symbols.Add(Symbol);
                    }
                }

                Utilities.GenerateDefinitionFile(
                    OutputExportsDefinitionPath,
                    string.Empty,
                    Symbols);
            }

            return true;
        }
    }
}
