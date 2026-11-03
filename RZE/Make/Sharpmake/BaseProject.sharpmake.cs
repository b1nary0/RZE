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
        // Lowercase, so they're the same folders on case-sensitive filesystems (Linux) as everywhere else that names
        // them: committed files in _project (Editor.args.json and others), and _build in .gitignore, CI and Filepath
        conf.ProjectPath = Path.Combine("[project.SharpmakeCsPath]", "_project");
        conf.ProjectFileName = @"[project.Name]_[target.Platform]";

        conf.TargetPath = Globals.BuildDir(target);
        conf.LibraryPaths.Add(Globals.BuildDir(target));

        // It seems like you use <> includes a lot, so this is needed per project
        conf.IncludePaths.Add(Path.Combine("[project.SharpmakeCsPath]", "Src"));
        if (target.Platform == Platform.linux)
        {
            // Someone else's headers, as system headers, so their warnings don't fail the build (-Werror) where ours
            // include them. The Makefile generator leaves out IncludeSystemPaths, so it's passed as an option, relative
            // to the project's folder as the generator writes -I paths.
            string projectFolder = Path.Combine(SharpmakeCsPath, "_project");
            conf.AdditionalCompilerOptions.Add("-isystem " + Util.PathGetRelative(projectFolder, Path.GetFullPath(Globals.IncludeDir)));
        }
        else
        {
            conf.IncludePaths.Add(Globals.IncludeDir);
        }
    }

    // For an exe with a window: no console window opens with it, which on Windows 11 opens in Windows Terminal and
    // takes focus from the app's window (-console opens one). The entry point stays main().
    // Other platforms have no console/windowed split, so it does nothing there.
    protected static void ConfigureAsWindowedApp(Project.Configuration conf)
    {
        if (conf.Platform != Platform.win64)
        {
            return;
        }

        conf.Options.Add(Options.Vc.Linker.SubSystem.Windows);
        conf.AdditionalLinkerOptions.Add("/ENTRY:mainCRTStartup");
    }

    // Folders that hold code for only some platforms or graphics APIs. A folder with one of these names, at any depth
    // under a project's Src, is compiled only where it's listed, so per-platform code needs no #ifdefs. E.g.
    // Utils/Platform/Windows/ShellWindows.cpp and Utils/Platform/Linux/ShellLinux.cpp implement Utils/Platform/Shell.h.
    private static readonly string[] k_windowsOnlyFolders = { "Windows", "DX11" };
    private static readonly string[] k_linuxOnlyFolders = { "Linux", "OpenGL" };

    private static void ExcludeFolders(Configuration conf, string[] folderNames)
    {
        foreach (string folderName in folderNames)
        {
            // Only below Src, so a folder of that name further up (where the repository was cloned) doesn't exclude
            // everything
            conf.SourceFilesBuildExcludeRegex.Add(@"[\\/]Src[\\/](?:[^\\/]+[\\/])*" + folderName + @"[\\/]");
        }
    }

    #region Platfoms

    [ConfigurePriority(ConfigurePriorities.Platform)]
    [Configure(Platform.win64)]
    public virtual void ConfigureWin64(Configuration conf, CommonTarget target)
    {
        conf.Options.Add(Options.Vc.Compiler.CppLanguageStandard.CPP17);
        conf.Options.Add(Options.Vc.Compiler.RTTI.Enable);
        conf.Options.Add(Options.Vc.General.CharacterSet.Unicode);
        conf.Options.Add(Options.Vc.Compiler.Exceptions.Enable);

        conf.Options.Add(Options.Vc.General.WarningLevel.Level3);
        conf.Options.Add(Options.Vc.General.TreatWarningsAsErrors.Enable);

        conf.Options.Add(
            new Options.Vc.Compiler.DisableSpecificWarnings(
                "4267"
                )
        );

        conf.Defines.Add("NOMINMAX");
        // The graphics API the renderer is built for (Rendering/Driver/GFXDeviceFactory)
        conf.Defines.Add("RZE_GFX_DX11");

        ExcludeFolders(conf, k_linuxOnlyFolders);
    }

    [ConfigurePriority(ConfigurePriorities.Platform)]
    [Configure(Platform.linux)]
    public virtual void ConfigureLinux(Configuration conf, CommonTarget target)
    {
        conf.Options.Add(Options.Makefile.General.PlatformToolset.Clang);
        conf.Options.Add(Options.Makefile.Compiler.CppLanguageStandard.Cpp17);
        conf.Options.Add(Options.Makefile.Compiler.Rtti.Enable);
        conf.Options.Add(Options.Makefile.Compiler.Exceptions.Enable);

        // -Wall, as errors, as Windows' /W3 /WX are
        conf.Options.Add(Options.Makefile.Compiler.Warnings.MoreWarnings);
        conf.Options.Add(Options.Makefile.Compiler.TreatWarningsAsErrors.Enable);
        // Except the PCHs' "Compiling ... PCH" #pragma message, which MSVC prints and clang reports as a warning. The
        // # is escaped for make, where it would start a comment.
        conf.AdditionalCompilerOptions.Add(@"-Wno-error=\#pragma-messages");
        // Symbols in every configuration, as Windows has PDBs in every configuration
        conf.Options.Add(Options.Makefile.Compiler.GenerateDebugInformation.Enable);

        switch (target.Optimization)
        {
            case Optimization.Debug:
                conf.Options.Add(Options.Makefile.Compiler.OptimizationLevel.Disable);
                break;
            case Optimization.Release:
                conf.Options.Add(Options.Makefile.Compiler.OptimizationLevel.Full);
                break;
            case Optimization.Retail:
                conf.Options.Add(Options.Makefile.Compiler.OptimizationLevel.FullWithInlining);
                // As Windows' Retail (AdvancedVectorExtensions2)
                conf.AdditionalCompilerOptions.Add("-mavx2");
                break;
        }

        conf.AdditionalCompilerOptions.Add("-pthread");
        conf.AdditionalLinkerOptions.Add("-pthread");
        // LLD finds symbols in static libraries whatever their order, as MSVC's linker does. GNU ld needs them in
        // dependency order, but the Makefile generator sorts them by name, and its --start-group option is passed to
        // clang++ without -Wl, so clang++ rejects it.
        conf.AdditionalLinkerOptions.Add("-fuse-ld=lld");

        // The graphics API the renderer is built for (Rendering/Driver/GFXDeviceFactory)
        conf.Defines.Add("RZE_GFX_OPENGL");

        ExcludeFolders(conf, k_windowsOnlyFolders);

        // Editor, Game, UnitTests rather than the generator's default Editor.elf
        if (conf.Output == Configuration.OutputType.Exe)
        {
            conf.TargetFileFullExtension = "";
        }
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

        conf.Defines.Add("RZE_RETAIL");
    }

    #endregion
}