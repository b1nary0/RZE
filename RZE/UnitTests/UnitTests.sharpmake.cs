using Sharpmake;
using System.IO;

// Unit tests (doctest), run as UnitTests.exe from the build folder. Links EditorLib, and through it Engine, so tests
// can cover either.
[Generate]
public class UnitTests : BaseProject
{
    public UnitTests()
        : base()
    {
        Name = "UnitTests";
        SourceRootPath = "Src";
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.Output = Configuration.OutputType.Exe;
        conf.IncludePaths.Add(Path.Combine(Globals.RootDir, "Utils"));
        conf.IncludePaths.Add(Path.Combine(Globals.RootDir, "Editor", "Src"));
        conf.AddPublicDependency<EditorLib>(target);
    }
}
