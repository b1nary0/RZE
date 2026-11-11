# Input System

`Engine\Src\EngineCore\Input\` provides gameplay-facing keyboard/mouse action binding, distinct from the generic `EventHandler` pub/sub bus ([Event-System.md](Event-System.md)) — both are fed by the window's one pump of SDL events ([Windowing.md](Windowing.md)) but serve different purposes.

## `InputCore.h`

Core types: `EButtonState` (`Pressed`/`Released`/`Hold`), `EMouseButton`, `EAxisType`, `EAxisBinding`, `InputKey` (keycode + display character wrapper). Constants: `MAX_KEYCODES_SUPPORTED = 256`, `MAX_MOUSE_BTN_SUPPORTED = 3`.

## Key codes

`KeyCode` (`KeyCodes.h`) is the engine's key enum, the same on every platform. Its values are Windows' virtual-key codes (`VK_*`), which they always were, so saved bindings and the code that names keys (`KeyCode::W`, `KeyCode::Escape`) didn't change when the window moved to SDL. `Window.cpp` translates SDL's keys to them:

- by **key code** (`SDL_Keycode`), not by scancode, so a key is the one its label says on the user's layout, as virtual-key codes are (on AZERTY, `KeyCode::A` is the key marked A);
- left and right Shift, Control and Alt become `Shift`, `Control` and `Menu`, as Windows reports them;
- the keypad's digits are `Numpad0`–`Numpad9` with Num Lock on, and the keys printed under them (arrows, Home, End...) with it off, as on Windows.

Alt and F10 now reach the engine and ImGui too, which they didn't before SDL (Windows sends them as system keys).

## `InputHandler`

Tracks **double-buffered** `KeyboardState`/`MouseState` (current + previous bitsets), so `Hold` can be computed as "down this frame AND down last frame." A second, parallel "proxy" state pair keeps tracking raw OS input even while input is being withheld from gameplay — this is what powers the Editor's ability to steal input for its own camera/UI while the underlying key/mouse state continues to be tracked.

Binding API:
```cpp
void BindAction(keyCode, buttonState, KeyActionFunc);
void BindAxis(bindingType, axisType, Functor<...>);
void BindMouseAction(button, state, MouseActionFunc);
```

`OnKeyDown`/`OnKeyUp`, `OnMouseMove`/`OnMouseWheel`/`OnMouseDown`/`OnMouseUp` are called from `Window::CompileInputMessages`, translating SDL's events into input-handler state updates. A key released in the same pump it was pressed in is told as released in the next one, so every press lasts at least a frame ([Windowing.md](Windowing.md#window-windowhcpp)). `RaiseEvents()` fires every bound callback whose current state matches, once per frame; `Reset()` clears state without firing anything (used when the app is stealing input — see below).

The window hands over every queued event each frame, so one frame can bring several of each. Mouse wheel turns add up within a frame rather than the last one winning (in Windows' units, 120 a notch); the wheel only reports deltas, so the window clears it (`ClearMouseWheel()`, state only) at the start of each frame, before that frame's events.

## Interaction and focus

Once a frame, after the window's input events, the engine calls `UpdateFrameState(isWindowFocused, isMouseButtonDown)` with the window's view of things:

- **Losing focus.** When the window has just lost focus, `ReleaseAll()` lets go of every key and mouse button still down, as if each was released, since their releases will go to another window. Together with SDL keeping the mouse while a button is down ([Windowing.md](Windowing.md)), this keeps the engine, the Editor's camera and ImGui from holding input the window never sees released.
- **`IsInteracting()`**: whether this frame had input (a key, a button, a wheel turn, or a mouse move that changed the position; a held key repeats, so it counts), or a mouse button is held while the window is focused. The held button is asked of the OS (`Window::IsMouseButtonDown`, SDL's global mouse state), so it can't get stuck down. The engine uses it to update the UI every frame while the user works ([Engine-Startup-And-Frame-Loop.md](../02-Architecture/Engine-Startup-And-Frame-Loop.md#ui-frame-rate)).
- **`GetInputEventCount()`** counts those input events since start-up. It only goes up, so a change since last time means there was input in between.

## The focus-stealing hook

`RZE_Application::ProcessInput(const InputHandler&)` returns a `bool`:
- `true` → engine calls `inputHandler.RaiseEvents()` — bound gameplay actions fire normally.
- `false` → engine calls `inputHandler.Reset()` instead — no bound actions fire this frame, because the application is consuming input itself (e.g. the Editor's `ProcessInput` returns whether the `SceneViewPanel` is hovered, so mouse input only reaches the editor camera/gizmos when the viewport actually has focus).

This is the same mechanism documented in [Engine-Startup-And-Frame-Loop.md](../02-Architecture/Engine-Startup-And-Frame-Loop.md)'s per-frame loop.

## Pattern comparison: binding vs pub/sub

`InputHandler`'s `BindAction`/`BindAxis` is a **delegate/binding pattern** — callbacks are registered against a specific key/axis and fire directly when that state matches. This is distinct from `EventHandler`'s generic **publish/subscribe pattern** ([Event-System.md](Event-System.md)), where callbacks register against an event *type* and a queue of heterogeneous events gets dispatched. Both are backed by the same underlying `Functor<>` wrapper type from `Utils`.

## Key files

- `Engine/Src/EngineCore/Input/InputCore.h`, `KeyCodes.h`
- `Engine/Src/EngineCore/Input/InputHandler.h/.cpp`
- `Engine/Src/Windowing/Window.h/.cpp` (feeds input via `CompileInputMessages`)
- `UnitTests/Src/Engine/InputHandlerTests.cpp`
