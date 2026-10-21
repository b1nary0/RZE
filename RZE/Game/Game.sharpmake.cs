using Sharpmake;
using System.IO;

[Generate]
public class Game : BaseProject
{
    public Game()
        : base()
    {
        Name = "Game";
        SourceRootPath = "Src";
        // The icon
        ResourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "Resources", "Game.rc"));
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Engine>(target);
        ConfigureAsWindowedApp(conf);
    }
}