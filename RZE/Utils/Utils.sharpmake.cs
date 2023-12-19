using Sharpmake;

[Generate]
public class Utils : BaseProject
{
    public Utils()
        : base()
    {
        Name = "Utils";
        SourceRootPath = "Src";
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.Output = Configuration.OutputType.Lib;

        conf.PrecompHeader = "StdAfx.h";
        conf.PrecompSource = "StdAfx.cpp";
    }
}