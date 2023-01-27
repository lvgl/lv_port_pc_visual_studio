namespace LvglSubmoduleProjectFileGenerator
{

    internal class Program
    {
        static void Main(string[] args)
        {
            string Root = GitRepositoryUtilities.GetRepositoryRoot();

            Console.WriteLine(Root);

            string rootPath = Path.GetFullPath(Root + @"\LvglPlatform");

            VisualStudioCppItemsProjectGenerator.Generate(
                rootPath + @"\lvgl",
                rootPath + @"\",
                rootPath,
                @"LVGL.Portable");

            VisualStudioCppItemsProjectGenerator.Generate(
                rootPath + @"\lv_drivers",
                rootPath + @"\",
                rootPath,
                @"LVGL.Drivers");

            Console.WriteLine("Hello, World!");

            Console.ReadKey();
        }
    }
}
