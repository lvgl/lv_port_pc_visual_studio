using Microsoft.Build.Construction;

namespace LvglProjectFileUpdater
{
    public class Utilities
    {
        public static bool IsHeaderFile(string FilePath)
        {
            switch (Path.GetExtension(FilePath).ToLowerInvariant())
            {
                case ".h":
                case ".hh":
                case ".hpp":
                case ".hxx":
                case ".h++":
                case ".hm":
                case ".inl":
                case ".inc":
                case ".ipp":
                    return true;
                default:
                    return false;
            }
        }

        public static bool IsSourceFile(string FilePath)
        {
            switch (Path.GetExtension(FilePath).ToLowerInvariant())
            {
                case ".cpp":
                case ".c":
                case ".cc":
                case ".cxx":
                case ".c++":
                case ".cppm":
                case ".ixx":
                    return true;
                default:
                    return false;
            }
        }

        public static bool CheckProjectItemElementExists(
            ProjectRootElement RootElement,
            string ItemType,
            string Include)
        {
            foreach (ProjectItemElement Item in RootElement.Items)
            {
                if (ItemType == Item.ItemType && Include == Item.Include)
                {
                    return true;
                }
            }
            return false;
        }
    }
}
