using Sharpmake;
using System.IO;

// The Editor with a harness for automation drivers (driver.py --harness): it takes commands on stdin and answers on
// stdout. It lives beside the Editor, so the Editor itself has none of it.
[Generate]
public class EditorHarness : BaseProject
{
    public EditorHarness()
        : base()
    {
        Name = "EditorHarness";
        SourceRootPath = "Src";
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.IncludePaths.Add(Path.Combine(Globals.RootDir, "Utils"));
        conf.IncludePaths.Add(Path.Combine(Globals.RootDir, "Editor", "Src"));
        EditorLib.LinkWhole(conf, target);
        // A console program on Windows, unlike the Editor, so its stdin and stdout are there for a driver
    }
}
