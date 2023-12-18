using Sharpmake;
using System.Collections.Generic;
using System;

public class CommonTarget : Sharpmake.ITarget
{
    public Platform Platform;
    public DevEnv DevEnv;
    public Optimization Optimization;
    public DotNetFramework Framework;
    public DotNetOS DotNetOS;

    public static Optimization GetThirdPartyOptimization(Optimization opt)
    {
        switch (opt)
        {
            case Optimization.Debug:
                return Optimization.Debug;
            case Optimization.Release:
                return Optimization.Release;
            case Optimization.Retail:
                return Optimization.Release;
            default:
                return Optimization.Release;
        }
    }

    [Fragment, Flags]
    public enum SubPlatformType
    {
        x64 = 1 << 0,
        //macOS = 1 << 1,
        //UWP = 1 << 2,
    }
    public SubPlatformType SubPlatform = SubPlatformType.x64;

    [Fragment, Flags]
    public enum Mode
    {
        Game = 1 << 0,
        //Editor = 1 << 1
    }
    public Mode SelectedMode = Mode.Game;

    public CommonTarget() { }

    public CommonTarget(
        Platform platform,
        DevEnv devEnv,
        Optimization optimization,
        DotNetFramework dotNetFramework,
        DotNetOS dotNetOS
    )
    {
        Platform = platform;
        DevEnv = devEnv;
        Optimization = optimization;
        Framework = dotNetFramework;
        DotNetOS = dotNetOS;
    }

    public static CommonTarget[] GetDefaultTargets()
    {
        switch (Util.GetExecutingPlatform())
        {
            case Platform.win64:
                {
                    var baseTarget = new CommonTarget(
                        Platform.win64,
                        DevEnv.vs2017,
                        Optimization.Debug | Optimization.Release | Optimization.Retail,
                        DotNetFramework.v4_8,
                        dotNetOS: 0);
                    baseTarget.SubPlatform = SubPlatformType.x64;

                    return new[] { baseTarget };
                }
            default:
                {
                    throw new NotImplementedException("The platform (" + Util.GetExecutingPlatform() + ") is not currently supported!");
                }
        }
    }

    public override string Name
    {
        get
        {
            var nameParts = new List<string>
            {
                Optimization.ToString(),
            };
            return string.Join("_", nameParts);
        }
    }

    public string DirectoryName
    {
        get
        {
            var dirNameParts = new List<string>()
            {
                SelectedMode.ToString(),
            };

            return string.Join("_", dirNameParts);
        }
    }

    public ITarget ToDefaultDotNetOSTarget()
    {
        return ToSpecificDotNetOSTarget(DotNetOS.Default);
    }

    public ITarget ToSpecificDotNetOSTarget(DotNetOS dotNetOS)
    {
        if (DotNetOS == 0 || DotNetOS == dotNetOS)
        {
            return this;
        }

        return Clone(dotNetOS);
    }
}
