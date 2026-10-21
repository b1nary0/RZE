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
| **Right-click** | Scene panel | Context menu: the object's name, Add Component, DetachFromParent, Delete (on empty space with nothing selected: Create GameObject) |
| **Right-click the name** in that menu | Scene panel | Rename: type the new name, then Enter |

Details: [Editor-Panels.md](Editor-Panels.md).

## File

| Shortcut | Action |
|---|---|
| **Ctrl + S** | Save the scene, as File > Save Scene does: a scene without a file asks for one in the file browser first |

It works anywhere in the Editor, but waits like undo and redo: not while a widget or the gizmo is held, so a drag or an unfinished text edit isn't saved halfway (press Enter to finish the edit first). Not while a scene is loading.

## Edit

| Shortcut | Action |
|---|---|
| **Ctrl + Z** | Undo the last step (a whole drag is one step) |
| **Ctrl + Y** or **Ctrl + Shift + Z** | Redo |
| **Ctrl + C** | Copy the selected objects (with their children) |
| **Ctrl + V** | Paste: new copies beside the originals, each paste one step further |
| **Ctrl + D** | Duplicate the selection, one step beside it |

These work anywhere in the Editor, except that a focused text field keeps its own Ctrl+C/V/Z. They're also in the Edit menu. Undo and redo wait while a widget or the gizmo is held, so a drag isn't half undone. Details: [Editor-Overview.md](Editor-Overview.md#edit-copy-paste-and-duplicate), [Editor-Framework.md](Editor-Framework.md#undo-and-redo).

## Other

| Shortcut | Where | Action |
|---|---|---|
| **Escape** | "Create GameObject" name box | Close it without creating |
| **Enter** | "Create GameObject" name box | Create the object |
| **Alt + F4** / window close button | anywhere | Exit, asking about unsaved changes first |
| **Escape** | Unsaved Changes prompt | Cancel |
| **Double-click** / **Enter** | File browser | Open the folder, or choose the file (Save mode: save to the typed name) |
| **Arrow keys** | File browser | Move the selection through the tiles |
| **Backspace** | File browser | Up a folder |
| **Escape** | File browser | Back out of "Replace it?", then close without choosing; in a text field, just leave the field |
| **Double-click** / **Enter** | Content Browser | Open the folder, or load the scene (asking about unsaved changes first) |
| **Arrow keys** | Content Browser | Move the selection through the tiles |
| **Backspace** | Content Browser | Up a folder |
| **F2** | Content Browser | Rename the selected file or folder: Enter renames, Escape cancels |
| **Delete** | Content Browser | Delete the selected file or folder, after asking (undoable: it goes to the editor's trash) |
| **Mouse back / forward** (buttons 4 / 5) | Content Browser hovered | Back / forward through the folders visited |
| **Drag a mesh tile** onto the SceneView | Content Browser | Load the mesh in the background, then add an object showing it where it's dropped |
| **Drag a file tile** onto an asset field | Content Browser → Object Inspector | Set it, if the field takes that kind of file; a mesh, material or texture loads in the background first |

Save Scene is the File menu's only shortcut, and the View and Help menus have none. The SceneView toolbar's tooltips name the shortcut for each control, e.g. "Shortcut (W)".

## Where shortcuts are bound

| Shortcuts | Code |
|---|---|
| Camera | `EditorCameraComponent::Update` → input handling, `Editor\Src\Game\World\GameObjectComponents\EditorCameraComponent.cpp` |
| Gizmo W/E/R/Q/X | `SceneViewPanel::RegisterInputs`, from `Initialize` (engine `InputHandler::BindAction`) |
| Ctrl during a gizmo drag | `SceneViewPanel::Draw` → `GizmoSnapper::PrepareManipulate` (ImGui's `KeyCtrl`) |
| Picking and marquee | `SceneViewPanel::HandleSelectionInput` |
| Escape in the SceneView | `EditorApp::RegisterInputEvents` |
| Scene panel | `ScenePanel::Draw` (ImGui item clicks) |
| Ctrl+S, Ctrl+Z/Y/Shift+Z, Ctrl+C/V/D | `MainMenuBar::HandleShortcuts` (`ImGui::Shortcut` with `ImGuiInputFlags_RouteGlobal`) |
| Window close | `EditorApp::OnCloseRequested` |
| File browser | `FileBrowserModal::HandleKeys` |
| Content Browser | `ContentBrowserPanel::HandleNavigationInput`; arrow keys in `FileTileGrid::HandleArrowKeys` |
| Dropping a mesh | `SceneViewPanel::HandleAssetDrop` |

When adding a shortcut, check this page for conflicts and add a row here.
