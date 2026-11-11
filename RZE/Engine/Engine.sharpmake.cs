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

        UsePrecompiledHeader(conf, target, "EnginePCH");

        conf.IncludePaths.Add("../../Engine/Src");

        conf.LibraryPaths.Add(Globals.BuildDir(target));

        if (target.Platform == Platform.win64)
        {
            conf.LibraryPaths.Add(Globals.ThirdPartyLibDir);

            conf.LibraryFiles.Add("OptickCore");
            // DwmSetWindowAttribute
            conf.LibraryFiles.Add("dwmapi");
            // SetWindowSubclass (Windowing/Windows/WindowNativeWindows.cpp)
            conf.LibraryFiles.Add("comctl32");

            var copyDirBuildStep = new Configuration.BuildStepCopy(
                Globals.ThirdPartyDllDir,
                Globals.BuildDir(target));

            copyDirBuildStep.IsFileCopy = true;
            copyDirBuildStep.CopyPattern = "*.dll";
            conf.EventPostBuildExe.Add(copyDirBuildStep);
        }
        else
        {
            // Built from its source in ThirdParty (OptickCore.sharpmake.cs)
            conf.AddPublicDependency<OptickCore>(target);
        }
    }
}