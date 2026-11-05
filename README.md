**Setup on Windows:**

NOTE: YOU CURRENTLY NEED YOUR DEPOT IN **C:\dev\RZE** FOR RZEHUB TO WORK.

Once you have the repo locally:
1) Run Tools/RZEHub.exe
2) Open and build solution
3) Click Burn Assets then Asset Copy in RZE Hub
5) Start Editor project or Game project (Game is default)

**Setup on Linux** (Fedora 44, with an OpenGL 4.5 driver):

1) Install the tools and libraries: `sudo dnf install clang lld make dotnet-sdk-8.0 SDL3-devel libshaderc-devel assimp-devel`
2) Generate the Makefiles: `cd RZE && ./GenerateSolutions.sh` (again whenever a source file is added or removed)
3) Build: `make config=debug -j$(nproc)` (or `release`, `retail`). It compiles every project at once, then links;
   `make -f RZE.make` is the same build, one project after another
4) Burn the assets: `./_build/debug/SourceAssetBurner`
5) Start the Editor or the Game: `./_build/debug/Editor` or `./_build/debug/Game`, optionally with `-scene Sponza.scene`



[![Codacy Badge](https://api.codacy.com/project/badge/Grade/1e7ff76592894b2799776314e66f1f4d)](https://app.codacy.com/manual/b1nary0/RZE?utm_source=github.com&utm_medium=referral&utm_content=b1nary0/RZE&utm_campaign=Badge_Grade_Dashboard)
[![Windows ](https://github.com/b1nary0/RZE/actions/workflows/Windows.yml/badge.svg)](https://github.com/b1nary0/RZE/actions/workflows/Windows.yml)
[![Linux](https://github.com/b1nary0/RZE/actions/workflows/Linux.yml/badge.svg)](https://github.com/b1nary0/RZE/actions/workflows/Linux.yml)