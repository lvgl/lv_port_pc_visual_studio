using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;

namespace Lvgl.Build.Tasks
{
    public class MigrateLvglConfiguration : Task
    {
        [Required]
        public string TargetFilePath { get; set; }

        [Required]
        public string TemplateFilePath { get; set; }

        public override bool Execute()
        {
            return true;
        }
    }
}
