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
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Utils>(target);
        conf.AddPublicDependency<Engine>(target);
        conf.AddPublicDependency<Rendering>(target);
    }
}