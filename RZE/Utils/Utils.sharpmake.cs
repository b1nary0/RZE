using Sharpmake;

[Generate]
public class Utils : BaseProject
{
    public Utils()
        : base()
    {
        Name = "Utils";
        SourceRootPath = "Src";
        //AdditionalSourceRootPaths.Add(Path.Combine(kEASTLPath, "include"));
        // NatvisFiles.Add(Path.Combine(kEASTLPath, "doc", "EASTL.natvis"));
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.Output = Configuration.OutputType.Lib;

        conf.PrecompHeader = "Utils/StdAfx.h";
        conf.PrecompSource = "Utils/StdAfx.cpp";

        //IncludeDir,
        //SourceDir,
        //RootDir .. "Utils/Src/",
        //RootDir .. "Modules/Rendering/Src/"
        //conf.SolutionFolder = "Modules";
        //conf.IncludePaths.Add("../../Engine/Src");
        //conf.IncludePaths.Add("../../Utils/Src");
        //conf.IncludePaths.Add("../../Modules/Rendering/Src");
        //conf.LibraryPaths.Add("C:/Program Files/Mono/lib");
        //conf.LibraryFiles.Add("mono-2.0-sgen");
        //conf.Defines.Add("MONO_HOME \"C:/Program Files/Mono/\"");
    }
}