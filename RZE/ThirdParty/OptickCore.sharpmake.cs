using Sharpmake;

// Optick's runtime, built from its source in Include/Optick (Optick 1.3.0, the same version as the prebuilt
// OptickCore.lib/.dll that Windows links). Only platforms without the prebuilt library use it (RZE.sharpmake.cs).
[Generate]
public class OptickCore : BaseProject
{
    public OptickCore()
        : base()
    {
        Name = "OptickCore";
        SourceRootPath = "Include/Optick";
    }

    public override void ConfigureAll(Project.Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.Output = Configuration.OutputType.Lib;

        // RZE profiles the CPU only (no OPTICK_GPU_* events), and Optick has no OpenGL GPU profiler
        conf.Defines.Add("OPTICK_ENABLE_GPU=0");
        // Someone else's code: its warnings would bury ours
        conf.AdditionalCompilerOptions.Add("-w");
    }
}
