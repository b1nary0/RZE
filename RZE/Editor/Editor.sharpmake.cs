using Sharpmake;
using System.IO;

// The editor's code, as a library that the Editor executable and the unit tests both link
[Generate]
public class EditorLib : BaseProject
{
    public EditorLib()
        : base()
    {
        Name = "EditorLib";
        SourceRootPath = "Src";
        // The executable's entry point, built by Editor
        SourceFilesExcludeRegex.Add(@"[\\/]EditorMain\.cpp$");
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.Output = Configuration.OutputType.Lib;
        // Editor's project is in the same folder, so each needs its own, and each configuration its own within that
        // (target.DirectoryName is the mode, the same for every configuration)
        conf.IntermediatePath = Path.Combine("[conf.ProjectPath]", "obj", "[project.Name]", "[target.Name]");
        conf.IncludePaths.Add(Path.Combine(Globals.RootDir, "Utils"));
        conf.AddPublicDependency<Engine>(target);
    }
}

[Generate]
public class Editor : BaseProject
{
    public Editor()
        : base()
    {
        Name = "Editor";
        SourceRootPath = "Src";
        // Everything else is in EditorLib
        SourceFilesFiltersRegex.Add(@"[\\/]EditorMain\.cpp$");
        // The icon. Outside Src, so the filter above doesn't drop it
        ResourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "Resources", "Editor.rc"));
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.IncludePaths.Add(Path.Combine(Globals.RootDir, "Utils"));
        conf.AddPublicDependency<EditorLib>(target);
        // Panels register themselves from static initializers (REGISTER_EDITOR_PANEL) that nothing references, so
        // the linker would drop them without this
        if (target.Platform == Platform.win64)
        {
            conf.AdditionalLinkerOptions.Add("/WHOLEARCHIVE:EditorLib.lib");
        }
        else
        {
            // Found through the build folder's -L, which comes first
            conf.AdditionalLinkerOptions.Add("-Wl,--whole-archive");
            conf.AdditionalLinkerOptions.Add("-l:libEditorLib.a");
            conf.AdditionalLinkerOptions.Add("-Wl,--no-whole-archive");
        }
        ConfigureAsWindowedApp(conf);
    }
}
