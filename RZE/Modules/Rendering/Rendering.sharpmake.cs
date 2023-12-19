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

        conf.LibraryFiles.Add("OptickCore");
        conf.LibraryFiles.Add("d3d9");
        conf.LibraryFiles.Add("d3d11");
        conf.LibraryFiles.Add("D3d12");
        conf.LibraryFiles.Add("DXGI");
        conf.LibraryFiles.Add("D3DCompiler");
        conf.LibraryFiles.Add("DirectXTK");
    }
}