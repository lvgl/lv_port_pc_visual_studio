using Mile.DotNet.Helpers;
using System.Collections.Generic;

namespace Lvgl.Build.Tasks
{
    public class Utilities
    {
        public static void GenerateDefinitionFile(
            string OutputFilePath,
            string LibraryName,
            SortedSet<string> Symbols)
        {
            string Content = string.Format(
                "LIBRARY {0}\r\n\r\nEXPORTS\r\n\r\n",
                LibraryName);

            foreach (string Symbol in Symbols)
            {
                Content += string.Format("{0}\r\n", Symbol);
            }

            Text.SaveTextToFileAsUtf8WithBom(OutputFilePath, Content);
        }
    }
}
