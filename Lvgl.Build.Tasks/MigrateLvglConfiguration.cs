using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace Lvgl.Build.Tasks
{
    public class MigrateLvglConfiguration : Task
    {
        [Required]
        public string TargetFilePath { get; set; }

        [Required]
        public string TemplateFilePath { get; set; }

        private static readonly Regex OptionRule = new Regex(
            @"^#define\s+([A-Z0-9_]+)\s+((?:.|\n)*?)(?:\s*(?:\/\/|\/\*)|(?:\n\s*#))",
            RegexOptions.Compiled | RegexOptions.Multiline);

        private static SortedDictionary<string, string> ParseLvglConfiguration(
            string FilePath)
        {
            SortedDictionary<string, string> Result =
                new SortedDictionary<string, string>();

            string Content = File.ReadAllText(FilePath, Encoding.UTF8);

            // Normalize line endings to Unix style for easier processing.
            Content = Content.Replace("\r\n", "\n").Replace('\r', '\n');

            // Add a sentinel to the end of the file to ensure the last macro
            // is matched correctly.
            Content += "\n#";

            MatchCollection Matches = OptionRule.Matches(Content);

            foreach (Match MatchedResult in Matches)
            {
                if (!MatchedResult.Success)
                {
                    continue;
                }

                // Remove newlines and backslashes, merging multi-line values
                // into one line.
                string PreprocessedValue = Regex.Replace(
                    MatchedResult.Groups[2].Value,
                    @"\s*\\\s*\n\s*", " ").Trim();

                Result.Add(
                    MatchedResult.Groups[1].Value,
                    PreprocessedValue);
            }

            return Result;
        }

        public override bool Execute()
        {
            return true;
        }
    }
}
