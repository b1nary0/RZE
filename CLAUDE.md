# RZE

C++17 engine with a Dear ImGui editor (RZEStudio): DX11 on Windows, OpenGL on Linux. Building, running, testing and
driving the editor are covered by the `run-rze` skill (`.claude/skills/run-rze/`).

## Both platforms must build

- Every change must keep **Windows and Linux** building and their unit tests passing. CI runs both
  (`.github/workflows/Windows.yml`, `Linux.yml`). If you can only build one platform on the machine you're on, say so
  and name what the other needs checked.
- Warnings are errors on both: MSVC, and clang with `-Wall -Werror` on Linux.
- Asserts (`AssertExpr`, `AssertMsg`, ...) compile to nothing outside `_DEBUG`, so a variable only an assert reads is
  unused in Release, which clang rejects. Put the whole check inside the assert (e.g. `std::all_of` with a lambda)
  instead of marking it.
- **Never write `[[maybe_unused]]` on a range-for variable** (`for ([[maybe_unused]] const T& x : xs)`): MSVC 14.51
  crashes on it with C1001.
- File names: Windows ignores case and Linux doesn't. Name files in code and data exactly as they are on disk. To accept
  names written on Windows, use `FileNames::FindIgnoringCase` / `IsTaken`.

## Code conventions

- **No function bodies in headers**, except simple returns (getters) and trivial setters. Constructors with work,
  loops, forwarding with logic and helpers go in a .cpp, and so do "header-only for testability" modules: unit tests
  link Engine.lib, so testable logic can live in an Engine .cpp.
- **Sink parameters:** never take a parameter by value and `std::move` it. Handed-over containers, handles and
  move-only types are `T&&`. A `std::shared_ptr` or a string the caller keeps is `const T&` and copied.
- **No `auto` for plain types** such as an index or a value a helper returns: write the type out. If it's long, alias it
  next to what produces it (e.g. `LogChannelIndex`). Iterator and lambda types, and `auto&` in range-for, are fine.
- **Avoid `friend`.** Find an ownership-based design first: the class that owns the data does the work, it exposes a
  narrow resolved view, or the data is split so the reader owns what it reads. If `friend` still seems right, ask
  first and explain why the alternatives don't fit. Existing uses aren't a precedent.
- **Transient state on demand:** state that only exists during an operation (a rename, a prompt, a pick in flight)
  goes in one struct behind `std::unique_ptr`, created when it starts and reset when it ends. Define the struct in
  the .cpp, which moves the owner's destructor there too. Use `std::optional` only for about pointer-sized data.
- **Put code where it belongs**, not next to its first caller. Ask what it depends on and who else could use it: pure
  maths in `Utils/Math`, asset-kind knowledge in `AssetPaths`, platform calls in `Utils/Platform`, editor-only
  behaviour in the Editor. Take the lowest module whose *subject* it is, not the lowest it compiles in (`CubeMap`
  encodes D3D's cube layout, so it's in `Engine/Graphics`). Search for an existing home first. Before writing a helper
  on a math type, read that type's header. Say where new code goes and why when proposing it.
- **glm only in `Utils/Src/Utils/Math`.** Nothing else uses `glm::`, includes `GLM/` or calls
  `GetInternalVec/Mat/Quat()`. If something is missing, add it to the math classes, then call that. For float-pointer
  APIs (ImGui, ImGuizmo) use `GetValuePtr()`. The math classes only get strictly mathematical helpers.
- **Only components implement `DescribeProperties`.** Nested types they show (e.g. `MaterialSlot`) are described by the
  owning component. `MaterialInstance::DescribeProperties` and `Texture2D::DescribeProperties` are existing violations,
  to be removed once the property API enforces this. Reuse them meanwhile, but add no new ones.
- **Shader buffer layouts:** layouts shared between render stages, or for a general concept like lighting, go in
  `Engine/Src/Graphics/GraphicsLayouts.h` (`namespace GraphicsLayouts`). A layout tied to one stage's behaviour stays
  nested in that stage. Don't change the Rendering module for this without asking.
- **Editor UI corners are rounded** with `EditorStyle::k_cornerRounding` (`UI/Framework/EditorStyle.h`):
  - use `EditorUI::TreeNode`, not `ImGui::TreeNodeEx`, which draws its highlight square;
  - hand-drawn highlights pass `k_cornerRounding` themselves;
  - after an ImGui upgrade, check tree-node highlights (`EditorUI::TreeNode` uses ImGui internals) in the Scene panel,
    the Content Browser tree, inspector arrays and Task History.

