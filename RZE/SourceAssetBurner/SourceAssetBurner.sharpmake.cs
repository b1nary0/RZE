using Sharpmake;
using System.IO;

[Generate]
public class SourceAssetBurner : BaseProject
{
    public SourceAssetBurner()
        : base()
    {
        Name = "SourceAssetBurner";
        SourceRootPath = "Src";
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.IncludePaths.Add(Path.Combine(Globals.RootDir, "Utils"));
        conf.AddPublicDependency<Engine>(target);

        if (target.Platform == Platform.win64)
        {
            conf.LibraryFiles.Add("assimp-vc141-mt");
        }
        else
        {
            // The system's (assimp-devel). A name ending in .so is linked as is; the Makefile generator would make
            // a bare name into lib<name>.a, a static library.
            conf.LibraryFiles.Add("libassimp.so");
        }
    }
}