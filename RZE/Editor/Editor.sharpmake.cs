using Sharpmake;
using System.IO;

[Generate]
public class Editor : BaseProject
{
    public Editor()
        : base()
    {
        Name = "Editor";
        SourceRootPath = "Src";
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.IncludePaths.Add(Path.Combine(Globals.RootDir, "Utils"));
        conf.AddPublicDependency<Utils>(target);
        conf.AddPublicDependency<Engine>(target);

        conf.LibraryFiles.Add("assimp");
    }
}