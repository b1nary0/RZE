using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using Sharpmake;

// Writes RZE/Makefile next to RZE.make (Linux only): every project's precompiled header and objects compile at once,
// then RZE.make archives and links in dependency order. RZE.make alone builds the projects one after another, because
// each project's target depends on its dependencies' whole targets, links included, while compiling needs only their
// headers. Phase 1 relies on that: nothing on Linux compiles against another project's build output. See "Building
// every project at once" in Docs/Generated/Engine/09-BuildSystem/Sharpmake-Project-Graph.md.
public static class ParallelMakefile
{
    private class ProjectInfo
    {
        public string Name;
        public string Folder;   // its _project folder, relative to RZE/
        public string Makefile; // e.g. Engine_Linux.make
        public List<string> Dependencies; // transitive, as Sharpmake resolves them
    }

    public static void Register()
    {
        Builder.Instance.EventPostGeneration += OnPostGeneration;
    }

    private static void OnPostGeneration(List<Project> projects, List<Solution> solutions)
    {
        foreach (Solution solution in solutions)
        {
            foreach (List<Solution.Configuration> configurations in solution.SolutionFilesMapping.Values)
            {
                // Windows' .sln has vs2026 configurations; only the Makefile generator's solution gets one
                if (configurations.Count == 0 || !configurations.All(conf => ((CommonTarget)conf.Target).DevEnv == DevEnv.make))
                {
                    continue;
                }
                Write(configurations);
            }
        }
    }

    private static void Write(List<Solution.Configuration> configurations)
    {
        // One Makefile serves every configuration, like RZE.make, so the projects and their dependencies must agree
        List<ProjectInfo> projects = Describe(configurations[0]);
        string expected = Summary(projects);
        foreach (Solution.Configuration conf in configurations.Skip(1))
        {
            if (Summary(Describe(conf)) != expected)
            {
                throw new Exception("ParallelMakefile: the projects or their dependencies differ between " + configurations[0].Name
                    + " and " + conf.Name + ", and RZE/Makefile has one list for every configuration");
            }
        }

        string path = Path.Combine(configurations[0].SolutionPath, "Makefile");
        // As make's config= takes them (Debug is debug), in the solution's order
        List<string> configNames = configurations.Select(conf => conf.Name.ToLowerInvariant()).ToList();
        // A plain make builds Development: optimized, with asserts and symbols. RZE.make and the project Makefiles keep
        // Sharpmake's default, the solution's first configuration (debug) and a fixed debug.
        string defaultConfig = CommonTarget.ConfigurationName(Optimization.Release).ToLowerInvariant();
        if (!configNames.Contains(defaultConfig))
        {
            throw new Exception("ParallelMakefile: the default configuration, " + defaultConfig + ", isn't one of the solution's ("
                + string.Join(", ", configNames) + ")");
        }
        string content = Content(projects, configNames, defaultConfig);
        // The hook runs on every generation, even when nothing changed: keep the file's date when its content is the same
        if (File.Exists(path) && File.ReadAllText(path) == content)
        {
            return;
        }
        File.WriteAllText(path, content);
        Console.WriteLine("  ParallelMakefile: wrote " + path);
    }

    private static List<ProjectInfo> Describe(Solution.Configuration solutionConf)
    {
        return solutionConf.IncludedProjectInfos
            .Select(info => new ProjectInfo
            {
                Name = info.Project.Name,
                // Sharpmake's own, not Path.GetRelativePath: the Visual Studio project for these scripts targets .NET
                // Framework 4.8, which doesn't have it
                Folder = Util.PathGetRelative(solutionConf.SolutionPath, info.Configuration.ProjectPath).Replace('\\', '/'),
                Makefile = Path.GetFileName(info.Configuration.ProjectFullFileNameWithExtension),
                Dependencies = info.Configuration.ResolvedDependencies.Select(dep => dep.Project.Name).Distinct().OrderBy(name => name, StringComparer.Ordinal).ToList(),
            })
            .OrderBy(project => project.Name, StringComparer.Ordinal)
            .ToList();
    }

