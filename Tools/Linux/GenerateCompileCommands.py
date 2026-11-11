#!/usr/bin/env python3
"""Writes compile_commands.json at the repository root, for VS Code IntelliSense, clangd and other tools.

The compile commands come from the Sharpmake-generated Makefiles themselves: for each project RZE/Makefile compiles,
this runs that project's object goal as a dry run (make -n -B), so every flag, define and include path is the one the
real build uses, and nothing is compiled. Run it again after GenerateSolutions.sh, or when switching configuration.

The build force-includes a copy of each project's precompiled header from its obj folder (-include obj/.../XPCH.h),
which only exists after a build and has a .gch beside it. The entries here include the source header instead, so they
work before the first build and tools never try to read a .gch from another compiler version.

Usage:
  GenerateCompileCommands.py [--config debug|development|retail] [--output path]
"""

import argparse
import json
import os
import re
import shlex
import subprocess
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
RZE_DIR = os.path.join(REPO_ROOT, "RZE")
OBJECTS_MK = os.path.join(RZE_DIR, "Make", "Objects.mk")
COMPILERS = {"clang++", "clang", "g++", "gcc", "c++", "cc"}

# RZE/Makefile runs each project's objects as: $(MAKE) ... -C "$(RZE_DIR)/<dir>" -f "<Project>_Linux.make" -f ...
PROJECT_RECIPE = re.compile(r'-C "\$\(RZE_DIR\)/([^"]+)" -f "([^"]+)"')


def find_projects():
    with open(os.path.join(RZE_DIR, "Makefile"), encoding="utf-8") as makefile:
        projects = PROJECT_RECIPE.findall(makefile.read())
    if not projects:
        sys.exit("No project object goals found in RZE/Makefile. Run RZE/GenerateSolutions.sh first, or this script "
                 "needs updating to match ParallelMakefile.sharpmake.cs")
    return projects


def run_make(directory, makefile, config, extra_args):
    command = ["make", "--no-print-directory", "-C", directory, "-f", makefile, "-f", OBJECTS_MK,
               f"config={config}"] + extra_args
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit(f"{' '.join(command)} failed:\n{result.stderr}")
    return result.stdout


def read_pch(directory, makefile, config):
    """The project's precompiled header and the obj-folder copy the build force-includes, or None without one."""
    output = run_make(directory, makefile, config,
                      ["--eval", "rze-print-pch: ; @echo '$(PCH)|$(PCHOUT)'", "rze-print-pch"]).strip()
    pch, pch_out = output.split("|")
    return (pch, pch_out) if pch else None


def project_entries(directory, makefile, config):
    pch = read_pch(directory, makefile, config)
    entries = []
    for line in run_make(directory, makefile, config, ["-n", "-B", "rze-objects"]).splitlines():
        try:
            arguments = shlex.split(line)
        except ValueError:
            continue
        if not arguments or os.path.basename(arguments[0]) not in COMPILERS or "-c" not in arguments:
            continue
        if pch:
            arguments = [pch[0] if previous == "-include" and argument == pch[1] else argument
                         for previous, argument in zip([""] + arguments, arguments)]
        source = arguments[arguments.index("-c") + 1]
        entries.append({
            "directory": directory,
            "file": os.path.normpath(os.path.join(directory, source)),
            "arguments": arguments,
        })
    return entries


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--config", default="development", choices=["debug", "development", "retail"])
    parser.add_argument("--output", default=os.path.join(REPO_ROOT, "compile_commands.json"))
    args = parser.parse_args()

    entries = []
    for relative_dir, makefile in find_projects():
        entries += project_entries(os.path.join(RZE_DIR, relative_dir), makefile, args.config)
    entries.sort(key=lambda entry: entry["file"])

    temporary = args.output + ".tmp"
    with open(temporary, "w", encoding="utf-8") as output:
        json.dump(entries, output, indent=1)
        output.write("\n")
    os.replace(temporary, args.output)
    print(f"Wrote {len(entries)} entries ({args.config}) to {args.output}")


if __name__ == "__main__":
    main()
