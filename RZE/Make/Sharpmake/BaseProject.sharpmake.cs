using Sharpmake;
using System.IO;
using System.Xml.Linq;
using static CommonTarget;

public abstract class BaseProject : Project
{
    public BaseProject()
        : base(typeof(CommonTarget))
    {
        Name = "BaseProject";
        SourceRootPath = @"Source";
        IsFileNameToLower = false;
        IsTargetFileNameToLower = false;
        AddTargets(CommonTarget.GetDefaultTargets());
    }

    public static class ConfigurePriorities
    {
        public const int All = -75;
        public const int Platform = -50;
        public const int Optimization = -25;
        /*     SHARPMAKE DEFAULT IS 0     */
        public const int Blobbing = 10;
        public const int BuildSystem = 30;
    }

    [ConfigurePriority(ConfigurePriorities.All)]
    [Configure]
    public virtual void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        conf.ProjectPath = Path.Combine("[project.SharpmakeCsPath]", "_Project");
        conf.ProjectFileName = @"[project.Name]_[target.Platform]";

        conf.TargetPath = Path.Combine(Globals.RootDir, "_Build/[target.Name]/");
        conf.LibraryPaths.Add(Path.Combine(Globals.RootDir, "_Build/[target.Name]/"));

        // It seems like you use <> includes a lot, so this is needed per project
        conf.IncludePaths.Add(Path.Combine("[project.SharpmakeCsPath]", "Src"));
        conf.IncludePaths.Add(Globals.IncludeDir);
    }

    #region Platfoms

    [ConfigurePriority(ConfigurePriorities.Platform)]
    [Configure(SubPlatformType.x64)]
    public virtual void ConfigureWin64(Configuration conf, CommonTarget target)
    {
        conf.Options.Add(Options.Vc.Compiler.CppLanguageStandard.CPP17);
        conf.Options.Add(Options.Vc.Compiler.RTTI.Enable);
        conf.Options.Add(Options.Vc.General.CharacterSet.Unicode);
        conf.Options.Add(Options.Vc.Compiler.Exceptions.Enable);
        //conf.Options.Add(Options.Vc.General.TreatWarningsAsErrors.Enable);

        conf.Options.Add(
            new Options.Vc.Compiler.DisableSpecificWarnings(
                "4267"
                )
        );

        conf.Defines.Add("NOMINMAX");
    }

    #endregion

    #region Optimizations

    [ConfigurePriority(ConfigurePriorities.Optimization)]
    [Configure(Optimization.Debug)]
    public virtual void ConfigureDebug(Configuration conf, CommonTarget target)
    {
        conf.DefaultOption = Options.DefaultTarget.Debug;

        conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDebugDLL);
        conf.Options.Add(Options.Vc.Compiler.Inline.Disable);
    }

    [ConfigurePriority(ConfigurePriorities.Optimization)]
    [Configure(Optimization.Release)]
    public virtual void ConfigureRelease(Configuration conf, CommonTarget target)
    {
        conf.DefaultOption = Options.DefaultTarget.Release;
        conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDLL);
        conf.Options.Add(Options.Vc.Compiler.Inline.OnlyInline);
    }


    [ConfigurePriority(ConfigurePriorities.Optimization)]
    [Configure(Optimization.Retail)]
    public virtual void ConfigureRetail(Configuration conf, CommonTarget target)
    {
        conf.DefaultOption = Options.DefaultTarget.Release;

        conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDLL);

        // Full inlining
        conf.Options.Add(Options.Vc.Compiler.Inline.AnySuitable);
        conf.Options.Add(Options.Vc.Compiler.Optimization.MaximizeSpeed);

        conf.Options.Add(Options.Vc.Compiler.Exceptions.EnableWithSEH);
        conf.Options.Add(Options.Vc.Compiler.EnhancedInstructionSet.AdvancedVectorExtensions2);
    }

    #endregion
}