    private static string Summary(List<ProjectInfo> projects)
    {
        return string.Join("\n", projects.Select(p => p.Name + "|" + p.Folder + "|" + p.Makefile + "|" + string.Join(" ", p.Dependencies)));
    }

    private static string Content(List<ProjectInfo> projects, List<string> configNames, string defaultConfig)
    {
        StringBuilder s = new StringBuilder();
        s.Append("# Generated by Sharpmake (Make/Sharpmake/ParallelMakefile.sharpmake.cs) -- Do not edit !\n");
        s.Append("# Phase 1 compiles every project's precompiled header and objects at once (Make/Objects.mk), phase 2 runs\n");
        s.Append("# RZE.make, which only archives and links. RZE.make on its own is the serial build. Type \"make help\" for usage.\n");
        s.Append("\n");
        s.Append("ifndef config\n");
        s.Append("  config=" + defaultConfig + "\n");
        s.Append("endif\n");
        s.Append("export config\n");
        s.Append("\n");
        s.Append("# This folder, so that make -f RZE/Makefile works from elsewhere too\n");
        s.Append("RZE_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))\n");
        s.Append("OBJECTS_MK := $(RZE_DIR)/Make/Objects.mk\n");
        s.Append("\n");
        s.Append("PROJECTS := " + string.Join(" ", projects.Select(p => p.Name)) + "\n");
        s.Append("OBJECT_GOALS := $(addprefix objects-,$(PROJECTS))\n");
        s.Append("\n");
        s.Append(".PHONY: all clean help $(PROJECTS) $(OBJECT_GOALS)\n");
        s.Append("\n");
        s.Append("all: $(OBJECT_GOALS)\n");
        s.Append("\t@$(MAKE) --no-print-directory -C \"$(RZE_DIR)\" -f RZE.make\n");
        s.Append("\n");
        s.Append("# One project: its own and its dependencies' objects, then RZE.make links it and what it depends on\n");
        foreach (ProjectInfo p in projects)
        {
            s.Append(p.Name + ":" + string.Concat(new[] { p.Name }.Concat(p.Dependencies).Select(name => " objects-" + name)) + "\n");
            s.Append("\t@$(MAKE) --no-print-directory -C \"$(RZE_DIR)\" -f RZE.make " + p.Name + "\n");
        }
        s.Append("\n");
        s.Append("# Phase 1: a project's PCH and objects, from its own Makefile with Make/Objects.mk read after it\n");
        foreach (ProjectInfo p in projects)
        {
            s.Append("objects-" + p.Name + ":\n");
            s.Append("\t@echo \" ==== Compiling " + p.Name + " ($(config)) ====\"\n");
            s.Append("\t@$(MAKE) --no-print-directory -C \"$(RZE_DIR)/" + p.Folder + "\" -f \"" + p.Makefile + "\" -f \"$(OBJECTS_MK)\" rze-objects\n");
        }
        s.Append("\n");
        s.Append("clean:\n");
        s.Append("\t@$(MAKE) --no-print-directory -C \"$(RZE_DIR)\" -f RZE.make $@\n");
        s.Append("\n");
        // Its own, not RZE.make's: Sharpmake's lists the Optimization values (release), not the configurations' names
        // (development, CommonTarget.ConfigurationName)
        s.Append("help:\n");
        s.Append("\t@echo \"Usage: make [config=name] [target]\"\n");
        s.Append("\t@echo \"\"\n");
        s.Append("\t@echo \"CONFIGURATIONS:\"\n");
        foreach (string configName in configNames)
        {
            s.Append("\t@echo \"   " + configName + (configName == defaultConfig ? " (default)" : "") + "\"\n");
        }
        s.Append("\t@echo \"\"\n");
        s.Append("\t@echo \"TARGETS:\"\n");
        s.Append("\t@echo \"   all (default)\"\n");
        s.Append("\t@echo \"   clean\"\n");
        foreach (ProjectInfo p in projects)
        {
            s.Append("\t@echo \"   " + p.Name + "\"\n");
        }
        return s.ToString();
    }
}
