# Utils Library Reference

`Utils` (`RZE\Utils\`) is the foundational layer of the entire stack — it has **zero dependencies** on any other RZE project (`Utils.sharpmake.cs` has no `AddPublicDependency` calls at all). Everything else, directly or transitively, depends on it. Built as a static Lib with a precompiled header (`Src\StdAfx.h`/`.cpp`), which is also where **GLM** is confirmed as the underlying math library (`GLM/glm.hpp`, `common.hpp`, `gtc/matrix_transform.hpp`, `gtc/type_ptr.hpp`) that RZE's own Vector/Matrix/Quaternion types wrap.

## Primitives & containers

- `PrimitiveDefs.h` — canonical fixed-width typedefs used everywhere: `Int8/16/32/64`, `Byte`/`U8/16/32/64`, `Float32/64`.
- `Utils.h` — `ContainerUtils::VectorEraseBack` — O(1) unordered erase (swap-with-back, then pop) used throughout the Engine (e.g. `RenderEngine`'s object containers) instead of `std::vector::erase`, which would shift elements.
- `Functor.h` — `template<TReturnType, class...Args> class Functor` — a copyable/movable wrapper around `std::function`, used pervasively for input-event callbacks, component factories, and job tasks.
- `StringUtils.h` — `StringUtils::ExtractFileNameFromPath`, `StringUtils::FormatString<Args...>` (sprintf-style formatting into a `std::string`, fixed 1024-char buffer).
- `Conversions.h/.cpp` — string/encoding conversions (e.g. `StringToWString`, used for D3DPERF event names in the render thread).
- `GUID.h/.cpp` — `GUIDHelper::GenerateGUID()`, wrapping Win32 `GUID`/`guiddef.h`.

## Math (`Math\`) — thin wrappers over GLM

- `Math.h/.cpp` — `MathUtils` namespace (`Pi`, `ToRadians`/`ToDegrees`, `Clamp`/`Clampf`); `VectorUtils` namespace (`Dot`, `Lerp`, `DistanceSq`).
- `Vector2D.h/.cpp`, `Vector3D.h/.cpp`, `Vector4D.h/.cpp` — wrap `glm::vec2/3/4` (`mVec`); usual operators, `Normalize`/`Normalized`, `Cross`, `LengthSq`, `GetInternalVec()` (raw glm vec, used directly for ImGuizmo interop), `ToString()`.
- `Matrix4x4.h/.cpp` — wraps `glm::mat4`. Statics: `CreateInPlace(pos, scale, rot)`, `CreateViewMatrix`, `CreatePerspectiveMatrix`, `CreateOrthoMatrix`; instance ops `Translate`/`Rotate`/`Scale`, `Inverse`/`Transpose`, `GetPosition`/`GetRotation`/`GetScale` (+ setters), `GetValuePtr()` (raw `float*`, used for ImGuizmo/D3D interop), `operator*` (matrix×matrix, matrix×`Vector4D`), static `IDENTITY`.
- `Quaternion.h/.cpp` — rotation representation used by `Matrix4x4::GetRotation()`.

## Memory (`Memory\`)

Covered in depth in [Memory-Management.md](Memory-Management.md): `ByteStream.h/.cpp` (binary asset serialization), `MemoryUtils.h/.cpp` (`Kilobytes`/`Megabytes`/`Gigabytes`, `AlignSize`).

## Platform (`Platform\`)

- `FilePath.h/.cpp` — `class Filepath`: stores both `m_relativePath` and `m_absolutePath`. `EDirectoryContext { Runtime, Tools }` — a static, **process-wide** context switch (`Filepath::SetDirectoryContext`) that changes how relative paths resolve. Set to `Tools` in `EditorMain.cpp` and `SourceAssetBurnerMain.cpp`; left at the default (`Runtime`) in the Game. This single toggle is what lets identical relative path strings like `"Assets/Scenes/..."` resolve correctly whether the code is running as an offline tool or the shipped game. Also: `CreateDir`, `FromAbsolutePathStr`, `Exists`, `GetAbsolutePath`/`GetRelativePath`, `GetAbsoluteDirectoryPath`/`GetRelativeDirectoryPath`, `IsValid`.
- `File.h/.cpp` — `class File` wraps `std::ofstream` with an `EFileOpenMode` (`Append`/`AtEnd`/`Binary`/`Read`/`Write`/`Truncate`/`Closed`, mapped onto `std::ios_base::openmode`), templated `Overwrite<T>`/`Write<T>`, `operator<<`, `Content()`.
- `CmdLine.h/.cpp` — `CmdLine::Arguments::Initialize(argv, argc)` / `Get(name, out std::string_view)` — simple `flag=value` command-line parsing, used by `RZE_Application::ParseArguments` and both app subclasses (e.g. `-scene`).
- `Timers\HiResTimer.h/.cpp` — high-resolution timer backing `START_TIMED_BLOCK` in `Debug.h`.

## Interfaces

- `Interfaces\Resource.h` — `class IResource`: `virtual bool Load(const Filepath&)`, `virtual void Release()`, plus a texture-specific `virtual bool Load(const U8* buffer, int width, int height)` overload (called "gross" in its own source comment). This is the contract `ResourceHandler` and every loadable resource type (`StaticMeshResource`, `EngineConfig`, texture resources) build on. See [Asset-Import-Runtime.md](../05-AssetPipeline/Asset-Import-Runtime.md).

## Reflection (`Reflect\`) — a second, separate reflection system

- `Reflection.h` — macros `REFLECT_REGISTER_COMPONENT(Type)` / `REFLECT_REGISTER_COMPONENT_CHILD(Type, ParentType)` (used by `EditorCameraComponent`'s constructor), and `REFLECT_NAME(Type)`.
- `ReflectDB.h/.cpp` — `class ReflectDB` (all-static): `RegisterComponent(typeStr)`, `RegisterComponentChild(typeStr, parentTypeStr)`, `GetIDFromName(typeName)`. Keeps `std::unordered_map<ReflectTypeID, ReflectTypeDescriptor>` where `ReflectTypeDescriptor { name, typeID, parentTypeID, children }`. `ReflectTypeID` is generated via `std::hash<const char*>` on the type-name string.

**This system is independent of, and not the same as,** the `GameObjectComponentRegistry`/`GameObjectComponentTypeID` system described in [Component-System.md](../03-SceneAndGameObjects/Component-System.md) — that system drives serialization/factory IDs; `ReflectDB` is used only for the Editor's inspector sort-ordering today (`EditorCameraComponent` calls both `REGISTER_GAMEOBJECTCOMPONENT` machinery *and* `REFLECT_REGISTER_COMPONENT_CHILD` for different purposes). See [10-KnownIssues](../10-KnownIssues/Architectural-Notes-And-TODOs.md) for a note on a latent risk in `ReflectDB`'s use of `std::hash<const char*>`.

## Key files

- `Utils\Src\StdAfx.h` — confirms GLM as the underlying math library
- `Utils\Src\Utils\PrimitiveDefs.h`, `Utils.h`, `Functor.h`, `StringUtils.h`, `Conversions.h`, `GUID.h`
- `Utils\Src\Utils\Math\Math.h`, `Vector2D.h`, `Vector3D.h`, `Vector4D.h`, `Matrix4x4.h`, `Quaternion.h`
- `Utils\Src\Utils\Memory\ByteStream.h`, `MemoryUtils.h`
- `Utils\Src\Utils\Platform\FilePath.h`, `File.h`, `CmdLine.h`, `Timers\HiResTimer.h`
- `Utils\Src\Utils\Interfaces\Resource.h`
- `Utils\Src\Utils\Reflect\Reflection.h`, `ReflectDB.h/.cpp`
