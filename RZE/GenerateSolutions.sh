#!/usr/bin/env bash
# Generates the Linux Makefiles: RZE.make, and one .make per project in its _project folder. Run it again after adding
# or removing a source file. Then build with: make -f RZE.make config=debug (or release, retail)
# Needs .NET 8 or later (the bundled Sharpmake targets .NET 6 and runs on newer runtimes).
set -euo pipefail
cd "$(dirname "$0")"
exec dotnet ../Tools/Sharpmake/Sharpmake.Application.dll "/sources('RZE.sharpmake.cs')"