## Repository

- **Generated projects aren't committed.** Sharpmake writes the `.sln`/`.vcxproj` (`GenerateSolutions.bat`) and the
  Makefiles (`GenerateSolutions.sh`). Only `.sharpmake.cs` changes are committed. Rerun the generator after adding or
  removing a source file.
- **Upgrading a vendored library:** put a zip of the new binaries in `Files/`, named like the old one (e.g.
  `assimp-<ver>-win64-<toolset>-binaries.zip`), and delete the old zip.
- **Specs in `Docs/Plans/` aren't committed** (gitignored). Write them there and leave them out of commits. The pages
  in `Docs/Generated/Engine` are committed.
- **Logs:** every Editor/Game run writes `RZE/Logs/RZELOG_<dd-mm-yyyy>_<hh-mm-ss>.txt`. For temporary instrumentation,
  use `RZE_LOG_ARGS` and read the newest file. Don't add ad-hoc file logging.
- **`RZE/Config/imgui.ini` is rewritten by every editor run** and often holds an intended, uncommitted layout. Back it
  up before a test launch and restore from that backup, not from git. `RZE/Editor/_project/Editor.args.json` also
  holds uncommitted edits.

## Git

- **Never `git stash`** (push, pop or apply). Old stashes in this repo are kept on purpose. A no-op push followed by a
  pop once applied one of them. To set work aside, copy the files somewhere temporary.
- **Scope checkouts and restores to exact files.** Never `git checkout <rev> -- <dir>`: it overwrites uncommitted files
  inside it (imgui.ini, Editor.args.json). Run `git status --short` before any checkout or restore.
- After restoring a file by copying a backup over it, update its timestamp (`touch`) before building. The copy keeps
  the old mtime, so an incremental build skips it and the binary keeps the temporary code.
- Stage only the files a commit is about. Never stage `imgui.ini`, `Editor.args.json` or `Docs/Plans/`.

## Working together

- **One item at a time.** After each item: build, verify, and report results (with numbers when relevant). Then give
  that item's commit subject and description, ending with the Co-Authored-By line, and wait. **Never commit without
  being told to.** "commit" or "committed" in reply means make the commit yourself, then check `git log -1`. Once it's
  committed, ask to start the next item, with that item's plan in implementation order in the same message.
- **Docs:** after each finished task, ask whether to update `Docs/Generated/Engine`, naming the pages likely affected.
  In a multi-task plan, don't ask per task: do all the docs when the last implementation task is committed.
- **Push back** on an idea (the user's or your own earlier one) when there's a decision-relevant case against it: no
  consumer yet, cost out of proportion, a hidden constraint, a simpler option. Back it with numbers or code
  references, give a recommendation, and let the user decide. Say nothing when nothing would change the decision. Ask
  about the engine's intended use when it decides between options.
- **Performance:** after perf-relevant work, ask whether to A/B test and what; don't run it unasked. Use
  `Sponza.scene` (`RenderPerfTest.scene` for probe and shadow performance). A = the commit before, built in a separate
  worktree with the `-perfcapture <file> -perfwarmup <s> -perfduration <s>` flags. Alternate A and B runs, several per
  side, in the same build configuration and `imgui.ini` layout. Report tables with a % delta column (mean, median, p95,
  p99, spread), and say plainly when a difference is within noise.

## Docs diagrams (Mermaid)

`Docs/Generated/Engine` is read in Obsidian, which ships **Mermaid 11.13.0**. Failures are silent in the source, so
render and look.
- Start every flowchart, graph, state and sequence diagram with the shared init line. Copy it from an existing doc. It
  sets subgraph title margins and the label backgrounds and halos that keep lines from showing through labels. Don't
  add fallback colours.
- Sequence diagrams: no `;` anywhere in text, no keyword participant aliases (`loop`, `alt`, `end`), notes spanning two
  participants (`Note over A,B:`), short participant names.
- Gantt: `dateFormat x`, `axisFormat %L`, tasks as `id, start, 4ms` or `id, after other, 4ms`.
- Prefer `flowchart TD` to long LR chains; use `direction TB` inside side-by-side subgraphs. Avoid several edges
  between the same two nodes and self-links in class diagrams. Keep quadrant points away from y≈0.5 and the titles.
- Before committing, render every block with the Mermaid CLI pinned to Mermaid 11.13.0
  (`@mermaid-js/mermaid-cli` 11.12.0 with `"overrides": { "mermaid": "11.13.0" }`), dark theme, a `-C` CSS file setting
  `--background-primary`, and look at each PNG.
