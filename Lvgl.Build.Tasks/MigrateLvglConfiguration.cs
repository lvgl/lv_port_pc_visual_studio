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

        [Required]
        public string DefaultConfigurationFilePath { get; set; }

        private static readonly Regex OptionRule = new Regex(
            @"#define\s+([A-Z0-9_]+)[^\S\r\n]*",
            RegexOptions.Compiled);

        private Dictionary<string, string> ParseDefaultConfiguration(string FilePath)
        {
            Dictionary<string, string> Result =
                new Dictionary<string, string>();

            foreach (string Line in File.ReadLines(FilePath, Encoding.UTF8))
            {
                if (string.IsNullOrWhiteSpace(Line) || Line.StartsWith("#"))
                {
                    continue;
                }

                Match MatchedResult = Regex.Match(
                    Line,
                    @"([A-Z0-9_]+)\s+(.+)");

                if (!MatchedResult.Success)
                {
                    Log.LogWarning(
                        "Ignoring invalid default configuration line: {0}",
                        Line);
                    continue;
                }

                Result[MatchedResult.Groups[1].Value] =
                    MatchedResult.Groups[2].Value;
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

            string DefaultConfigurationFileFullPath = Path.GetFullPath(
                DefaultConfigurationFilePath);
            if (!File.Exists(DefaultConfigurationFileFullPath))
            {
                Log.LogError(
                    "Please ensure that the default configuration file '{0}' exists.",
                    DefaultConfigurationFileFullPath);
                return false;
            }

            Log.LogMessage(
                MessageImportance.High,
                "Migrating LVGL configuration '{0}' with template '{1}' " +
                "and default configuration '{2}'.",
                TargetFileFullPath,
                TemplateFileFullPath,
                DefaultConfigurationFileFullPath);

            Dictionary<string, string> DefaultConfiguration =
                ParseDefaultConfiguration(DefaultConfigurationFileFullPath);

            HashSet<string> UsedKeys = new HashSet<string>();
            StringBuilder Content = new StringBuilder();

            foreach (string SourceLine in File.ReadLines(
                TemplateFileFullPath,
                Encoding.UTF8))
            {
                string DestinationLine = SourceLine;
                Match MatchedResult = OptionRule.Match(SourceLine);

                if (MatchedResult.Success &&
                    DefaultConfiguration.TryGetValue(
                        MatchedResult.Groups[1].Value,
                        out string Value))
                {
                    string Key = MatchedResult.Groups[1].Value;
                    string Prefix = MatchedResult.Value;

                    if (!char.IsWhiteSpace(Prefix[Prefix.Length - 1]))
                    {
                        Prefix += " ";
                    }

                    DestinationLine =
                        SourceLine.Substring(0, MatchedResult.Index) +
                        Prefix +
                        Value;

                    UsedKeys.Add(Key);

                    Log.LogMessage(
                        MessageImportance.Normal,
                        "Applying: {0} = {1}",
                        Key,
                        Value);
                }
                else if (SourceLine.Contains(
                    "Set this to \"1\" to enable content"))
                {
                    DestinationLine = "#if 1 /* Enable content */";
                }

                Content.Append(DestinationLine);
                Content.Append("\r\n");
            }

            foreach (string Key in DefaultConfiguration.Keys)
            {
                if (!UsedKeys.Contains(Key))
                {
                    Log.LogWarning(
                        "Default configuration option '{0}' was not found in " +
                        "the template.",
                        Key);
                }
            }

            File.WriteAllText(
                TargetFileFullPath,
                Content.ToString(),
                Encoding.UTF8);

            Log.LogMessage(
                MessageImportance.High,
                "Successfully migrated '{0}'.",
                TargetFileFullPath);

            return !Log.HasLoggedErrors;
        }
    }
}
