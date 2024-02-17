using Mile.Project.Helpers;
using System.Text;

namespace LvglModuleDefinitionGenerator
{
    internal class Program
    {
        const int ImageArchiveStartSize = 8;
        const string ImageArchiveStart = "!<arch>\n";
        const string ImageArchiveEnd = "`\n";
        const string ImageArchivePad = "\n";
        const string ImageArchiveLinkerMember = "/               ";
        const string ImageArchiveLongnamesMember = "//              ";
        const string ImageArchiveHybridmapMember = "/<HYBRIDMAP>/   ";

        const int ImageArchiveMemberHeaderNameSize = 16;
        const int ImageArchiveMemberHeaderDateSize = 12;
        const int ImageArchiveMemberHeaderUserIDSize = 6;
        const int ImageArchiveMemberHeaderGroupIDSize = 6;
        const int ImageArchiveMemberHeaderModeSize = 8;
        const int ImageArchiveMemberHeaderSizeSize = 10;
        const int ImageArchiveMemberHeaderEndHeaderSize = 2;

        static void GetAllSymbolsFromStaticLibraryFile(
            string FilePath,
            string Filters,
            ref List<string> Symbols)
        {
            string[] ConvertedFilters = Filters.Split(';');

            byte[] Content = File.ReadAllBytes(FilePath);

            int CurrentOffset = 0;

            string CurrentImageArchiveStart = Encoding.ASCII.GetString(
                Content,
                CurrentOffset,
                ImageArchiveStartSize);
            if (CurrentImageArchiveStart != ImageArchiveStart)
            {
                return;
            }

            CurrentOffset += ImageArchiveStartSize;
            string CurrentImageArchiveLinkerMember = Encoding.ASCII.GetString(
                Content,
                CurrentOffset,
                ImageArchiveMemberHeaderNameSize);
            if (CurrentImageArchiveLinkerMember != ImageArchiveLinkerMember)
            {
                return;
            }

            CurrentOffset += ImageArchiveMemberHeaderNameSize;
            CurrentOffset += ImageArchiveMemberHeaderDateSize;
            CurrentOffset += ImageArchiveMemberHeaderUserIDSize;
            CurrentOffset += ImageArchiveMemberHeaderGroupIDSize;
            CurrentOffset += ImageArchiveMemberHeaderModeSize;
            CurrentOffset += ImageArchiveMemberHeaderSizeSize;
            string CurrentImageArchiveMemberHeaderEndHeader =
                Encoding.ASCII.GetString(
                    Content,
                    CurrentOffset,
                    ImageArchiveMemberHeaderEndHeaderSize);
            if (CurrentImageArchiveMemberHeaderEndHeader != ImageArchiveEnd)
            {
                return;
            }

            CurrentOffset += ImageArchiveMemberHeaderEndHeaderSize;
            int SymbolsCount = 0;
            {
                byte[] RawBytes = new Span<byte>(
                    Content,
                    CurrentOffset,
                    4).ToArray();
                if (BitConverter.IsLittleEndian)
                {
                    Array.Reverse(RawBytes);
                }

                SymbolsCount = BitConverter.ToInt32(
                    RawBytes);
            }
            if (SymbolsCount == 0)
            {
                return;
            }

            CurrentOffset += sizeof(uint);
            CurrentOffset += sizeof(uint) * SymbolsCount;
            {
                string[] RawStrings = Encoding.ASCII.GetString(
                    Content,
                    CurrentOffset,
                    Content.Length - CurrentOffset).Split('\0');

                for (int i = 0; i < SymbolsCount; ++i)
                {
                    bool Excluded = true;
                    foreach (string Filter in ConvertedFilters)
                    {
                        if (RawStrings[i].StartsWith(Filter))
                        {
                            Excluded = false;
                            break;
                        }
                    }
                    if (Excluded)
                    {
                        continue;
                    }

                    Symbols.Add(RawStrings[i]);
                }
            }
        }
        static void Main(string[] args)
        {
            string ProjectRootPath = GitRepository.GetRootPath();

            Console.WriteLine(ProjectRootPath);

            List<KeyValuePair<string, string>> RootPaths = new List<KeyValuePair<string, string>>
            {
                new KeyValuePair<string, string>(
                    ProjectRootPath + @"\Output\Binaries\Release\x64\",
                    ProjectRootPath + @"\LvglWindows\LvglWindows.def"),
            };

            foreach (KeyValuePair<string, string> RootPath in RootPaths)
            {
                List<KeyValuePair<string, string>> Files =
                new List<KeyValuePair<string, string>>
                {
                    new KeyValuePair<string, string>(
                        RootPath.Key + "LvglWindowsStatic.lib",
                        "lv_;_lv_"),
                };

                List<string> Symbols = new List<string>();

                foreach (KeyValuePair<string, string> File in Files)
                {
                    GetAllSymbolsFromStaticLibraryFile(
                        File.Key,
                        File.Value,
                        ref Symbols);
                }

                Console.WriteLine(Symbols.Count);

                string Result = "LIBRARY\r\n\r\nEXPORTS\r\n\r\n";

                foreach (string Symbol in Symbols)
                {
                    Result += Symbol + "\r\n";
                }

                FileUtilities.SaveTextToFileAsUtf8Bom(RootPath.Value, Result);
            }

            Console.WriteLine("Hello, World!");
            Console.ReadKey();
        }
    }
}
