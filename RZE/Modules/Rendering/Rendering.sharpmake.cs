using Sharpmake;
using System.IO;

[Generate]
public class Rendering : BaseProject
{
    public Rendering()
        : base()
    {
        Name = "Rendering";
        SourceRootPath = "Src";
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.Output = Configuration.OutputType.Lib;
        conf.AddPublicDependency<Externals>(target);
        conf.AddPublicDependency<Utils>(target);

        if (target.Platform == Platform.win64)
        {
            conf.LibraryFiles.Add("OptickCore");
            conf.LibraryFiles.Add("d3d9");
            conf.LibraryFiles.Add("d3d11");
            conf.LibraryFiles.Add("D3d12");
            conf.LibraryFiles.Add("DXGI");
            conf.LibraryFiles.Add("D3DCompiler");
            conf.LibraryFiles.Add("DirectXTK");
        }
        else
        {
            // Built from its source in ThirdParty (OptickCore.sharpmake.cs)
            conf.AddPublicDependency<OptickCore>(target);
            // HLSL to SPIR-V for the OpenGL renderer's shaders (Driver/OpenGL/GLShaderTranslator)
            conf.LibraryFiles.Add("libshaderc_shared.so");
        }
    }
}