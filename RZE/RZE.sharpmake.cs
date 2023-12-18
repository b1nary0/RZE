using System;
using System.Collections.Generic;
using System.IO;
using Sharpmake;

[module: Sharpmake.Include("CommonTarget.sharpmake.cs")]
[module: Sharpmake.Include("BaseProject.sharpmake.cs")]
[module: Sharpmake.Include("Sharpmake.sharpmake.cs")]
[module: Sharpmake.Include("Engine.sharpmake.cs")]
// Libraries
[module: Sharpmake.Include("Utils/Utils.sharpmake.cs")]
[module: Sharpmake.Include("SourceAssetBurner/SourceAssetBurner.sharpmake.cs")]

[Generate]
public class BaseGameSolution : Solution
{
    public BaseGameSolution()
        : base(typeof(CommonTarget))
    {
        Name = "BaseGameSolution";

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
        conf.AddProject<Utils>(target);
        conf.AddProject<SourceAssetBurner>(target);
    }
}

public class Globals
{
    public static string RootDir = "./";
    public static string FMOD_Win64_Dir = string.Empty;
    public static string FMOD_UWP_Dir = string.Empty;
    public static string IncludeDir { get { return RootDir + "ThirdParty/Include/"; } }
    public static string LibDir { get { return Path.Combine(RootDir, "_Build/[target.Name]/[target.Platform]"); } }
    public static string ThirdPartyLibDir { get { return Path.Combine(RootDir, "ThirdParty/Lib/x64/"); } }
    //public static string LibDir = RootDir.."_Build/".."%{cfg.buildcfg}/".."%{cfg.platform}"

    public static string MONO_Win64_Dir = "C:/Program Files/Mono/";
    public static string MONO_macOS_Dir = "/Library/Frameworks/Mono.framework/";

    public static string UWP_Thumbprint = "ae94a2f10501aae7d3449e789a8e7ddd7c10baaf";
    public static string UWP_CertificateName = "Game_EntryPoint_UWP_TemporaryKey.pfx";
    /*
    function GetFullPath(pathStr)
	    local fullPath = debug.getinfo(1).source:match("@?(.*\/)")
	    local endPos = string.find(fullPath, "Make")
	    fullPath = string.sub(fullPath, 0, endPos - 1)
	    fullPath = string.gsub(fullPath, "/", "\\")
	
	    return fullPath
    end

    local AbsoluteRootDir = GetFullPath()
    local RootDir = "../../"
    local IncludeDir = RootDir ..  "ThirdParty/Include/"
    local SourceFolder = "Src/"
    local LibDir = RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}"
    local ThirdPartyLibDir = RootDir .. "ThirdParty/Lib/x64/"

    local ProjectAction = "UNDEFINED"
    */
}


public static class Main
{
    [Sharpmake.Main]
    public static void SharpmakeMain(Sharpmake.Arguments arguments)
    {
        KitsRootPaths.SetUseKitsRootForDevEnv(DevEnv.vs2017, KitsRootEnum.KitsRoot10, Options.Vc.General.WindowsTargetPlatformVersion.v10_0_18362_0);//10.0.18362.0
        arguments.Generate<BaseGameSolution>();
    }
}