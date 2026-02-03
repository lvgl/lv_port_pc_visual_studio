using Microsoft.Build.Construction;

namespace LvglProjectFileUpdater
{
    public class Utilities
    {
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
