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

        // ImGui's Windows platform and Direct3D 11 renderer backends. Named one by one rather than kept in Windows/ and
        // DX11/ folders (BaseProject), which would mean editing ImGui's own files: they include "imgui.h" beside them.
        if (target.Platform != Platform.win64)
        {
            conf.SourceFilesBuildExcludeRegex.Add(@"[\\/]imgui[\\/]imgui_impl_win32\.cpp$");
            conf.SourceFilesBuildExcludeRegex.Add(@"[\\/]imgui[\\/]imgui_impl_dx11\.cpp$");
        }
    }
}