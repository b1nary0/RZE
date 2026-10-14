# Utils Library Reference

`Utils` (`RZE\Utils\`) is the foundational layer of the entire stack — it has **zero dependencies** on any other RZE project (`Utils.sharpmake.cs` has no `AddPublicDependency` calls at all). Everything else, directly or transitively, depends on it. Built as a static Lib with a precompiled header (`Src\StdAfx.h`/`.cpp`), which is also where **GLM** is confirmed as the underlying math library (`GLM/glm.hpp`, `common.hpp`, `gtc/matrix_transform.hpp`, `gtc/type_ptr.hpp`) that RZE's own Vector/Matrix/Quaternion types wrap.

## Primitives & containers

- `PrimitiveDefs.h` — canonical fixed-width typedefs used everywhere: `Int8/16/32/64`, `Byte`/`U8/16/32/64`, `Float32/64`.
- `Utils.h` — `ContainerUtils::VectorEraseBack` — O(1) unordered erase (swap-with-back, then pop) used throughout the Engine (e.g. `RenderEngine`'s object containers) instead of `std::vector::erase`, which would shift elements.
- `Functor.h` — `template<TReturnType, class...Args> class Functor` — a copyable/movable wrapper around `std::function`, used pervasively for input-event callbacks, component factories, and job tasks.
- `StringUtils.h` — `StringUtils::ExtractFileNameFromPath`, `StringUtils::FormatString<Args...>` (sprintf-style formatting into a `std::string`, fixed 1024-char buffer).
- `Conversions.h/.cpp` — string/encoding conversions (e.g. `StringToWString`, used for D3DPERF event names in the render thread).
- `GUID.h/.cpp` — `GUIDHelper::GenerateGUID()`, wrapping Win32 `GUID`/`guiddef.h`.

## Math (`Math\`): thin wrappers over GLM

All spatial values follow [World-Conventions.md](../02-Architecture/World-Conventions.md): metres, right-handed, +Y up, degrees for anything a person sees.

**Convention: glm stays inside these classes.** Code outside `Utils\Src\Utils\Math\` uses `Vector2D/3D/4D`, `Matrix4x4`, `Quaternion` and the `MathUtils` / `VectorUtils` helpers, never `glm::` types or `GetInternalVec()` / `GetInternalMat()` / `GetInternalQuat()`. When something is missing, add a method or helper here rather than reaching for glm.

| File | What's in it |
|---|---|
| `Units.h` | The conventions themselves (as a comment), `Units::Metre / Centimetre / Millimetre`, and `WorldAxes::Right() / Up() / Forward()` (+X, +Y, −Z). |
| `Math.h/.cpp` | `MathUtils`: `Pi`, `ToRadians` / `ToDegrees` (constants to multiply by: `degrees * MathUtils::ToRadians`), `Clamp` / `Clampf`, `SmoothStep(t)`, `CeilPowerOfTwo`. `VectorUtils`: `kEpsilon` / `kEpsilonSq`, `Dot`, `Min` / `Max` (component-wise), `Lerp`, `Slerp`, `DistanceSq`. |
| `Vector2D`, `Vector3D`, `Vector4D` | Wrap `glm::vec2/3/4`. Usual arithmetic, `Normalize` / `Normalized`, `Cross`, `Length` / `LengthSq`, `GetValuePtr()` (const and non-const) hands the components to float-array APIs like ImGui and ImGuizmo. `Vector4D(xyz, w)` and `XYZ()` convert between 3D points and homogeneous coordinates. |
| `Matrix4x4.h/.cpp` | Wraps `glm::mat4` (column-major). Builders: `CreateInPlace(position, scale, eulerDegrees)`, `CreateViewMatrix`, `CreatePerspectiveMatrix` / **`CreatePerspectiveMatrixZeroToOne`** (use this one: D3D's 0..1 depth), `CreateOrthoMatrix` / `CreateOrthoMatrixZeroToOne`, `CreateSubRectProjection(ndcMinX, ndcMinY, ndcMaxX, ndcMaxY)` (narrows a projection to part of the view; used by marquee selection and picking). Instance: `Translate`, `Rotate(angleRadians, axis)`, `Scale`, `Inverse`, `Transpose`, `GetPosition` / `GetRotation` / `GetScale`, `GetRow(i)` (as it multiplies a column vector), `GetValuePtr()`, `operator*` with matrices and `Vector4D`, `IDENTITY`. |
| `Quaternion.h/.cpp` | Rotations. Built from `eulerRadians`, `FromEulerDegrees`, `FromAngleAxis(angleRadians, axis)` or the rotation between two vectors; `ToEulerDegrees()`, `ToAngleRadians()`, `ToAxis()`, `Inverse()`, `Normalized()`, multiplication by quaternions and vectors. |
| `Plane.h/.cpp` | `Plane(normal, distance)`, or from `ax + by + cz + d` coefficients (`Vector4D`). `SignedDistance(point)` is positive on the side the normal points to. |
| `BoundingBox.h/.cpp` | An axis-aligned box. **Empty when made with no arguments**: it holds nothing, and growing it by a point or box makes it exactly that (`IsEmpty()`; growing by an empty box changes nothing). `Around(points)`, `GetCentre`, `GetSize`, `GetRadius` (half the diagonal), `GetCorners` / `GetTransformedCorners(matrix)`, and `Transformed(matrix)`: the axis-aligned box around it once transformed, worked out from the matrix (Arvo's method) instead of 8 corners, for affine transforms. Used for render object, scene and hierarchy bounds, culling and framing. |

`Frustum` (six planes taken from a view-projection matrix, with a box test) lives in the Engine at `Engine\Src\Graphics\Frustum.h`, since only rendering and the Editor use it.

## Memory (`Memory\`)

Covered in depth in [Memory-Management.md](Memory-Management.md): `ByteStream.h/.cpp` (binary asset serialization), `MemoryUtils.h/.cpp` (`Kilobytes`/`Megabytes`/`Gigabytes`, `AlignSize`).

## Platform (`Platform\`)

- `FilePath.h/.cpp` — `class Filepath`: stores both `m_relativePath` and `m_absolutePath`. `EDirectoryContext { Runtime, Tools }` — a static, **process-wide** context switch (`Filepath::SetDirectoryContext`) that changes how relative paths resolve. Set to `Tools` in `EditorMain.cpp` and `SourceAssetBurnerMain.cpp`; left at the default (`Runtime`) in the Game. This single toggle is what lets identical relative path strings like `"Assets/Scenes/..."` resolve correctly whether the code is running as an offline tool or the shipped game. Also: `CreateDir`, `FromAbsolutePathStr`, `Exists`, `GetAbsolutePath`/`GetRelativePath`, `GetAbsoluteDirectoryPath`/`GetRelativeDirectoryPath`, `IsValid`.
- `File.h/.cpp` — `class File` wraps `std::ofstream` with an `EFileOpenMode` (`Append`/`AtEnd`/`Binary`/`Read`/`Write`/`Truncate`/`Closed`, mapped onto `std::ios_base::openmode`), templated `Overwrite<T>`/`Write<T>`, `operator<<`, `Content()`. For writing a file so readers never see half of it: write to `File::GetTemporaryPathFor(path)` (the path plus `.tmp`), then `File::MoveReplacing(temporary, path)` renames it over the old file in one step. It returns false if it couldn't (e.g. the file is open elsewhere) and removes the temporary file either way. Scene saves and PNG writes use it.
- `CmdLine.h/.cpp` — `CmdLine::Arguments::Initialize(argv, argc)`, `Get(name, out std::string_view)` and `GetInt(name, out int)`: simple `-flag value` command-line parsing, used for `-scene` (Editor, Game) and `-launchMonitor` (engine).
- `Timers\HiResTimer.h/.cpp` — high-resolution timer backing `START_TIMED_BLOCK` in `Debug.h`.

## Interfaces

- `Interfaces\Resource.h` — `class IResource`: `virtual bool Load(const Filepath&)`, `virtual void Release()`, plus a texture-specific `virtual bool Load(const U8* buffer, int width, int height)` overload (called "gross" in its own source comment). This is the contract `ResourceHandler` and every loadable resource type (`StaticMeshResource`, `EngineConfig`, texture resources) build on. It also has optional hooks for asynchronous loading (`SupportsAsyncLoad`, `LoadCPU`, `RequestDependencies`, `GetNextFinalizeStepCost`, `FinalizeStep`). See [Asset-Import-Runtime.md](../05-AssetPipeline/Asset-Import-Runtime.md).

## Reflection (`Reflect\`) — a second, separate reflection system

- `Reflection.h` — macros `REFLECT_REGISTER_COMPONENT(Type)` / `REFLECT_REGISTER_COMPONENT_CHILD(Type, ParentType)` (used by `EditorCameraComponent`'s constructor), and `REFLECT_NAME(Type)`.
- `ReflectDB.h/.cpp` — `class ReflectDB` (all-static): `RegisterComponent(typeStr)`, `RegisterComponentChild(typeStr, parentTypeStr)`, `GetIDFromName(typeName)`. Keeps `std::unordered_map<ReflectTypeID, ReflectTypeDescriptor>` where `ReflectTypeDescriptor { name, typeID, parentTypeID, children }`. `ReflectTypeID` is generated via `std::hash<const char*>` on the type-name string.

**This system is independent of, and not the same as,** the `GameObjectComponentRegistry`/`GameObjectComponentTypeID` system described in [Component-System.md](../03-SceneAndGameObjects/Component-System.md) — that system drives serialization/factory IDs; `ReflectDB` is used only for the Editor's inspector sort-ordering today (`EditorCameraComponent` calls both `REGISTER_GAMEOBJECTCOMPONENT` machinery *and* `REFLECT_REGISTER_COMPONENT_CHILD` for different purposes). See [10-KnownIssues](../10-KnownIssues/Architectural-Notes-And-TODOs.md) for a note on a latent risk in `ReflectDB`'s use of `std::hash<const char*>`.

## Key files

- `Utils\Src\StdAfx.h` — confirms GLM as the underlying math library
- `Utils\Src\Utils\PrimitiveDefs.h`, `Utils.h`, `Functor.h`, `StringUtils.h`, `Conversions.h`, `GUID.h`
- `Utils\Src\Utils\Math\Units.h`, `Math.h`, `Vector2D.h`, `Vector3D.h`, `Vector4D.h`, `Matrix4x4.h`, `Quaternion.h`, `Plane.h`, `BoundingBox.h`
- `Utils\Src\Utils\Memory\ByteStream.h`, `MemoryUtils.h`
- `Utils\Src\Utils\Platform\FilePath.h`, `File.h`, `CmdLine.h`, `Timers\HiResTimer.h`
- `Utils\Src\Utils\Interfaces\Resource.h`
- `Utils\Src\Utils\Reflect\Reflection.h`, `ReflectDB.h/.cpp`
