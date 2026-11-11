**Setup on Windows:**

NOTE: YOU CURRENTLY NEED YOUR DEPOT IN **C:\dev\RZE** FOR RZEHUB TO WORK.

Once you have the repo locally:
1) Run Tools/RZEHub.exe
2) Open and build solution. Pick **Development** in Visual Studio's configuration list the first time (optimized, with
   asserts and symbols; Visual Studio remembers it). Debug is unoptimized, for stepping through code line by line
3) Click Burn Assets then Asset Copy in RZE Hub
5) Start Editor project or Game project (Game is default)

**Setup on Linux** (Fedora 44, with an OpenGL 4.5 driver):

1) Install the tools and libraries: `sudo dnf install clang lld make dotnet-sdk-8.0 SDL3-devel libshaderc-devel assimp-devel`
2) Generate the Makefiles: `cd RZE && ./GenerateSolutions.sh` (again whenever a source file is added or removed)
3) Build: `make -j$(nproc)`, which builds Development: optimized, with asserts and symbols. `config=debug` is
   unoptimized, for stepping through code line by line, and `config=retail` is the build that ships. It compiles every
   project at once, then links; `make -f RZE.make config=development` is the same build, one project after another
   (`RZE.make` on its own defaults to Debug)
4) Burn the assets: `./_build/development/SourceAssetBurner`
5) Start the Editor or the Game: `./_build/development/Editor` or `./_build/development/Game`, optionally with
   `-scene Sponza.scene`

To build, debug and get IntelliSense in VS Code, open `Tools/Linux/VSCode/RZE.code-workspace` (see the README there).



[![Codacy Badge](https://api.codacy.com/project/badge/Grade/1e7ff76592894b2799776314e66f1f4d)](https://app.codacy.com/manual/b1nary0/RZE?utm_source=github.com&utm_medium=referral&utm_content=b1nary0/RZE&utm_campaign=Badge_Grade_Dashboard)
[![Windows ](https://github.com/b1nary0/RZE/actions/workflows/Windows.yml/badge.svg?branch=look-what-claude-can-do)](https://github.com/b1nary0/RZE/actions/workflows/Windows.yml?query=branch%3Alook-what-claude-can-do)
[![Linux](https://github.com/b1nary0/RZE/actions/workflows/Linux.yml/badge.svg?branch=look-what-claude-can-do)](https://github.com/b1nary0/RZE/actions/workflows/Linux.yml?query=branch%3Alook-what-claude-can-do)