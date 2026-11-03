using Sharpmake;
using System.IO;

[Generate]
public class Externals : BaseProject
{
    public Externals()
        : base()
    {
        Name = "Externals";
        SourceRootPath = "Src";
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.Output = Configuration.OutputType.Lib;

        // SDL, the window and input on every platform (imgui_impl_sdl3 here; the engine's window). The DLL is copied
        // next to the executables with the rest (Engine.sharpmake.cs).
        if (target.Platform == Platform.win64)
        {
            conf.LibraryPaths.Add(Globals.ThirdPartyLibDir);
            conf.LibraryFiles.Add("SDL3");
        }
        else
        {
            conf.LibraryFiles.Add("libSDL3.so");

            // ImGui's Direct3D 11 renderer backend. Named rather than kept in a DX11/ folder (BaseProject), which would
            // mean editing ImGui's own files: they include "imgui.h" beside them.
            conf.SourceFilesBuildExcludeRegex.Add(@"[\\/]imgui[\\/]imgui_impl_dx11\.cpp$");
        }
    }
}