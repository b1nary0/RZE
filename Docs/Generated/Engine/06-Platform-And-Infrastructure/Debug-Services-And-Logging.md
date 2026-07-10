# Debug Services & Logging

## `DebugServices` (`Engine\Src\DebugUtils\DebugServices.h/.cpp`)

A singleton (`static DebugServices& Get()`), thread-safe in-memory log store, guarded by three separate mutexes (entry-add, data-entries, channel-lookups). `LogChannel` enum: `Info`, `Debug`, `Build`. `AddData(text, color)` / `Trace(channel, text)` append a `LogEntry { TextColor, Text }` to `mDataEntries`, with per-channel index lists (`mChannelLookups[ChannelCount]`) so a debug console/editor UI can filter by channel. Initialized early in `RZE_Engine::Init()` alongside `Debug::CreateLogFile()`. The Editor's `LogPanel` reads through this to display build/launch output ([Editor-Camera-And-Build-Launch.md](../07-Editor/Editor-Camera-And-Build-Launch.md)).

## `Debug.h` macros (`Utils\Src\Utils\DebugUtils\Debug.h`) — gated on `_DEBUG`

In Debug builds:
- `AssertFalse`/`AssertEqual`/`AssertIsNull`/`AssertNotNull`/`AssertExpr`/`AssertMsg` — map to `assert()`.
- `RZE_LOG(Msg)`, `RZE_LOG_ARGS(Msg, ...)`, `LOG_CONSOLE_ANNOUNCE(Msg, ...)` — build a `std::stringstream`, `printf_s` it, and also write to the log file via `Debug::WriteToLogFile`.
- `START_TIMED_BLOCK(Name)` — creates a `ScopedHiResTimer` (backed by `Utils\Platform\Timers\HiResTimer.h`).

In non-Debug builds, **all of the above compile to no-ops** — zero overhead in Release/Retail.

## The `IMGUI_ENABLED` coupling

`#define IMGUI_ENABLED` lives inside this same header's `_DEBUG` branch. This means whether the shipped **Game** executable compiles in ImGui at all is controlled by the same header that defines assert/log macros — a somewhat surprising coupling to be aware of if you're trying to trace why ImGui disappears in a Release/Retail Game build. See `GameApp::Initialize()`/`Update()` in [Game-App.md](../08-Game/Game-App.md), which conditionally compiles ImGui usage behind this macro.

## File logging

`Debug::CreateLogFile()`, `Debug::GetLogFile()`, `Debug::WriteToLogFile()` — backed by the `File` class (`Utils\Src\Utils\Platform\File.h`).

## Key files

- `Engine\Src\DebugUtils\DebugServices.h/.cpp`
- `Utils\Src\Utils\DebugUtils\Debug.h`
- `Utils\Src\Utils\Platform\Timers\HiResTimer.h/.cpp`
