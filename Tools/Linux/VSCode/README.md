# VS Code on Linux

Open `RZE.code-workspace`, not the repository folder:

```bash
code Tools/Linux/VSCode/RZE.code-workspace
```

VS Code lists it under File > Open Recent afterwards. When it asks, install the recommended C/C++ extension
(`ms-vscode.cpptools`).

## Setup (once)

1. Install the build dependencies from the main README, and `gdb` for debugging.
2. Run the **RZE: generate Makefiles** task (Terminal > Run Task), which runs `RZE/GenerateSolutions.sh`.
3. Build with Ctrl+Shift+B, then run the **RZE: burn assets** task.

## What's in it

| | |
|---|---|
| Ctrl+Shift+B | **RZE: build development**: `make -C RZE config=development`, optimized with asserts and symbols, after regenerating `compile_commands.json`. Errors open in the Problems panel. |
| F5 | **Editor** (asks which scene to open) or **UnitTests** (asks which test cases), with gdb, in Development or Debug. Each builds its configuration first. Debug is unoptimized, for stepping through code the optimizer rearranges. |
| Tasks | **build debug**, **unit tests** and **burn assets** (Development), **generate Makefiles** (again after adding or removing a source file), **compile_commands.json**. |
| IntelliSense | Reads `compile_commands.json` at the repository root, written by `Tools/Linux/GenerateCompileCommands.py` from the Makefiles, so it uses the build's own flags. The file is gitignored. |

Settings of your own go in your VS Code user settings, or in the repository's `.vscode/`, which is untracked.
A `.vscode/c_cpp_properties.json` there replaces this workspace's IntelliSense settings.
