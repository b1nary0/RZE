# App icon and Project Settings

Status: Step 1 done. Editor.exe and Game.exe embed `Assets/Application/RZE.ico` and use it for their window, taskbar
button and Explorer icon. Step 2 (Project Settings and `Config/Game.ini` for the game's own icon) is still to do.

## Context

Both executables show the stock Windows icon. `Win32Window::Create` (`Engine/Src/Windowing/Win32Window.cpp:88,93`)
registers its class with `LoadIcon(0, IDI_APPLICATION)` and a `@TODO fill this with proper customizable icon`. The
repo has no `.ico`, `.rc` or icon code. The editor hides the OS caption (custom title bar), so the icon only shows
in the taskbar, Alt+Tab, the system menu and, once it's embedded, Explorer.

There are two goals:

1. **Step 1:** RZEStudio (and, by default, the Game) shows the RZE mark in the taskbar. The mark is the hexagon
   with the gizmo, without the "RZE" letters or the divider.
2. **Step 2:** a new **Project Settings** window in the editor lets you pick the game's icon. The choice is written
   to a new `Config/Game.ini`, and Game.exe uses it for its window.

## What exists already

| Piece | Where | Use for this |
|---|---|---|
| Logo source art | `Assets/Editor/logo.svg` | The hexagon and gizmo shapes (lines 7-14) become the icon's source |
| Window class registration | `Win32Window::Create`, `Win32Window.cpp:81-93` | Where the default icon gets set |
| Sharpmake projects | `Editor/Editor.sharpmake.cs`, `Game/Game.sharpmake.cs`, `Make/Sharpmake/BaseProject.sharpmake.cs` | Add the `.rc` files. vcxprojs are generated, not committed, so regenerate with `GenerateSolutions.bat` |
| INI config resource | `EngineCore/Config/Config.h` (SimpleIni, read-only), `EngineConfig` | Pattern for `GameConfig`. SimpleIni already has `SetValue`/`SaveFile` |
| Config loading | `RZE_Engine::LoadEngineConfig`, `Engine.cpp:399-407` | `LoadResource<EngineConfig>(Filepath("Config/Engine.ini"))` pattern |
| PNG loading | stb_image (`Texture2D::LoadCPU`, `Texture2D.cpp:40-52`) | Load the chosen icon's pixels (RGBA, to be converted to BGRA for Win32) |
| Settings window | `UI/Panels/SettingsPanel.*`, `UI/Settings/*SettingsPage.*` | Layout and look for Project Settings |
| Panel registration | `REGISTER_EDITOR_PANEL`, `PanelDesc::MenuPath` | `"Edit/Project Settings"` appears in the Edit menu automatically |
| File picker | `FileBrowserModal` (`EditorContext::GetFileBrowser()`) | Pick the PNG; `RequesterID` + `ConsumeResult` for immediate-mode use |
| Path field pattern | `PropertyDrawers.cpp:279-307` (asset path drawer) | Read-only path, `...`, Clear, drag-and-drop |
| Property table helpers | `EditorUI::BeginPropertyTable`/`PropertyRow`/`EndPropertyTable` | The form layout |
| Game launch | `GameBuilder.cpp:166-196` (Build and Launch Game) | How to test Step 2 end to end |

## Step 1: The RZE mark as the app icon

Goal: the RZE mark appears on the taskbar, in Alt+Tab and on `Editor.exe`/`Game.exe` in Explorer, starting from the
first frame.

### 1. Icon art

- **New `Assets/Application/icon.svg`.** Copy the hexagon, the three gizmo arrows and the centre dot from `logo.svg`,
  without the text or the divider. Use a square `viewBox="32 42 448 448"`: the hexagon (with its stroke) spans
  y 42-490 and is centred on x 256, so this crops tightly with the gizmo centred. Like `logo.svg`, give it a header
  comment saying how the `.ico` was made. It lives in `Assets/Application`, not `Assets/Editor`, since both
  executables use it.
- **Small sizes need heavier strokes.** At 16 px the 20-unit outline is 0.7 px and the arrows are about 1 px, so
  they'd blur away. Draw a second variant for 16-24 px with a thicker outline and arrows and bigger arrowheads, and
  drop the centre dot if it clutters. Render both and look at them at 100% before committing.
- **New `Assets/Application/RZE.ico`,** with frames at 16, 20, 24, 32, 40, 48, 64 and 256 px (PNG-compressed at 256). To
  make it, draw the shapes at 8x with Pillow (the art has no text, so it doesn't need an SVG renderer), downscale
  with Lanczos, and save with Pillow's multi-size ICO writer. Use a one-off script in the scratchpad, and record the
  method in the SVG comment, the same as `logo.png`.

### 2. Embed it in both executables

- **New `Editor/Resources/Editor.rc` and `Game/Resources/Game.rc`.** Each contains
  `1 ICON "..\..\Assets\Editor\RZE.ico"`. Check during implementation that rc.exe resolves the path from the
  `.rc`'s folder; if not, add an include path.
  - ID 1 is the lowest ID, so Explorer uses it for the `.exe`.
- **Sharpmake:** add the `.rc` to `Editor` and `Game` explicitly with `ResourceFiles.Add(...)`.
  - `Editor`'s `SourceFilesFiltersRegex` would drop a scanned file, and the `Resources` folder sits outside `Src`
    anyway.
  - Regenerate the projects, then check that the vcxproj has a `ResourceCompile` item.

### 3. Use it for the window class

- In `Win32Window::Create`, load the module's icon 1 with `LoadImage(..., IMAGE_ICON, SM_CXICON/SM_CYICON)` for
  `hIcon` and with `SM_CXSMICON/SM_CYSMICON` for `hIconSm`.
  - `LoadIcon` only picks the large size, so `hIconSm` would be a scaled-down 32 px icon.
  - Fall back to `IDI_APPLICATION` when the exe has no icon resource (SourceAssetBurner and UnitTests are console
    apps and never make a window, but the engine shouldn't assume that).
  - Put the ID in a named constant with a comment pointing at the two `.rc` files, and remove the `@TODO`s.

### Out of scope for Step 1

The ImGui multi-viewport class (`imgui_impl_win32.cpp:1488`) also has no icon. Viewports aren't enabled, so leave it
alone.

## Step 2: Project Settings and Game.ini

Goal: in the editor, **Edit > Project Settings** lets you pick a PNG as the game's icon. The choice saves straight
to `Config/Game.ini`, and Game.exe sets its window icon from it at startup.

### 1. `Config/Game.ini` and `GameConfig`

- **File format:**
  ```ini
  [Game]
  ; The game window's icon: a square PNG under Assets, 256 px or larger. Empty for the RZE icon.
  Icon=Assets/Icons/MyGame.png
  ```
  The path is relative to the project root (the same as other `Filepath`s). Commit a `Game.ini` with an empty
  `Icon=` so the file and its comment exist from the start.
- **Writing INI files:**
  - `Config` (`EngineCore/Config/Config.h`) gets a protected `Write(const Filepath&)` that calls
    `mINIParser.SaveFile`. SimpleIni keeps comments, so the file's comment survives a save.
  - This is the first time the engine writes an INI file. `Engine.ini` stays read-only.
- **New `EngineCore/Config/GameConfig.h/.cpp`,** with `class GameConfig final : public Config`, following
  `EngineConfig`:
  - `Load(const Filepath&)`. A missing file means all defaults.
  - `const std::string& GetIconPath() const`.
  - `void SetIconPath(const std::string&)`, a sink parameter taken by const& and copied. It updates the parser
    value too.
  - `bool Save()`, which writes back to the file it loaded from.
  - It's loaded through the resource handler, `LoadResource<GameConfig>(Filepath("Config/Game.ini"))`, like
    `EngineConfig`.
  - It lives in Engine, not Editor, because the Game reads it.

### 2. Window icons from a PNG

- **New `Windowing/WindowIcon.h/.cpp` in Engine.** It owns a large and a small `HICON` and calls `DestroyIcon` when
  destroyed or replaced.
  - It's built from a file: load RGBA with stb_image, then make one image at `SM_CXICON` and one at `SM_CXSMICON`
    (for the window's DPI, via `GetSystemMetricsForDpi`).
  - Shrink with an area-average filter (written here; stb_image_resize isn't vendored). Make a non-square image
    square with transparent padding, centred.
  - Swap RGBA to BGRA, then `CreateIconIndirect` with a 32-bit DIB section colour bitmap and a mask bitmap.
  - If the file is missing or fails to load, return nothing and log; the window keeps the embedded RZE icon.
- **`Win32Window::SetIcon(const Filepath&)`** builds a `WindowIcon` and sends `WM_SETICON` with `ICON_BIG` and
  `ICON_SMALL`. The window owns the current `WindowIcon` so the icons outlive the `WM_SETICON`.
  - It rebuilds the icon when the window's DPI changes, if the window already reacts to DPI changes. Check this; the
    only DPI hook found so far is `GetDpiScale()`.
  - Only the large/small DIB-to-`HICON` code is Win32-specific, and it stays in Windowing.
- **`GameApp::Initialize`** loads `GameConfig` and calls `GetWindow()->SetIcon(...)` when `Icon` is set.
  - The editor never reads `Icon`, so RZEStudio keeps the RZE mark.
  - The Game's window shows the embedded RZE icon until `Initialize` runs. If the change is noticeable at startup,
    see Open points.

### 3. Project Settings window

- **New `UI/Panels/ProjectSettingsPanel.h/.cpp`,** registered with `REGISTER_EDITOR_PANEL`, with
  `MenuPath = "Edit/Project Settings"`, Floating, `DefaultOpen = false` and `ImGuiWindowFlags_NoCollapse`. It's
  sized and styled like `SettingsPanel`.
  - Editor Settings (`Edit/Settings`) are the editor's own preferences and don't last between runs. Project
    Settings describe the game and are saved, so they're kept apart.
- It has one section for now, **Game**, built as an `EditorUI` property table with an **Icon** row. The row follows
  the asset-path drawer (`PropertyDrawers.cpp:279-307`):
  - A read-only path field.
  - A `...` button that opens `FileBrowserModal` with `RootFolder = "Assets"`, `Extensions = { ".png" }` and
    `RequesterID`.
  - A Clear icon button.
  - Drag-and-drop from the asset browser.
- Under the row, a preview of the chosen image at 16, 32 and 48 px (a `Texture2D` loaded through the resource
  handler, drawn with `EditorUI::TextureID`), so you can see how it reads at taskbar size. With no icon chosen, it
  shows the RZE icon instead.
- A disabled-text note appears when the image isn't square or is smaller than 256 px.
- Changes are saved straight away: each pick or clear calls `SetIconPath` and then `Save`. If saving fails, the
  error goes to the log and an `EditorUI::k_errorColour` line shows in the panel.
- The panel loads `GameConfig` in `Initialize` and releases it on shutdown, like `TitleBar`'s logo handle.
- When another page is needed (title, start scene...), split this into pages with the `SettingsPanel` list layout.
  It isn't built up front.

### Tests (UnitTests, doctest)

- `GameConfig`: write a temp `Game.ini`, load it, set the icon, save, and reload. Check the value round-trips and
  the comment line survives. A missing file gives an empty icon path.
- `WindowIcon`'s scaling (a free function on RGBA buffers, so it's testable without a window):
  - 2x2 to 1x1 averages the pixels.
  - Non-square input is padded and centred.
  - Fully transparent padding stays transparent.

## Open points (defaults chosen; say if you want otherwise)

- **Game.exe's Explorer icon stays the RZE mark.** Matching Game.ini would mean generating an `.ico` and `.rc` in
  `BuildGame.bat` before compiling. That's a follow-up, together with packaging.
- **The Game shows the RZE icon briefly at startup.** If that's noticeable, read `GameConfig` before window
  creation and pass an icon path in `WindowCreationParams` (with the Editor ignoring it). Left out for now to keep
  the engine's start-up order unchanged.
- **PNG only.** `.ico` input could come later. Since the PNG is shrunk at runtime, a 256 px source looks fine.

## Files

- **New:**
  - `Assets/Application/icon.svg` and `Assets/Application/RZE.ico`
  - `Editor/Resources/Editor.rc` and `Game/Resources/Game.rc`
  - `Config/Game.ini`
  - `Engine/Src/EngineCore/Config/GameConfig.h/.cpp`
  - `Engine/Src/Windowing/WindowIcon.h/.cpp`
  - `Editor/Src/UI/Panels/ProjectSettingsPanel.h/.cpp`
  - Unit tests under `UnitTests/Src/Engine/`
- **Changed:**
  - `Editor/Editor.sharpmake.cs` and `Game/Game.sharpmake.cs`
  - `Engine/Src/Windowing/Win32Window.h/.cpp`
  - `Engine/Src/EngineCore/Config/Config.h/.cpp`
  - `Game/Src/GameApp.cpp`

## Verification

- **Step 1:**
  - Run `GenerateSolutions.bat`, build, and check that `Editor_Win64.vcxproj` has `ResourceCompile`.
  - Launch the editor (run-rze skill) and check the taskbar, Alt+Tab and the right-click system menu, at 100% and
    150% scaling.
  - Check `_build\debug\Editor.exe` and `Game.exe` in Explorer (large icons and details view).
- **Step 2:**
  - Open Edit > Project Settings, pick a PNG under `Assets`, and check that `Config/Game.ini` holds the path and
    still has its comment.
  - Run Build and Launch Game and check that the game's taskbar entry uses the PNG.
  - Clear the icon, relaunch, and check the game is back to the RZE mark. Then point `Icon` at a missing file,
    relaunch, and check the game keeps the RZE mark and logs a warning.
  - Run UnitTests.
- **Docs:** a short note about `Game.ini` and Project Settings, in the final commit of Step 2.
