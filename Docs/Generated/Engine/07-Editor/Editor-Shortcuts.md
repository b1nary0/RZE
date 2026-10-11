# Editor Shortcuts

Every keyboard and mouse shortcut in the Editor, in one place. Most SceneView shortcuts only fire while the **mouse is over the SceneView**: `EditorApp::ProcessInput` only lets engine input actions through then, so typing in a panel doesn't move the camera or switch the gizmo.

## Camera (SceneView hovered)

| Shortcut | Action |
|---|---|
| **Hold RMB + move mouse** | Look around |
| **Hold RMB + W / A / S / D** | Fly forward / left / back / right (speeds up the longer it's held) |
| **Hold RMB + E / Q** | Fly up / down, relative to the view |
| **Hold R** while flying | ×4 speed |
| **Hold Space** while flying | ÷8 speed |
| **Hold MMB + move mouse** | Orbit around the pivot |
| **Shift + MMB drag** | Pan |
| **Ctrl + MMB drag** | Dolly (drag down to pull back, up to push in) |
| **Mouse wheel** | Dolly toward the pivot |

Details: [Editor-Camera-And-Build-Launch.md](Editor-Camera-And-Build-Launch.md).

## Transform gizmo (SceneView hovered, RMB not held)

| Shortcut | Action |
|---|---|
| **W** | Translate mode |
| **E** | Rotate mode |
| **R** | Scale mode |
| **Q** | Toggle Local / World space |
| **X** | Toggle snapping |
| **Hold Ctrl** while dragging the gizmo | Invert snapping for this drag (snap when off, move freely when on) |

W/E/R/Q/X are ignored while RMB is held, because those keys fly the camera then. Snapping details: [Gizmo-Snapping-And-Grid.md](Gizmo-Snapping-And-Grid.md).

## Selection

| Shortcut | Where | Action |
|---|---|---|
| **Click** | SceneView | Select the object under the cursor; on nothing, clear the selection |
| **Ctrl + click** | SceneView | Add the object to the selection, or remove it |
| **Drag** (over 4 px) | SceneView | Marquee: select everything in the rectangle, including hidden objects |
| **Ctrl + drag** | SceneView | Marquee that adds to the selection |
| **Escape** | SceneView hovered | Clear the selection |
| **Click** | Scene panel | Select the object |
| **Ctrl + click** | Scene panel | Add the object to the selection, or remove it |
| **Click empty space** | Scene panel | Clear the selection |
| **Double-click** | Scene panel | Frame the object with the editor camera |
| **Drag and drop** onto another object | Scene panel | Parent it to that object |
| **Right-click** | Scene panel | Context menu: Rename, Add Component, Detach From Parent, Delete (on empty space with nothing selected: Create GameObject) |

Details: [Editor-Panels.md](Editor-Panels.md).

## Other

| Shortcut | Where | Action |
|---|---|---|
| **Escape** | "Create GameObject" name box | Close it without creating |
| **Enter** | "Create GameObject" name box | Create the object |
| **H** | SceneView hovered | Debug leftover: detaches the object named "M4" from its parent (`EditorApp::RegisterInputEvents`). It assumes M4 exists, so avoid it in other scenes. |

The File, View and Help menus have no keyboard shortcuts.

## Where shortcuts are bound

| Shortcuts | Code |
|---|---|
| Camera | `EditorCameraComponent::Update` → input handling, `Editor\Src\Game\World\GameObjectComponents\EditorCameraComponent.cpp` |
| Gizmo W/E/R/Q/X | `SceneViewPanel::Temp_RegisterInputs` (engine `InputHandler::BindAction`) |
| Ctrl during a gizmo drag | `SceneViewPanel::Display` → `GizmoSnapper::PrepareManipulate` (ImGui's `KeyCtrl`) |
| Picking and marquee | `SceneViewPanel::HandleSelectionInput` |
| Escape / H in the SceneView | `EditorApp::RegisterInputEvents` |
| Scene panel | `ScenePanel::Display` (ImGui item clicks) |

When adding a shortcut, check this page for conflicts and add a row here.
