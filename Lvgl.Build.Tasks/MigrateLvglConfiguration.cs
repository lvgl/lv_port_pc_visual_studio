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
            string TargetFileFullPath = Path.GetFullPath(TargetFilePath);
            if (!File.Exists(TargetFileFullPath))
            {
                Log.LogError(
                    "Please ensure that the target file '{0}' exists.",
                    TargetFileFullPath);
                return false;
            }

            string TemplateFileFullPath = Path.GetFullPath(TemplateFilePath);
            if (!File.Exists(TemplateFileFullPath))
            {
                Log.LogError(
                    "Please ensure that the template file '{0}' exists.",
                    TemplateFileFullPath);
                return false;
            }

            Log.LogMessage(
                MessageImportance.High,
                "Migrating LVGL configuration '{0}' with template '{1}'.",
                TargetFileFullPath,
                TemplateFileFullPath);


            SortedDictionary<string, string> CurrentOptions =
                ParseLvglConfiguration(TargetFileFullPath);

            SortedDictionary<string, string> TemplateOptions =
               ParseLvglConfiguration(TemplateFileFullPath);

            foreach (var CurrentOption in CurrentOptions)
            {
                if (!TemplateOptions.ContainsKey(CurrentOption.Key))
                {
                    Log.LogError(
                        "Please remove obsolete option '{0}' from '{1}'.",
                        CurrentOption.Key,
                        TargetFileFullPath);
                }
            }
            if (Log.HasLoggedErrors)
            {
                return false;
            }

            string Content = File.ReadAllText(
                TemplateFileFullPath,
                Encoding.UTF8);

            // Normalize line endings to Unix style for easier processing.
            Content = Content.Replace("\r\n", "\n").Replace('\r', '\n');

            Content = Regex.Replace(
                Content,
                @"^.*Set this to ""1"" to enable content.*$",
                "#if 1 /* Enable content */",
                RegexOptions.Multiline);

            Content = OptionRule.Replace(Content, MatchedResult =>
            {
                string Key = MatchedResult.Groups[1].Value;

                // Directly use the value from the pre-parsed dictionary.
                string TemplateValue = TemplateOptions[Key];

                if (CurrentOptions.TryGetValue(Key, out string CurrentValue) &&
                    CurrentValue != TemplateValue)
                {
                    Log.LogMessage(
                        MessageImportance.Normal,
                        "Applying {0} to {1} (Original: {2}).",
                        Key,
                        CurrentValue,
                        TemplateValue);

                    // Reconstruct the line with the new value.

                    int PrefixLength =
                        MatchedResult.Groups[2].Index - MatchedResult.Index;
                    int SuffixLength =
                        PrefixLength + MatchedResult.Groups[2].Length;

                    return string.Format(
                        "{0}{1}{2}",
                        MatchedResult.Value.Substring(0, PrefixLength),
                        CurrentValue,
                        MatchedResult.Value.Substring(SuffixLength));
                }

                // If no changes are needed for this key, return the original
                // matched string.
                return MatchedResult.Value;
            });

            // Normalize line endings to Windows style for the output file.
            Content = Content.Replace("\n", "\r\n");

            File.WriteAllText(TargetFileFullPath, Content, Encoding.UTF8);

            Log.LogMessage(
                MessageImportance.High,
                "Successfully migrated '{0}'.",
                TargetFileFullPath);

            return !Log.HasLoggedErrors;
        }
    }
}
