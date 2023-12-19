using Sharpmake;
using System.IO;

[Generate]
public class Externals : BaseProject
{
    public Externals()
        : base()
    {
        Name = "Externals";
        SourceRootPath = "Src";
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.Output = Configuration.OutputType.Lib;
    }
}