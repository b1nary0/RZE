using Sharpmake;

[Generate]
public class Engine : BaseProject
{
    public Engine()
        : base()
    {
        Name = "Engine";
        SourceRootPath = "Engine/Src";
        //AdditionalSourceRootPaths.Add(Path.Combine(kEASTLPath, "include"));
        // NatvisFiles.Add(Path.Combine(kEASTLPath, "doc", "EASTL.natvis"));
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Utils>(target);

        conf.PrecompHeader = "StdAfx.h";
        conf.PrecompSource = "StdAfx.cpp";

        conf.LibraryPaths.Add(Globals.LibDir);
        conf.LibraryPaths.Add(Globals.ThirdPartyLibDir);
        conf.LibraryFiles.Add("OptickCore");
        //IncludeDir,
        //SourceDir,
        //RootDir .. "Utils/Src/",
        //RootDir .. "Modules/Rendering/Src/"
        //conf.SolutionFolder = "Modules";
        conf.IncludePaths.Add("../../ThirdParty/Include");
        conf.IncludePaths.Add("../../Engine/Src");
        conf.IncludePaths.Add("../../Utils/Src");
        conf.IncludePaths.Add("../../Modules/Rendering/Src");
        conf.Defines.Add("MONO_HOME \"C:/Program Files/Mono/\"");
    }
}