using System;
using System.Collections.Generic;
using System.IO;
using Sharpmake;

[module: Sharpmake.Include("Make/Sharpmake/CommonTarget.sharpmake.cs")]
[module: Sharpmake.Include("Make/Sharpmake/BaseProject.sharpmake.cs")]
[module: Sharpmake.Include("Make/Sharpmake/Sharpmake.sharpmake.cs")]
[module: Sharpmake.Include("Engine/Engine.sharpmake.cs")]
// Libraries
[module: Sharpmake.Include("Utils/Utils.sharpmake.cs")]
[module: Sharpmake.Include("SourceAssetBurner/SourceAssetBurner.sharpmake.cs")]
[module: Sharpmake.Include("Externals/Externals.sharpmake.cs")]
[module: Sharpmake.Include("Modules/Rendering/Rendering.sharpmake.cs")]
// Applications
[module: Sharpmake.Include("Editor/Editor.sharpmake.cs")]
[module: Sharpmake.Include("Game/Game.sharpmake.cs")]


[Generate]
public class RZE : Solution
{
    public RZE()
        : base(typeof(CommonTarget))
    {
        Name = "RZE";
        AddTargets(CommonTarget.GetDefaultTargets());

        IsFileNameToLower = false;
    }

    [Configure]
    public virtual void ConfigureAll(Solution.Configuration conf, CommonTarget target)
    {
        conf.SolutionPath = @"[solution.SharpmakeCsPath]";
        conf.SolutionFileName = "[solution.Name]";
        Globals.RootDir = Path.GetFullPath("./");

        conf.AddProject<Engine>(target);
        conf.AddProject<SharpmakeProjectBase>(target);
        conf.AddProject<Rendering>(target);
        conf.AddProject<Utils>(target);
        conf.AddProject<SourceAssetBurner>(target);
        conf.AddProject<Externals>(target);
        conf.AddProject<Editor>(target);
        conf.AddProject<Game>(target);
    }
}

public class Globals
{
    public static string RootDir = "./";
    public static string IncludeDir { get { return RootDir + "ThirdParty/Include/"; } }
    public static string LibDir { get { return Path.Combine(RootDir, "_Build/[target.Name]/"); } }
    public static string ThirdPartyLibDir { get { return Path.Combine(RootDir, "ThirdParty/Lib/x64/"); } }
    public static string ThirdPartyDllDir { get { return Path.Combine(RootDir, "ThirdParty/Dll/x64/"); } }
}


public static class Main
{
    [Sharpmake.Main]
    public static void SharpmakeMain(Sharpmake.Arguments arguments)
    {
        KitsRootPaths.SetUseKitsRootForDevEnv(DevEnv.vs2017, KitsRootEnum.KitsRoot10, Options.Vc.General.WindowsTargetPlatformVersion.v10_0_18362_0);
        arguments.Generate<RZE>();
    }
}