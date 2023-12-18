using Sharpmake;
using static Sharpmake.Project;
using System.Xml.Linq;

[Generate]
public class SharpmakeProjectBase : CSharpProject
{
    public SharpmakeProjectBase()
        : base(typeof(CommonTarget))
    {
        Name = "SharpmakeProject";
        SourceRootPath = @"./";

        ProjectSchema = CSharpProjectSchema.NetFramework;
        string[] things = { ".xml", ".map", ".config", ".bat", ".txt", ".xsd", ".h.template", ".resx", ".cur" };
        NoneExtensions.Remove(things);
        SourceFilesExtensions = new Strings(".sharpmake.cs");

        ContentExtension.Add("GenerateSolution.bat", "macOS.yml", "Windows.yml", "UWP.yml");

        SourceFiles.Add(@"[project.SharpmakeCsPath]/RZE.sharpmake.cs");
        SourceFiles.Add(@"[project.SharpmakeCsPath]/BaseProject.sharpmake.cs");
        SourceFiles.Add(@"[project.SharpmakeCsPath]/CommonTarget.sharpmake.cs");
        SourceFiles.Add(@"[project.SharpmakeCsPath]/Sharpmake.sharpmake.cs");
        SourceFiles.Add(@"[project.SharpmakeCsPath]/Engine.sharpmake.cs");
        SourceFiles.Add(@"[project.SharpmakeCsPath]/SourceAssetBurner/SourceAssetBurner.sharpmake.cs");
        //SourceFilesCompileExtensions.Clear();
        //SourceFilesCompileExtensions.Add(".cs");
        DependenciesCopyLocal = DependenciesCopyLocalTypes.None;
        AddTargets(CommonTarget.GetDefaultTargets());
        //SourceFilesBuildExclude.Add("Game.sharpmake.cs");

        //PublicResourceFiles.Clear();
        //ContentExtension.Remove(contentExtension);
        //ResourceFilesExtensions.Remove(contentExtension);
        //EmbeddedResourceExtensions.Remove(contentExtension);
    }

    [Configure]
    public virtual void ConfigureAll(Configuration conf, CommonTarget target)
    {
        conf.Output = Configuration.OutputType.DotNetClassLibrary;
        conf.ProjectFileName = @"[project.Name]_[target.Platform]";
        conf.SolutionFolder = "Tools";

        conf.TargetPath = "$(SolutionDir).build/Sharpmake/[target.Optimization]/";
        conf.ProjectPath = @"[project.SharpmakeCsPath]/.tmp/project/[target.Framework]";
        CSharpProjectExtensions.AddAspNetReferences(conf);
        conf.ReferencesByPath.Add(@"[project.SharpmakeCsPath]/../Tools/Sharpmake/Sharpmake.dll");
        conf.ReferencesByPath.Add(@"[project.SharpmakeCsPath]/../Tools/Sharpmake/Sharpmake.Generators.dll");
    }
}

