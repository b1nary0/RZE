using Sharpmake;
using static Sharpmake.Project;
using System.Xml.Linq;
using System.IO;

[Generate]
public class SharpmakeProjectBase : CSharpProject
{
    public SharpmakeProjectBase()
        : base(typeof(CommonTarget))
    {
        Name = "SharpmakeProject";
        SourceRootPath = @"../../";

        ProjectSchema = CSharpProjectSchema.NetFramework;
        string[] things = { ".xml", ".map", ".config", ".bat", ".txt", ".xsd", ".h.template", ".resx", ".cur" };
        NoneExtensions.Remove(things);
        SourceFilesExtensions = new Strings(".sharpmake.cs");

        ContentExtension.Add("GenerateSolution.bat", "macOS.yml", "Windows.yml", "UWP.yml");

        DependenciesCopyLocal = DependenciesCopyLocalTypes.None;
        AddTargets(CommonTarget.GetDefaultTargets());
    }

    [Configure]
    public virtual void ConfigureAll(Configuration conf, CommonTarget target)
    {
        conf.Output = Configuration.OutputType.DotNetClassLibrary;
        conf.ProjectFileName = @"[project.Name]_[target.Platform]";
        conf.SolutionFolder = "Tools";
        conf.ProjectPath = Path.Combine(Globals.RootDir, "_Project");

        conf.TargetPath = Path.Combine(Globals.RootDir, "_Build/Sharpmake/[target.Optimization]/");
        //conf.ProjectPath = @"[project.SharpmakeCsPath]/.tmp/project/[target.Framework]";
        CSharpProjectExtensions.AddAspNetReferences(conf);
        conf.ReferencesByPath.Add(Path.Combine(Globals.RootDir, @"../Tools/Sharpmake/Sharpmake.dll"));
        conf.ReferencesByPath.Add(Path.Combine(Globals.RootDir, @"../Tools/Sharpmake/Sharpmake.Generators.dll"));
    }
}

