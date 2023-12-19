using Sharpmake;
using System.IO;

[Generate]
public class Engine : BaseProject
{
    public Engine()
        : base()
    {
        Name = "Engine";
        SourceRootPath = "Src";
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.Output = Configuration.OutputType.Lib;
        conf.AddPublicDependency<Utils>(target);
        conf.AddPublicDependency<Externals>(target);
        conf.AddPublicDependency<Rendering>(target);

        conf.PrecompHeader = "StdAfx.h";
        conf.PrecompSource = "StdAfx.cpp";

        conf.IncludePaths.Add("../../Engine/Src");

        conf.LibraryPaths.Add(Globals.LibDir);
        conf.LibraryPaths.Add(Globals.ThirdPartyLibDir);

        conf.LibraryFiles.Add("OptickCore");

        var copyDirBuildStep = new Configuration.BuildStepCopy(
            Globals.ThirdPartyDllDir,
            Globals.LibDir);

        copyDirBuildStep.IsFileCopy = true;
        copyDirBuildStep.CopyPattern = "*.dll";
        conf.EventPostBuildExe.Add(copyDirBuildStep);
    }
}