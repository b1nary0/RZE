# Status Bar & Settings Window

Two pieces of Editor chrome: the **status bar** along the bottom, which follows background work, and the **Settings window** (Edit > Settings), a set of settings pages that new features add to.

## The status bar

`StatusBar` (`UI\StatusBar.h/.cpp`), owned and drawn by `EditorApp` right after the main menu bar. It takes its height out of the viewport's work area, so the dockspace fits above it. It's made of fixed-width items: **"Ready"** and the **frame stats** on the left, and the **tasks area** on the right.

```
| Ready   16.7 ms · 60 FPS · idle 9.8 ms         (o) [Rendering thumbnails +1  33%] |
                                                  |   '-- box: fills with the accent colour as the task progresses
                                                  '-- status circle
```

**The frame stats** are the frame time, frames a second and idle time per frame, averaged over half a second by the `FramePacer` (`FramePacer::Stats`, [Engine-Startup-And-Frame-Loop.md](../02-Architecture/Engine-Startup-And-Frame-Loop.md#frame-pacing)). Hovering opens a panel just above them, like the tasks one, with how many times a second the UI updates, and the idle time split into what went to tasks (`AsyncOperationManager::TickIdle`) and what was spent waiting for the next frame. Frames only have idle time with vsync or a frame rate cap. They replace the menu bar's old frame time readout.

**The tasks area** shows the running `AsyncOperation`s that the engine's visibility filter matches (`GetVisibleOperations`, [Asset-Import-Runtime.md](../05-AssetPipeline/Asset-Import-Runtime.md#which-operations-are-shown)). Operations only say what they are; **Settings > Background Tasks** and Engine.ini's `[BackgroundTasks]` decide which show ([below](#pages)). By default that's everything except steps of other tasks: opening a scene (which also has its progress modal), the Content Browser's "Loading thumbnails" (one task per folder), thumbnail runs, assets loading for the [Object Inspector](Object-Inspector-And-Properties.md#asset-fields) or a [mesh dropped in the scene view](Editor-Panels.md#dropping-meshes), and game builds.

**The status circle** says how tasks are going at a glance (`ETaskStatus`):

| Colour | Means |
|---|---|
| Green | The last task succeeded, or none has run |
| Yellow | A task is running |
| Orange | The last task finished with warnings, or was cancelled |
| Red | The last task failed |

**The box** (240 px) is always there and always clickable:

- **While tasks run** it fills with the accent colour as the oldest one progresses. On its one line it always shows how many others are running ("+1") and the oldest's percentage, and fits the name in what's left.
- **With none running** it shows the last task's name and how it went, in the circle's colour ("Done", "Warnings", "Cancelled" or "Failed"), or "No background tasks", greyed out, if none has run.

| | While tasks run | With none running |
|---|---|---|
| **Hover** | A line per running task at a glance: a dot for how it's going, its name, roughly how long it has left and its percentage | How the last task went, in full (its warnings, error or status) |
| **Click** | Each running task's progress and status, with a **Cancel** button | **History:** each run of a task under its name, oldest first: when it started and ended, its warnings, and its **last 5 steps** ("...and 60 more in the full history" where the rest would be). It opens on the newest and follows new entries while it's scrolled to the bottom. **Full history** beside the heading opens [Task History](#task-history). |

Both open just above the box, as wide as it, and stay put rather than following the mouse.

### `TaskTracker`

`UI\TaskTracker.h/.cpp` is what the status bar and Task History show, kept separate from the drawing so it's unit tested. `EditorContext` owns it (`GetTasks()`), and `EditorApp::UpdateUI` updates it once a UI frame with `GetVisibleOperations()`, before anything draws. It keeps:

- the **running** tasks, oldest first;
- a **note** about the last one to finish ("Rendering thumbnails finished: 12 rendered", "... finished with warnings: ...", "... failed: ...", "... cancelled") until another finishes;
- the **history**: a timestamped entry when each task starts, for each step it logs (`AsyncOperation::LogStep`) and warning it adds (`AddWarning`), and how it ended. Each entry has its **run** number, given out as the task starts, so runs at the same time stay apart even with the same name. It keeps up to 20,000 entries, dropping the oldest. `GroupByRun` gathers it into runs, `FindRunning(run)` gives a run's operation while it's still running, and `ClearHistory` forgets it.

A task that stops matching the filter while it runs leaves without an ending; if it's shown again it starts again in the history.

**Time left** comes from the operation (`AsyncOperation::EstimateSecondsLeft`): once it's at least 5% done and has run for a second, at the rate it's gone so far, shown short by `TimeUtils::FormatDuration` ("8s", "1m 05s", "2h 03m"). It assumes the rest goes as fast as what's done, so a batch whose last items are its largest finishes later than it said.

So for new long-running work to show up here, make it an `AsyncOperation` with the right kind, flag it `SubStep` if another operation reports on it, and have it `LogStep` and `AddWarning` as it goes. Many small operations that make one task go in an `AsyncOperationGroup` with a label each.

### Task History

`TaskHistoryPanel` (`UI\Panels\TaskHistoryPanel.h/.cpp`) is a floating panel, closed until opened from **Full history** in the status bar's history or **View > Windows > Task History**. It shows the whole history, so a task's every step can be looked back on once it's done, e.g. each image a "Loading thumbnails" loaded.

```
+ Task History ------------------------------------------------------------------+
| [Search tasks and what they did]  [ ] Problems only  [Copy] [Clear]  3 runs    |
| Task                       Result                        Started    Took       |
| > Rendering thumbnails     Finished: 16 rendered         01:23:34   54s        |
| v Loading thumbnails       Finished: 65 done             01:23:14   5s         |
|     arch_stone_wall_01_Metalness.png                     01:23:14              |
|     ...                                                                        |
| > Loading Scene            Finished: Creating objects    01:23:06   0s         |
+--------------------------------------------------------------------------------+
```

- **A row per run**, newest first: how it ended in its colour (or "Running, 68%" while it runs), when it started, and how long it took (counting up while it runs, from the operation's own running time).
- **Opening a run** lists everything it did with the time; long runs draw only the rows in view.
- **Search** matches task names and what they did. A run that matches only in some of its entries opens on its own, listing just those.
- **Problems only** keeps the runs that failed, were cancelled or warned.
- **Copy** puts the runs shown on the clipboard as text; **Clear** forgets the history.

## The Settings window

`SettingsPanel` (`UI\Panels\SettingsPanel.h/.cpp`) is an ordinary [panel](Editor-Framework.md#panels): floating, closed until opened from **Edit > Settings**, 760×460 and centred the first time (`PanelDesc::DefaultSize`). Its pages are listed down the left, sorted by name, and the chosen one is drawn on the right under its name as a heading.

```
+ Settings ------------------------------------------------------------+
| Background   | Thumbnails                                            |
|  Tasks       | ----------------------------------------------------- |
| Frame Rate   | Thumbnails render in the background, one at a time... |
| Scene View   |                                                       |
| >Thumbnails  |                                                       |
|              | -- Scenes ------------------------------------------- |
|              | 7 of 7 up to date                                     |
|              | In ProjectData\SceneThumbnails                        |
|              | [Update] [Regenerate All]                             |
|              | -- Meshes ------------------------------------------- |
|              | 9 of 12 up to date . 3 out of date                    |
|              | In ProjectData\MeshThumbnails                         |
|              | [Update] [Regenerate All]                             |
+--------------+-------------------------------------------------------+
```

### Pages

| Page | Shows |
|---|---|
| **Background Tasks** | Which tasks the status bar shows: **Steps of other tasks** (`SubStep`, e.g. each image of a "Loading thumbnails"), **Tasks with a progress dialog** (`Blocking`, e.g. opening a scene), and one checkbox per kind: **Resource loads**, **Scene loads**, **Rendering**, **Saving** and **Other**. It changes the engine's `AsyncOperationFilter` (`SetVisibilityFilter`) at once. Hidden tasks still run. These last until the editor closes; Engine.ini's `[BackgroundTasks]` sets them at start-up, everything but steps by default ([Config-System.md](../06-Platform-And-Infrastructure/Config-System.md)). |
| **Frame Rate** | **Vsync**, the **max frame rate** (Off, 30, 60, 120, 144, 240), the frame rate **while unfocused** (Off, 10, 15, 30, 60) and the **UI rate** (Every frame, 60, 30, 20, 15): how often the panels and menus update while you're not using the mouse or keyboard. They're `FrameSettings`, applied at once through `RZE_Engine::SetFrameSettings`, and last until the editor closes; Engine.ini's `[FrameSettings]` sets them at start-up ([Config-System.md](../06-Platform-And-Infrastructure/Config-System.md)). Engine.ini can set rates the page doesn't list, and then none of its options is picked. |
| **Scene View** | The gizmo's **move step** (0.25–1 m, also the grid's square size and the paste offset) and **rotate step** (15–90°). These are the same `EditorSettings` values the scene view's toolbar edits, drawn by the same code (`GizmoSnapper::DrawSnapSizeOptions` / `DrawRotationStepOptions`), so the two always agree. See [Gizmo-Snapping-And-Grid.md](Gizmo-Snapping-And-Grid.md). |
| **Thumbnails** | For scenes and for meshes: how many thumbnails are up to date, out of date and missing, and their folder. **Update** renders the missing and out-of-date ones (greyed out when all are up to date); **Regenerate All** renders every one again. Both queue the work on the background `ThumbnailRenderer`, so the status bar shows it and the editor carries on. The counts (`FindFiles` + `Thumbnails::Survey`) refresh every second while the page is shown. |

### Adding a page

```cpp
// MyPage.h
class MyPage : public SettingsPage
{
public:
    std::string GetName() const override { return "My Feature"; }   // in the list, and as the heading
    void Draw(EditorContext& context) override;                     // while it's the page shown
};

// MyPage.cpp
REGISTER_SETTINGS_PAGE(MyPage);
```

Put it in `Editor\Src\UI\Settings\` and run `GenerateSolutions.bat`. The window creates one of each registered page when it's created (`SettingsPageRegistry::CreateAll`, sorted by name). Registration works like panels', so it relies on `Editor` linking `EditorLib` with `/WHOLEARCHIVE` ([Editor-Framework.md](Editor-Framework.md#panels)). A preference more than one place reads goes in `EditorSettings` and gets its control on a page.

Settings last for the session only; nothing is saved between runs yet. Settings that describe the game rather than the editor go in [Project Settings](Editor-Panels.md#project-settings-projectsettingspanel), which saves to `Config\Game.ini`.

## Key files

- `Editor\Src\UI\StatusBar.h/.cpp`, `TaskTracker.h/.cpp`, `TaskColours.h/.cpp`
- `Editor\Src\UI\Panels\TaskHistoryPanel.h/.cpp`, `SettingsPanel.h/.cpp`
- `Editor\Src\UI\Settings\SettingsPage.h`, `SettingsPageRegistry.h/.cpp`, `BackgroundTasksSettingsPage.h/.cpp`, `FrameRateSettingsPage.h/.cpp`, `SceneViewSettingsPage.h/.cpp`, `ThumbnailsSettingsPage.h/.cpp`
- `Engine\Src\EngineCore\Async\AsyncOperationFilter.h/.cpp`
- `Editor\Src\UI\Framework\EditorSettings.h/.cpp`
- `UnitTests\Src\Editor\TaskTrackerTests.cpp`, `SettingsPageRegistryTests.cpp`
