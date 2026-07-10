# Input System

`Engine\Src\EngineCore\Input\` provides gameplay-facing keyboard/mouse action binding, distinct from the generic `EventHandler` pub/sub bus ([Event-System.md](Event-System.md)) — both are fed by the same Win32 message pump but serve different purposes.

## `InputCore.h`

Core types: `EButtonState` (`Pressed`/`Released`/`Hold`), `EMouseButton`, `EAxisType`, `EAxisBinding`, `InputKey` (keycode + display character wrapper). Constants: `MAX_KEYCODES_SUPPORTED = 256`, `MAX_MOUSE_BTN_SUPPORTED = 3`.

## `InputHandler`

Tracks **double-buffered** `KeyboardState`/`MouseState` (current + previous bitsets), so `Hold` can be computed as "down this frame AND down last frame." A second, parallel "proxy" state pair keeps tracking raw OS input even while input is being withheld from gameplay — this is what powers the Editor's ability to steal input for its own camera/UI while the underlying key/mouse state continues to be tracked.

Binding API:
```cpp
void BindAction(keyCode, buttonState, KeyActionFunc);
void BindAxis(bindingType, axisType, Functor<...>);
void BindMouseAction(button, state, MouseActionFunc);
```

`OnKeyDown`/`OnKeyUp`, `OnMouseMove`/`OnMouseWheel`/`OnMouseDown`/`OnMouseUp` are called from `Win32Window::CompileInputMessages`, translating raw WinProc messages into input-handler state updates. `RaiseEvents()` fires every bound callback whose current state matches, once per frame; `Reset()` clears state without firing anything (used when the app is stealing input — see below).

## The focus-stealing hook

`RZE_Application::ProcessInput(const InputHandler&)` returns a `bool`:
- `true` → engine calls `inputHandler.RaiseEvents()` — bound gameplay actions fire normally.
- `false` → engine calls `inputHandler.Reset()` instead — no bound actions fire this frame, because the application is consuming input itself (e.g. the Editor's `ProcessInput` returns whether the `SceneViewPanel` is hovered, so mouse input only reaches the editor camera/gizmos when the viewport actually has focus).

This is the same mechanism documented in [Engine-Startup-And-Frame-Loop.md](../02-Architecture/Engine-Startup-And-Frame-Loop.md)'s per-frame loop.

## Pattern comparison: binding vs pub/sub

`InputHandler`'s `BindAction`/`BindAxis` is a **delegate/binding pattern** — callbacks are registered against a specific key/axis and fire directly when that state matches. This is distinct from `EventHandler`'s generic **publish/subscribe pattern** ([Event-System.md](Event-System.md)), where callbacks register against an event *type* and a queue of heterogeneous events gets dispatched. Both are backed by the same underlying `Functor<>` wrapper type from `Utils`.

## Key files

- `Engine\Src\EngineCore\Input\InputCore.h`
- `Engine\Src\EngineCore\Input\InputHandler.h/.cpp`
- `Engine\Src\Windowing\Win32Window.h/.cpp` (feeds input via `CompileInputMessages`)
