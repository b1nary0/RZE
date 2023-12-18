using Sharpmake;
using System.IO;

[Generate]
public class SourceAssetBurner : BaseProject
{
    public SourceAssetBurner()
        : base()
    {
        Name = "SourceAssetBurner";
        SourceRootPath = "Src";
        //AdditionalSourceRootPaths.Add(Path.Combine(kEASTLPath, "include"));
        // NatvisFiles.Add(Path.Combine(kEASTLPath, "doc", "EASTL.natvis"));
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.IncludePaths.Add(Path.Combine(Globals.RootDir, "Utils"));
        conf.AddPublicDependency<Utils>(target);
        conf.AddPublicDependency<Engine>(target);

        //conf.PrecompHeader = "StdAfx.h";
        //conf.PrecompSource = "StdAfx.cpp";

        //IncludeDir,
        //SourceDir,
        //RootDir .. "Utils/Src/",
        //RootDir .. "Modules/Rendering/Src/"
        //conf.SolutionFolder = "Modules";
        //conf.IncludePaths.Add("../../ThirdParty/Include");
        //conf.IncludePaths.Add("../../Engine/Src");
        //conf.IncludePaths.Add("../../Utils/Src");
        //conf.IncludePaths.Add("../../Modules/Rendering/Src");
        //conf.LibraryPaths.Add("C:/Program Files/Mono/lib");
        //conf.LibraryFiles.Add("mono-2.0-sgen");
        //conf.Defines.Add("MONO_HOME \"C:/Program Files/Mono/\"");
    }
}